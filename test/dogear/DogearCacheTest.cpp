#include <gtest/gtest.h>

#include <map>
#include <set>

#include "TestSha256.h"
#include "activities/dogear/DogearCache.h"

namespace {
using dogear_cache::Cache;
using dogear_cache::Generation;

// In-memory SD card with a power cut: after `budget` state-changing operations
// the next one is interrupted (a write leaves half its bytes) and every later
// one fails, as if the device lost power mid-commit.
class MemFs : public dogear_cache::FileOps {
 public:
  std::map<std::string, std::string> files;
  int budget = -1;  // -1: no cut
  bool cut = false;

  void powerBack() {
    budget = -1;
    cut = false;
  }
  bool exists(const std::string& p) override { return files.count(p) > 0; }
  bool read(const std::string& p, std::string& out, size_t maxBytes) override {
    const auto it = files.find(p);
    if (it == files.end() || it->second.size() > maxBytes) return false;
    out = it->second;
    return true;
  }
  bool write(const std::string& p, const std::string& data) override {
    if (!tick()) {
      if (!cutWasThisOp) return false;
      files[p] = data.substr(0, data.size() / 2);  // torn write
      return false;
    }
    files[p] = data;
    return true;
  }
  bool rename(const std::string& from, const std::string& to) override {
    if (!tick() || !files.count(from) || files.count(to)) return false;
    files[to] = files[from];
    files.erase(from);
    return true;
  }
  bool remove(const std::string& p) override {
    if (!tick()) return false;
    files.erase(p);
    return true;
  }
  bool list(const std::string& dir, std::vector<std::string>& names) override {
    names.clear();
    for (const auto& f : files) {
      if (f.first.rfind(dir + "/", 0) == 0 && f.first.find('/', dir.size() + 1) == std::string::npos) {
        names.push_back(f.first.substr(dir.size() + 1));
      }
    }
    return true;
  }
  bool ensureDir(const std::string&) override { return !cut; }

 private:
  bool cutWasThisOp = false;
  bool tick() {
    cutWasThisOp = false;
    if (cut) return false;
    if (budget == 0) {
      cut = true;
      cutWasThisOp = true;
      return false;
    }
    if (budget > 0) --budget;
    return true;
  }
};

std::string issue(const std::string& id, const std::string& note) {
  return "{\"schemaVersion\":2,\"issueId\":\"" + id + "\",\"note\":\"" + note + "\"}";
}

struct Fixture {
  MemFs fs;
  Cache cache{fs, testSha256Hex};
  bool commit(const std::string& id, const std::string& bytes) { return cache.commit(id, testSha256Hex(bytes), bytes); }
  std::string best() {
    Generation g;
    return cache.loadBest(g) ? g.bytes : std::string("<none>");
  }
};

TEST(DogearCache, CommitsAndLoads) {
  Fixture f;
  const std::string a = issue("dogear-2026-11-23", "a");
  ASSERT_TRUE(f.commit("dogear-2026-11-23", a));
  EXPECT_EQ(f.best(), a);
  EXPECT_TRUE(f.fs.exists("/dogear/active.txt"));
  EXPECT_FALSE(f.fs.exists("/dogear/active.txt.tmp"));
}

TEST(DogearCache, RefusesBytesThatDoNotMatchTheirHash) {
  Fixture f;
  EXPECT_FALSE(f.cache.commit("dogear-2026-11-23", testSha256Hex("other"), issue("dogear-2026-11-23", "a")));
  EXPECT_EQ(f.best(), "<none>");
}

// Every possible interruption point of a new-week update and of a same-week
// correction leaves a verified generation that loads: the old one, or the new
// one if the commit got far enough. Never nothing.
void interruptEverywhere(const std::string& oldId, const std::string& oldBytes, const std::string& newId,
                         const std::string& newBytes) {
  bool sawOld = false, sawNew = false;
  for (int budget = 0; budget < 40; ++budget) {
    Fixture f;
    ASSERT_TRUE(f.commit(oldId, oldBytes));
    f.fs.budget = budget;
    const bool committed = f.commit(newId, newBytes);
    f.fs.powerBack();
    Fixture after;  // a fresh boot over the same card
    after.fs.files = f.fs.files;
    const std::string loaded = after.best();
    ASSERT_TRUE(loaded == oldBytes || loaded == newBytes) << "budget " << budget << " loaded " << loaded;
    if (committed) EXPECT_EQ(loaded, newBytes) << "budget " << budget;
    sawOld = sawOld || loaded == oldBytes;
    sawNew = sawNew || loaded == newBytes;
    // The card stays usable: the next update succeeds and becomes active.
    const std::string next = issue("dogear-2027-01-04", "next");
    ASSERT_TRUE(after.commit("dogear-2027-01-04", next)) << "budget " << budget;
    EXPECT_EQ(after.best(), next);
  }
  EXPECT_TRUE(sawOld);
  EXPECT_TRUE(sawNew);
}

TEST(DogearCache, NewWeekUpdateSurvivesInterruptionAtEveryStep) {
  interruptEverywhere("dogear-2026-11-23", issue("dogear-2026-11-23", "old"), "dogear-2026-11-30",
                      issue("dogear-2026-11-30", "new"));
}

TEST(DogearCache, SameWeekCorrectionSurvivesInterruptionAtEveryStep) {
  interruptEverywhere("dogear-2026-11-23", issue("dogear-2026-11-23", "v1"), "dogear-2026-11-23",
                      issue("dogear-2026-11-23", "v2 corrected"));
}

// A commit cut off after its temp manifest verified but before it became
// active, on a card with no active generation (an interrupted first install).
// The temp generation loads; the next commit must not lose it.
TEST(DogearCache, InterruptedFirstInstallIsPromotedBeforeTheNextCommit) {
  const std::string a = issue("dogear-2026-11-23", "a");
  // Find the cut that leaves only the verified temp manifest.
  std::map<std::string, std::string> card;
  for (int budget = 0; budget < 40 && card.empty(); ++budget) {
    Fixture f;
    f.fs.budget = budget;
    f.commit("dogear-2026-11-23", a);
    f.fs.powerBack();
    if (!f.fs.exists(dogear_cache::kActive) && f.fs.exists(dogear_cache::kActiveTmp) && f.best() == a) {
      card = f.fs.files;
    }
  }
  ASSERT_FALSE(card.empty());
  // The next commit is itself cut at its first write: the pending generation
  // has already been made active, so it still loads.
  for (const std::string& next : {issue("dogear-2026-11-23", "a corrected"), issue("dogear-2026-11-30", "b")}) {
    for (int budget = 0; budget < 40; ++budget) {
      Fixture g;
      g.fs.files = card;
      g.fs.budget = budget;
      const bool committed =
          g.commit(next.find("11-30") != std::string::npos ? "dogear-2026-11-30" : "dogear-2026-11-23", next);
      g.fs.powerBack();
      const std::string loaded = g.best();
      ASSERT_TRUE(loaded == a || loaded == next) << "budget " << budget << " loaded " << loaded;
      if (committed) EXPECT_EQ(loaded, next);
    }
  }
}

// Two interrupted commits in a row, at every pair of cut points, from an empty
// card and from one with an active issue. Once any generation has loaded after
// a reboot, a later interruption never leaves nothing to load.
TEST(DogearCache, RepeatedInterruptionsNeverLoseTheLastLoadedGeneration) {
  const std::string old = issue("dogear-2026-11-16", "old"), a = issue("dogear-2026-11-23", "a"),
                    aFix = issue("dogear-2026-11-23", "a corrected"), b = issue("dogear-2026-11-30", "b");
  struct Step {
    std::string id, bytes;
  };
  const std::vector<std::pair<Step, Step>> sequences = {
      {{"dogear-2026-11-23", a}, {"dogear-2026-11-23", aFix}},  // first install, then a correction
      {{"dogear-2026-11-23", a}, {"dogear-2026-11-30", b}},     // first install, then a new week
  };
  for (const bool startWithActive : {false, true}) {
    for (const auto& seq : sequences) {
      for (int cut1 = 0; cut1 < 40; ++cut1) {
        Fixture first;
        if (startWithActive) ASSERT_TRUE(first.commit("dogear-2026-11-16", old));
        first.fs.budget = cut1;
        first.commit(seq.first.id, seq.first.bytes);
        first.fs.powerBack();
        const std::string afterFirst = first.best();
        for (int cut2 = 0; cut2 < 40; ++cut2) {
          Fixture second;
          second.fs.files = first.fs.files;
          second.fs.budget = cut2;
          const bool committed = second.commit(seq.second.id, seq.second.bytes);
          second.fs.powerBack();
          const std::string loaded = second.best();
          if (committed) {
            EXPECT_EQ(loaded, seq.second.bytes);
          } else if (afterFirst != "<none>") {
            ASSERT_TRUE(loaded == afterFirst || loaded == seq.second.bytes)
                << "active " << startWithActive << " cuts " << cut1 << "/" << cut2 << " loaded " << loaded;
          }
          // And the card still accepts a clean update afterwards.
          const std::string next = issue("dogear-2027-01-04", "next");
          ASSERT_TRUE(second.commit("dogear-2027-01-04", next));
          EXPECT_EQ(second.best(), next);
        }
      }
    }
  }
}

TEST(DogearCache, PendingGenerationIsLeftInPlaceIfItCannotBePromoted) {
  const std::string a = issue("dogear-2026-11-23", "a");
  Fixture f;
  ASSERT_TRUE(f.commit("dogear-2026-11-23", a));
  // Recreate the interrupted state: the verified manifest only under the temp name.
  f.fs.files[dogear_cache::kActiveTmp] = f.fs.files[dogear_cache::kActive];
  f.fs.files.erase(dogear_cache::kActive);
  f.fs.budget = 0;  // the promotion's rename is the first operation; it fails
  EXPECT_FALSE(f.commit("dogear-2026-11-30", issue("dogear-2026-11-30", "b")));
  f.fs.powerBack();
  EXPECT_TRUE(f.fs.exists(dogear_cache::kActiveTmp));
  EXPECT_EQ(f.best(), a);
}

TEST(DogearCache, PreviousGenerationIsKeptAndUsedWhenActiveIsDamaged) {
  Fixture f;
  const std::string a = issue("dogear-2026-11-23", "a"), b = issue("dogear-2026-11-30", "b");
  ASSERT_TRUE(f.commit("dogear-2026-11-23", a));
  ASSERT_TRUE(f.commit("dogear-2026-11-30", b));
  Generation prev;
  ASSERT_TRUE(f.cache.loadFrom(dogear_cache::kPrevious, prev));
  EXPECT_EQ(prev.bytes, a);

  const std::string bPath = dogear_cache::issuePath("dogear-2026-11-30", testSha256Hex(b));
  // A modified issue that is still valid JSON is not accepted as the active one.
  f.fs.files[bPath] = issue("dogear-2026-11-30", "tampered");
  bool fromPrevious = false;
  Generation g;
  ASSERT_TRUE(f.cache.loadBest(g, &fromPrevious));
  EXPECT_TRUE(fromPrevious);
  EXPECT_EQ(g.bytes, a);
  // Truncated.
  f.fs.files[bPath] = b.substr(0, b.size() - 3);
  EXPECT_FALSE(f.cache.loadFrom(dogear_cache::kActive, g));
  // Oversized: rejected by size before it is read or hashed.
  f.fs.files[bPath] = std::string(dogear_limits::kMaxIssueBytes + 1, ' ');
  EXPECT_FALSE(f.cache.loadFrom(dogear_cache::kActive, g));
  // Missing.
  f.fs.files.erase(bPath);
  EXPECT_FALSE(f.cache.loadFrom(dogear_cache::kActive, g));
  EXPECT_EQ(f.best(), a);
}

TEST(DogearCache, ManifestMustBeWellFormedAndMatchTheFile) {
  Fixture f;
  const std::string a = issue("dogear-2026-11-23", "a");
  ASSERT_TRUE(f.commit("dogear-2026-11-23", a));
  Generation g;
  const std::string good = f.fs.files["/dogear/active.txt"];
  for (const std::string& bad :
       {std::string("v1 dogear-2026-11-23 ") + std::string(64, '0') + "\n",  // wrong hash
        std::string("v1 dogear-2026-11-30 ") + testSha256Hex(a) + "\n",      // wrong issue id (no such file)
        std::string("v1 ../etc/passwd ") + testSha256Hex(a) + "\n", good.substr(0, good.size() - 1),  // no newline
        std::string("v2 ") + good.substr(3), std::string(200, 'x')}) {
    f.fs.files["/dogear/active.txt"] = bad;
    EXPECT_FALSE(f.cache.loadFrom(dogear_cache::kActive, g)) << bad;
  }
  f.fs.files["/dogear/active.txt"] = good;
  EXPECT_TRUE(f.cache.loadFrom(dogear_cache::kActive, g));
}

TEST(DogearCache, CleanupKeepsOnlyReferencedFilesAndOldLayout) {
  Fixture f;
  f.fs.files["/dogear/current.json"] = "{}";  // the first prototype's layout
  f.fs.files["/dogear/issues/dogear-2026-11-23.json"] = "{}";
  const std::string a = issue("dogear-2026-11-23", "a"), b = issue("dogear-2026-11-30", "b"),
                    c = issue("dogear-2026-12-07", "c");
  ASSERT_TRUE(f.commit("dogear-2026-11-23", a));
  ASSERT_TRUE(f.commit("dogear-2026-11-30", b));
  ASSERT_TRUE(f.commit("dogear-2026-12-07", c));
  std::vector<std::string> names;
  f.fs.list("/dogear/issues", names);
  EXPECT_EQ(names.size(), 2u);  // active (c) and previous (b)
  EXPECT_FALSE(f.fs.exists("/dogear/current.json"));
  EXPECT_FALSE(f.fs.exists(dogear_cache::issuePath("dogear-2026-11-23", testSha256Hex(a))));
}

TEST(DogearCache, RecommittingTheActiveIssueIsANoOp) {
  Fixture f;
  const std::string a = issue("dogear-2026-11-23", "a"), b = issue("dogear-2026-11-30", "b");
  ASSERT_TRUE(f.commit("dogear-2026-11-23", a));
  ASSERT_TRUE(f.commit("dogear-2026-11-30", b));
  ASSERT_TRUE(f.commit("dogear-2026-11-30", b));
  Generation prev;
  ASSERT_TRUE(f.cache.loadFrom(dogear_cache::kPrevious, prev));
  EXPECT_EQ(prev.bytes, a);  // previous is not overwritten by the same issue
}

// A quiet week (schema 3) on firmware that reads only schema 2: the newest
// generation it can read is shown and nothing is deleted, so after an upgrade (or
// a rollback to the old build) the same SD card serves whatever that build reads.
TEST(DogearCache, OlderFirmwarePassesOverAnIssueSchemaItCannotRead) {
  Fixture f;
  const std::string regular = issue("dogear-2026-11-09", "regular");
  std::string quiet = issue("dogear-2026-11-16", "quiet");
  quiet.replace(quiet.find("\"schemaVersion\":2"), 17, "\"schemaVersion\":3");
  ASSERT_TRUE(f.commit("dogear-2026-11-09", regular));
  ASSERT_TRUE(f.commit("dogear-2026-11-16", quiet));
  const auto schema = [](const Generation& g, int v) {
    return g.bytes.find("\"schemaVersion\":" + std::to_string(v)) != std::string::npos;
  };
  const auto readsTwo = [&](const Generation& g) { return schema(g, 2); };
  const auto readsTwoAndThree = [&](const Generation& g) { return schema(g, 2) || schema(g, 3); };

  Generation g;
  EXPECT_STREQ(dogear_cache::loadNewestReadable(f.cache, g, readsTwo), dogear_cache::kPrevious);
  EXPECT_EQ(g.bytes, regular);
  EXPECT_STREQ(dogear_cache::loadNewestReadable(f.cache, g, readsTwoAndThree), dogear_cache::kActive);
  EXPECT_EQ(g.bytes, quiet);
  // Nothing was removed by the older build's pass: both generations still load.
  EXPECT_STREQ(dogear_cache::loadNewestReadable(f.cache, g, readsTwo), dogear_cache::kPrevious);

  // A quiet issue as the only generation: the older build has nothing it can read
  // (it then falls back to its compiled-in sample).
  Fixture fresh;
  ASSERT_TRUE(fresh.commit("dogear-2026-11-16", quiet));
  EXPECT_EQ(dogear_cache::loadNewestReadable(fresh.cache, g, readsTwo), nullptr);
}

}  // namespace
