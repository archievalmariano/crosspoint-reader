// OnPointStore (Stage 2, P2): /.crosspoint/onpoint.json persistence, recovery, schema handling,
// migration from the legacy main route and rollback reconciliation, over the shipped collections.

#include <ArduinoJson.h>
#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "FakeStorage.h"
#include "activities/on_point/OnPointStore.h"

namespace {

using on_point::ON_POINT_STORE_BAK_PATH;
using on_point::ON_POINT_STORE_PATH;
using on_point::ON_POINT_STORE_TMP_PATH;
using on_point::OnPointStore;
using Source = OnPointStore::Source;
using Outcome = OnPointStore::SaveOutcome;

constexpr size_t BULACAN = 0, NORTH_LUZON = 1, AIRPORTS = 2;
constexpr char BALAGTAS[] = "p2p-balagtas-trinoma";
constexpr char CAYPOMBO[] = "p2p-caypombo-sm-north-edsa";
constexpr char CUBAO_CLARK[] = "p2p-cubao-clark-airport";
constexpr char TRINOMA_CLARK[] = "p2p-trinoma-clark-airport";
constexpr char CLARK_NAIA[] = "p2p-clark-airport-naia-t3";
constexpr char NAIA_PITX[] = "p2p-naia-pitx";

// SETTINGS.onPointMainRouteId is char[48]; the store only ever reads a copy like this.
struct Legacy {
  char value[48] = {};
  explicit Legacy(const char* v) { strncpy(value, v, sizeof(value) - 1); }
};

void load(OnPointStore& store, const Legacy& legacy) { store.load(legacy.value, sizeof(legacy.value)); }

std::string v1(const char* active, const char* defaults, const char* baseline = nullptr, const char* extra = "") {
  std::string s = std::string("{\"version\":1,\"activeRegion\":\"") + active + "\",\"defaults\":{" + defaults + "}";
  if (baseline) s += std::string(",\"legacyBaseline\":\"") + baseline + "\"";
  return s + extra + "}";
}

JsonDocument parse(const std::string& text) {
  JsonDocument doc;
  EXPECT_FALSE(deserializeJson(doc, text));
  return doc;
}

struct Snapshot {
  std::string active, bulacan, north, airports;
  bool operator==(const Snapshot& o) const {
    return active == o.active && bulacan == o.bulacan && north == o.north && airports == o.airports;
  }
};
Snapshot snapshot(const OnPointStore& s) {
  return {on_point::regionAt(s.activeRegionIndex())->regionId, s.defaultRouteId(BULACAN), s.defaultRouteId(NORTH_LUZON),
          s.defaultRouteId(AIRPORTS)};
}

// ---- Migration (no valid file) ----

TEST(OnPointStore, MigratesTheDefaultLegacyRoute) {
  FakeStorage fs;
  OnPointStore store(fs);
  load(store, Legacy(BALAGTAS));
  EXPECT_EQ(store.source(), Source::Migrated);
  EXPECT_EQ(store.activeRegionIndex(), BULACAN);
  EXPECT_STREQ(store.defaultRouteId(BULACAN), BALAGTAS);
  EXPECT_STREQ(store.defaultRouteId(NORTH_LUZON), CUBAO_CLARK);  // shipped defaults
  EXPECT_STREQ(store.defaultRouteId(AIRPORTS), CLARK_NAIA);
  EXPECT_FALSE(store.readOnly());
  EXPECT_FALSE(store.hasUnsavedChanges());
  EXPECT_EQ(fs.mutations, 0);
}

TEST(OnPointStore, MigratesAnotherLegacyRouteIntoItsCollection) {
  FakeStorage fs;
  OnPointStore store(fs);
  load(store, Legacy(TRINOMA_CLARK));
  EXPECT_EQ(store.activeRegionIndex(), NORTH_LUZON);
  EXPECT_STREQ(store.defaultRouteId(NORTH_LUZON), TRINOMA_CLARK);
  EXPECT_STREQ(store.defaultRouteId(BULACAN), BALAGTAS);
  EXPECT_STREQ(store.defaultRouteId(AIRPORTS), CLARK_NAIA);
}

TEST(OnPointStore, EmptyOrUnknownLegacyGivesTheNoMainStateAndKeepsTheValue) {
  for (const char* value : {"", "p2p-retired-route", "P2P-BALAGTAS-TRINOMA"}) {
    FakeStorage fs;
    OnPointStore store(fs);
    load(store, Legacy(value));
    EXPECT_EQ(store.activeRegionIndex(), BULACAN) << value;
    EXPECT_STREQ(store.defaultRouteId(BULACAN), value);  // resolves to Unset/Unknown: no main route
    EXPECT_STREQ(store.defaultRouteId(NORTH_LUZON), CUBAO_CLARK);
    EXPECT_EQ(fs.mutations, 0);
  }
}

TEST(OnPointStore, UnterminatedLegacyBufferIsNeverReadPast) {
  char legacy[48];
  memset(legacy, 'x', sizeof(legacy));  // no NUL
  FakeStorage fs;
  OnPointStore store(fs);
  store.load(legacy, sizeof(legacy));
  EXPECT_EQ(store.activeRegionIndex(), BULACAN);
  EXPECT_STREQ(store.defaultRouteId(BULACAN), "");
  store.load(nullptr, 0);
  EXPECT_EQ(store.source(), Source::Migrated);
}

// ---- Round trip, stable IDs, no writes except user changes ----

TEST(OnPointStore, RoundTripUsesStableIds) {
  FakeStorage fs;
  {
    OnPointStore store(fs);
    load(store, Legacy(BALAGTAS));
    EXPECT_EQ(store.setDefaultRoute(BULACAN, CAYPOMBO), Outcome::Saved);
    EXPECT_EQ(store.setActiveRegion(AIRPORTS), Outcome::Saved);
    EXPECT_EQ(store.setDefaultRoute(AIRPORTS, NAIA_PITX), Outcome::Saved);
    EXPECT_FALSE(store.hasUnsavedChanges());
  }
  ASSERT_TRUE(fs.exists(ON_POINT_STORE_PATH));
  EXPECT_FALSE(fs.exists(ON_POINT_STORE_TMP_PATH));
  const JsonDocument doc = parse(fs.files[ON_POINT_STORE_PATH]);
  EXPECT_EQ(doc["version"].as<int>(), 1);
  EXPECT_STREQ(doc["activeRegion"].as<const char*>(), "airport-transfers");
  EXPECT_STREQ(doc["defaults"]["bulacan"].as<const char*>(), CAYPOMBO);
  EXPECT_STREQ(doc["defaults"]["airport-transfers"].as<const char*>(), NAIA_PITX);
  EXPECT_STREQ(doc["legacyBaseline"].as<const char*>(), BALAGTAS);
  EXPECT_TRUE(doc["defaults"]["0"].isNull());  // never a catalogue or table position

  OnPointStore reloaded(fs);
  const int before = fs.mutations;
  load(reloaded, Legacy(BALAGTAS));
  EXPECT_EQ(reloaded.source(), Source::Live);
  EXPECT_EQ(reloaded.activeRegionIndex(), AIRPORTS);
  EXPECT_STREQ(reloaded.defaultRouteId(BULACAN), CAYPOMBO);
  EXPECT_STREQ(reloaded.defaultRouteId(NORTH_LUZON), CUBAO_CLARK);
  EXPECT_STREQ(reloaded.defaultRouteId(AIRPORTS), NAIA_PITX);
  EXPECT_EQ(fs.mutations, before);  // loading never writes
}

TEST(OnPointStore, SecondSaveKeepsTheFirstAsTheBackup) {
  FakeStorage fs;
  OnPointStore store(fs);
  load(store, Legacy(BALAGTAS));
  ASSERT_EQ(store.setDefaultRoute(BULACAN, CAYPOMBO), Outcome::Saved);
  const std::string first = fs.files[ON_POINT_STORE_PATH];
  ASSERT_EQ(store.setActiveRegion(NORTH_LUZON), Outcome::Saved);
  EXPECT_EQ(fs.files[ON_POINT_STORE_BAK_PATH], first);
  EXPECT_FALSE(fs.exists(ON_POINT_STORE_TMP_PATH));
}

TEST(OnPointStore, InvalidRequestsChangeNothing) {
  FakeStorage fs;
  OnPointStore store(fs);
  load(store, Legacy(BALAGTAS));
  std::string tooLong(48, 'r');
  EXPECT_EQ(store.setActiveRegion(3), Outcome::Failed);
  EXPECT_EQ(store.setActiveRegion(on_point::NO_REGION), Outcome::Failed);
  EXPECT_EQ(store.setDefaultRoute(7, CAYPOMBO), Outcome::Failed);
  EXPECT_EQ(store.setDefaultRoute(BULACAN, nullptr), Outcome::Failed);
  EXPECT_EQ(store.setDefaultRoute(BULACAN, tooLong.c_str()), Outcome::Failed);
  EXPECT_STREQ(store.defaultRouteId(BULACAN), BALAGTAS);
  EXPECT_EQ(store.defaultRouteId(3), nullptr);
  EXPECT_EQ(fs.mutations, 0);
  EXPECT_FALSE(store.hasUnsavedChanges());
}

// ---- Recovery order and damaged files ----

TEST(OnPointStore, RecoveryOrder) {
  const std::string live = v1("north-luzon", "\"north-luzon\":\"p2p-trinoma-clark-airport\"", BALAGTAS);
  const std::string tmp = v1("airport-transfers", "", BALAGTAS);
  const std::string bak = v1("bulacan", "\"bulacan\":\"p2p-caypombo-sm-north-edsa\"", BALAGTAS);
  struct Case {
    const char* name;
    const char* live;
    const char* tmp;
    const char* bak;
    Source source;
    size_t active;
  } cases[] = {
      {"live wins", live.c_str(), tmp.c_str(), bak.c_str(), Source::Live, NORTH_LUZON},
      {"no live: tmp", nullptr, tmp.c_str(), bak.c_str(), Source::Temporary, AIRPORTS},
      {"no live, bad tmp: backup", nullptr, "{\"version\":1,", bak.c_str(), Source::Backup, BULACAN},
      {"corrupt live: tmp before backup", "garbage", tmp.c_str(), bak.c_str(), Source::Temporary, AIRPORTS},
      {"corrupt live, bad tmp: backup", "garbage", "{", bak.c_str(), Source::Backup, BULACAN},
      {"nothing valid: migrate", "{}", "[1]", "{\"version\":0}", Source::Migrated, BULACAN},
  };
  for (const Case& c : cases) {
    FakeStorage fs;
    if (c.live) fs.files[ON_POINT_STORE_PATH] = c.live;
    if (c.tmp) fs.files[ON_POINT_STORE_TMP_PATH] = c.tmp;
    if (c.bak) fs.files[ON_POINT_STORE_BAK_PATH] = c.bak;
    OnPointStore store(fs);
    load(store, Legacy(BALAGTAS));
    EXPECT_EQ(store.source(), c.source) << c.name;
    EXPECT_EQ(store.activeRegionIndex(), c.active) << c.name;
    EXPECT_FALSE(store.readOnly()) << c.name;
    EXPECT_EQ(fs.mutations, 0) << c.name;
  }
}

TEST(OnPointStore, MalformedFilesAreInvalid) {
  for (const char* bad :
       {"", "null", "[]", "\"x\"", "{\"activeRegion\":\"bulacan\"}", "{\"version\":\"1\"}", "{\"version\":1.5}",
        "{\"version\":-1}", "{\"version\":true}", "{\"version\":1", "{\"version\":1,\"defaults\":{\"bulacan\":"}) {
    FakeStorage fs;
    fs.files[ON_POINT_STORE_PATH] = bad;
    OnPointStore store(fs);
    load(store, Legacy(TRINOMA_CLARK));
    EXPECT_EQ(store.source(), Source::Migrated) << bad;
    EXPECT_EQ(store.activeRegionIndex(), NORTH_LUZON) << bad;
  }
}

TEST(OnPointStore, WrongTypedOrOversizeFieldsAreIgnored) {
  FakeStorage fs;
  fs.files[ON_POINT_STORE_PATH] = "{\"version\":1,\"activeRegion\":7,\"defaults\":{\"bulacan\":3,\"north-luzon\":\"" +
                                  std::string(60, 'n') +
                                  "\",\"airport-transfers\":\"p2p-naia-pitx\"},\"legacyBaseline\":[\"x\"]}";
  OnPointStore store(fs);
  load(store, Legacy(BALAGTAS));
  EXPECT_EQ(store.source(), Source::Live);
  EXPECT_FALSE(store.activeRegionKnown());
  EXPECT_EQ(store.activeRegionIndex(), BULACAN);
  EXPECT_STREQ(store.defaultRouteId(BULACAN), BALAGTAS);         // shipped
  EXPECT_STREQ(store.defaultRouteId(NORTH_LUZON), CUBAO_CLARK);  // too long: shipped
  EXPECT_STREQ(store.defaultRouteId(AIRPORTS), NAIA_PITX);
}

TEST(OnPointStore, UnknownActiveRegionIsShownAsTheFirstButKept) {
  FakeStorage fs;
  fs.files[ON_POINT_STORE_PATH] = v1("rizal", "\"rizal\":\"p2p-rizal-route\"", BALAGTAS);
  OnPointStore store(fs);
  load(store, Legacy(BALAGTAS));
  EXPECT_FALSE(store.activeRegionKnown());
  EXPECT_EQ(store.activeRegionIndex(), BULACAN);
  ASSERT_EQ(store.setDefaultRoute(BULACAN, CAYPOMBO), Outcome::Saved);  // not a collection change
  const JsonDocument doc = parse(fs.files[ON_POINT_STORE_PATH]);
  EXPECT_STREQ(doc["activeRegion"].as<const char*>(), "rizal");
  EXPECT_STREQ(doc["defaults"]["rizal"].as<const char*>(), "p2p-rizal-route");
}

// ---- Fault injection: every mutation of a save ----

// A save from state S0 to S1 interrupted at mutation k (failure or power loss, with or without a
// torn write) leaves a card from which the next load yields S0 or S1, never a migration.
TEST(OnPointStore, InterruptedSavesAlwaysLeaveAValidCopy) {
  for (const bool powerLoss : {false, true}) {
    for (const bool torn : {false, true}) {
      for (int k = 0; k < 8; ++k) {
        FakeStorage fs;
        Snapshot s0, s1;
        {
          OnPointStore setup(fs);
          load(setup, Legacy(BALAGTAS));
          ASSERT_EQ(setup.setDefaultRoute(BULACAN, CAYPOMBO), Outcome::Saved);  // S0 on the card
          ASSERT_EQ(setup.setActiveRegion(NORTH_LUZON), Outcome::Saved);        // S0 has a backup too
          s0 = snapshot(setup);
        }
        OnPointStore store(fs);
        load(store, Legacy(BALAGTAS));
        const int start = fs.mutations;
        (powerLoss ? fs.powerLossAt : fs.failAt) = start + k;
        fs.tornWrites = torn;
        const Outcome outcome = store.setDefaultRoute(NORTH_LUZON, TRINOMA_CLARK);
        s1 = snapshot(store);
        fs.clearFaults();
        if (outcome != Outcome::Saved) EXPECT_TRUE(store.hasUnsavedChanges());

        OnPointStore after(fs);
        load(after, Legacy(BALAGTAS));
        const std::string context = std::string(powerLoss ? "power loss" : "failure") + (torn ? ", torn" : "") +
                                    " at mutation " + std::to_string(k);
        EXPECT_NE(after.source(), Source::Migrated) << context;
        const Snapshot got = snapshot(after);
        EXPECT_TRUE(got == s0 || got == s1) << context;
        if (outcome == Outcome::Saved) EXPECT_TRUE(got == s1) << context;
      }
    }
  }
}

TEST(OnPointStore, InterruptedFirstSaveLeavesTheCardLoadable) {
  for (int k = 0; k < 4; ++k) {
    FakeStorage fs;
    OnPointStore store(fs);
    load(store, Legacy(BALAGTAS));
    fs.powerLossAt = k;
    fs.tornWrites = true;
    store.setDefaultRoute(BULACAN, CAYPOMBO);
    fs.clearFaults();
    OnPointStore after(fs);
    load(after, Legacy(BALAGTAS));
    const std::string bulacan = after.defaultRouteId(BULACAN);
    EXPECT_TRUE(bulacan == BALAGTAS || bulacan == CAYPOMBO) << k;  // migrated again, or saved
  }
}

TEST(OnPointStore, FailedSaveIsSessionOnlyUntilASaveSucceeds) {
  FakeStorage fs;
  OnPointStore store(fs);
  load(store, Legacy(BALAGTAS));
  fs.failAt = 0;  // the tmp write
  EXPECT_EQ(store.setDefaultRoute(BULACAN, CAYPOMBO), Outcome::Failed);
  EXPECT_TRUE(store.hasUnsavedChanges());
  EXPECT_STREQ(store.defaultRouteId(BULACAN), CAYPOMBO);  // kept for the session
  fs.clearFaults();
  EXPECT_EQ(store.setActiveRegion(BULACAN), Outcome::Saved);  // everything in memory is now saved
  EXPECT_FALSE(store.hasUnsavedChanges());
  OnPointStore after(fs);
  load(after, Legacy(BALAGTAS));
  EXPECT_STREQ(after.defaultRouteId(BULACAN), CAYPOMBO);
}

TEST(OnPointStore, ReadBackMismatchAbortsBeforeTouchingTheLiveFile) {
  struct CorruptingStorage : FakeStorage {
    bool write(const char* path, const char* data, const size_t length) override {
      std::string copy(data, length);
      const size_t at = copy.find("caypombo");
      if (std::string(path) == ON_POINT_STORE_TMP_PATH && at != std::string::npos) copy[at] = 'k';
      return FakeStorage::write(path, copy.data(), copy.size());
    }
  } fs;
  OnPointStore setup(fs);
  load(setup, Legacy(BALAGTAS));
  ASSERT_EQ(setup.setActiveRegion(BULACAN), Outcome::Saved);
  const std::string live = fs.files[ON_POINT_STORE_PATH];
  EXPECT_EQ(setup.setDefaultRoute(BULACAN, CAYPOMBO), Outcome::Failed);
  EXPECT_EQ(fs.files[ON_POINT_STORE_PATH], live);
  EXPECT_FALSE(fs.exists(ON_POINT_STORE_TMP_PATH));
}

// ---- Codex P2 findings (regressions) ----

// Finding 1: with no live file, a recovered tmp is the only copy. A save interrupted in any way
// must never leave the card without it (or without the new state).
TEST(OnPointStore, Finding1_RecoveredTmpSurvivesEveryInterruptedSave) {
  enum class Fault { WriteFails, ShortWrite, CloseFails, PowerLoss };
  for (const Fault fault : {Fault::WriteFails, Fault::ShortWrite, Fault::CloseFails, Fault::PowerLoss}) {
    for (int k = 0; k < 6; ++k) {
      FakeStorage fs;
      fs.files[ON_POINT_STORE_TMP_PATH] = v1("north-luzon", "\"north-luzon\":\"p2p-trinoma-clark-airport\"", BALAGTAS);
      OnPointStore store(fs);
      load(store, Legacy(BALAGTAS));
      ASSERT_EQ(store.source(), Source::Temporary);
      const Snapshot s0 = snapshot(store);
      (fault == Fault::PowerLoss ? fs.powerLossAt : fs.failAt) = k;
      fs.tornWrites = fault == Fault::ShortWrite || fault == Fault::PowerLoss;
      fs.closeFailures = fault == Fault::CloseFails;
      const Outcome outcome = store.setDefaultRoute(BULACAN, CAYPOMBO);
      const Snapshot s1 = snapshot(store);
      fs.clearFaults();

      OnPointStore after(fs);
      load(after, Legacy(BALAGTAS));
      const std::string context =
          "fault " + std::to_string(static_cast<int>(fault)) + " at mutation " + std::to_string(k);
      EXPECT_NE(after.source(), Source::Migrated) << context;
      const Snapshot got = snapshot(after);
      EXPECT_TRUE(got == s0 || got == s1) << context;
      if (outcome == Outcome::Saved) EXPECT_TRUE(got == s1) << context;
    }
  }
}

// Finding 2: a backup that can't be positively identified is never deleted or replaced.
TEST(OnPointStore, Finding2_UnreadableOrOversizeBackupIsProtected) {
  for (const bool oversize : {false, true}) {
    FakeStorage fs;
    fs.files[ON_POINT_STORE_PATH] = v1("bulacan", "", BALAGTAS);
    const std::string backup = oversize ? std::string(5000, '{') : v1("north-luzon", "", BALAGTAS);
    fs.files[ON_POINT_STORE_BAK_PATH] = backup;
    if (!oversize) fs.unreadable.insert(ON_POINT_STORE_BAK_PATH);
    OnPointStore store(fs);
    load(store, Legacy(BALAGTAS));
    EXPECT_NE(store.setDefaultRoute(BULACAN, CAYPOMBO), Outcome::Saved) << oversize;
    EXPECT_TRUE(store.hasUnsavedChanges());
    EXPECT_EQ(fs.files[ON_POINT_STORE_BAK_PATH], backup) << oversize;
    EXPECT_EQ(fs.files[ON_POINT_STORE_PATH], v1("bulacan", "", BALAGTAS)) << oversize;
  }
}

TEST(OnPointStore, Finding2_UnrecognisedBackupIsKeptWhileSaving) {
  FakeStorage fs;
  fs.files[ON_POINT_STORE_PATH] = v1("bulacan", "", BALAGTAS);
  fs.files[ON_POINT_STORE_BAK_PATH] = "not json";  // unrecognised: never deleted or replaced
  OnPointStore store(fs);
  load(store, Legacy(BALAGTAS));
  EXPECT_EQ(store.setDefaultRoute(BULACAN, CAYPOMBO), Outcome::Saved);
  EXPECT_EQ(fs.files[ON_POINT_STORE_BAK_PATH], "not json");
  EXPECT_STREQ(parse(fs.files[ON_POINT_STORE_PATH])["defaults"]["bulacan"].as<const char*>(), CAYPOMBO);
}

// Finding 3: an existing tmp file is inspected before the writer truncates it.
TEST(OnPointStore, Finding3_NewerOrUnreadableTmpIsNeverTruncated) {
  struct Case {
    const char* name;
    std::string tmp;
    bool unreadable;
    Outcome outcome;
  } cases[] = {
      {"newer", "{\"version\":2,\"activeRegion\":\"bulacan\"}", false, Outcome::ReadOnly},
      {"oversize", std::string(5000, 'x'), false, Outcome::Failed},
      {"unreadable", v1("bulacan", "", BALAGTAS), true, Outcome::Failed},
  };
  for (const Case& c : cases) {
    FakeStorage fs;
    fs.files[ON_POINT_STORE_PATH] = v1("bulacan", "", BALAGTAS);
    fs.files[ON_POINT_STORE_TMP_PATH] = c.tmp;
    if (c.unreadable) fs.unreadable.insert(ON_POINT_STORE_TMP_PATH);
    OnPointStore store(fs);
    load(store, Legacy(BALAGTAS));
    EXPECT_EQ(store.setActiveRegion(NORTH_LUZON), c.outcome) << c.name;
    EXPECT_EQ(fs.files[ON_POINT_STORE_TMP_PATH], c.tmp) << c.name;
    EXPECT_EQ(fs.files[ON_POINT_STORE_PATH], v1("bulacan", "", BALAGTAS)) << c.name;
    EXPECT_TRUE(store.hasUnsavedChanges()) << c.name;
  }
}

// Finding 4: every schema-1 field a newer file carries is used, not only activeRegion.
TEST(OnPointStore, Finding4_NewerFileFieldsAreUsedWithoutActiveRegion) {
  FakeStorage fs;
  fs.files[ON_POINT_STORE_PATH] =
      "{\"version\":2,\"defaults\":{\"north-luzon\":\"p2p-trinoma-clark-airport\",\"airport-transfers\":"
      "\"p2p-naia-pitx\"},\"legacyBaseline\":\"p2p-clark-airport-naia-t3\"}";
  OnPointStore store(fs);
  load(store, Legacy(CLARK_NAIA));
  EXPECT_TRUE(store.readOnly());
  EXPECT_STREQ(store.defaultRouteId(NORTH_LUZON), TRINOMA_CLARK);
  EXPECT_STREQ(store.defaultRouteId(AIRPORTS), NAIA_PITX);
  EXPECT_EQ(store.activeRegionIndex(), AIRPORTS);  // no activeRegion: the legacy route's collection, not the first
  EXPECT_STREQ(store.defaultRouteId(BULACAN), BALAGTAS);
  EXPECT_FALSE(store.reconciledLegacyRoute());  // the baseline it carries matches
  EXPECT_EQ(fs.mutations, 0);
}

TEST(OnPointStore, UnreadableIsNotMissing) {
  // A live file that can't be read is never treated as absent: read-only, values from the backup.
  FakeStorage fs;
  fs.files[ON_POINT_STORE_PATH] = v1("bulacan", "", BALAGTAS);
  fs.unreadable.insert(ON_POINT_STORE_PATH);
  fs.files[ON_POINT_STORE_BAK_PATH] = v1("north-luzon", "", BALAGTAS);
  OnPointStore store(fs);
  load(store, Legacy(BALAGTAS));
  EXPECT_TRUE(store.readOnly());
  EXPECT_EQ(store.source(), Source::Backup);
  EXPECT_EQ(store.activeRegionIndex(), NORTH_LUZON);
  EXPECT_EQ(store.setActiveRegion(AIRPORTS), Outcome::ReadOnly);
  EXPECT_EQ(fs.mutations, 0);

  // No live file and an unreadable tmp: the backup is used, and saves are refused untouched.
  FakeStorage fs2;
  fs2.files[ON_POINT_STORE_TMP_PATH] = v1("airport-transfers", "", BALAGTAS);
  fs2.unreadable.insert(ON_POINT_STORE_TMP_PATH);
  fs2.files[ON_POINT_STORE_BAK_PATH] = v1("north-luzon", "", BALAGTAS);
  OnPointStore store2(fs2);
  load(store2, Legacy(BALAGTAS));
  EXPECT_EQ(store2.source(), Source::Backup);
  EXPECT_FALSE(store2.readOnly());
  EXPECT_EQ(store2.setActiveRegion(AIRPORTS), Outcome::Failed);
  EXPECT_TRUE(store2.hasUnsavedChanges());
  EXPECT_EQ(fs2.mutations, 0);
  EXPECT_EQ(fs2.files.size(), 2u);
}

// Every starting state a save may proceed from, every fault, every mutation: the next load
// yields the old or the new state, never a migration.
TEST(OnPointStore, InterruptedSavesFromEveryRecoverableState) {
  const std::string s0 = v1("north-luzon", "\"north-luzon\":\"p2p-trinoma-clark-airport\"", BALAGTAS);
  struct Start {
    const char* name;
    const char* live;
    const char* tmp;
    const char* bak;
  } starts[] = {
      {"live + current backup", s0.c_str(), nullptr, s0.c_str()},
      {"live + invalid backup", s0.c_str(), nullptr, "corrupt"},
      {"live + invalid tmp", s0.c_str(), "{", nullptr},
      {"invalid live + backup", "corrupt", nullptr, s0.c_str()},
      {"invalid live + recovered tmp", "corrupt", s0.c_str(), nullptr},
      {"recovered tmp only", nullptr, s0.c_str(), nullptr},
      {"backup only", nullptr, nullptr, s0.c_str()},
      {"backup + invalid tmp", nullptr, "{", s0.c_str()},
  };
  enum class Fault { Fails, Short, CloseFails, PowerLoss, TornPowerLoss };
  for (const Start& start : starts) {
    for (const Fault fault : {Fault::Fails, Fault::Short, Fault::CloseFails, Fault::PowerLoss, Fault::TornPowerLoss}) {
      for (int k = 0; k < 7; ++k) {
        FakeStorage fs;
        if (start.live) fs.files[ON_POINT_STORE_PATH] = start.live;
        if (start.tmp) fs.files[ON_POINT_STORE_TMP_PATH] = start.tmp;
        if (start.bak) fs.files[ON_POINT_STORE_BAK_PATH] = start.bak;
        OnPointStore store(fs);
        load(store, Legacy(BALAGTAS));
        ASSERT_NE(store.source(), Source::Migrated) << start.name;
        const Snapshot before = snapshot(store);
        const bool lose = fault == Fault::PowerLoss || fault == Fault::TornPowerLoss;
        (lose ? fs.powerLossAt : fs.failAt) = k;
        fs.tornWrites = fault == Fault::Short || fault == Fault::TornPowerLoss;
        fs.closeFailures = fault == Fault::CloseFails;
        const Outcome outcome = store.setDefaultRoute(BULACAN, CAYPOMBO);
        const Snapshot after = snapshot(store);
        fs.clearFaults();
        OnPointStore reloaded(fs);
        load(reloaded, Legacy(BALAGTAS));
        const std::string context = std::string(start.name) + ", fault " + std::to_string(static_cast<int>(fault)) +
                                    " at mutation " + std::to_string(k);
        EXPECT_NE(reloaded.source(), Source::Migrated) << context;
        const Snapshot got = snapshot(reloaded);
        EXPECT_TRUE(got == before || got == after) << context;
        if (outcome == Outcome::Saved) EXPECT_TRUE(got == after) << context;
        if (start.bak && std::string(start.bak) == "corrupt") EXPECT_EQ(fs.files[ON_POINT_STORE_BAK_PATH], "corrupt");
      }
    }
  }
}

// ---- Newer schema and oversize files: read-only ----

TEST(OnPointStore, NewerSchemaIsReadOnlyAndNeverTouched) {
  FakeStorage fs;
  const std::string newer =
      "{\"version\":2,\"activeRegion\":\"north-luzon\",\"defaults\":{\"north-luzon\":\"p2p-trinoma-clark-airport\"},"
      "\"legacyBaseline\":\"p2p-balagtas-trinoma\",\"profiles\":[1,2]}";
  fs.files[ON_POINT_STORE_PATH] = newer;
  OnPointStore store(fs);
  load(store, Legacy(BALAGTAS));
  EXPECT_TRUE(store.readOnly());
  EXPECT_EQ(store.activeRegionIndex(), NORTH_LUZON);  // schema-1 fields still read
  EXPECT_STREQ(store.defaultRouteId(NORTH_LUZON), TRINOMA_CLARK);
  EXPECT_FALSE(store.hasUnsavedChanges());
  EXPECT_EQ(store.setActiveRegion(AIRPORTS), Outcome::ReadOnly);
  EXPECT_EQ(store.setDefaultRoute(BULACAN, CAYPOMBO), Outcome::ReadOnly);
  EXPECT_TRUE(store.hasUnsavedChanges());          // shown as NOT SAVED in P3
  EXPECT_EQ(store.activeRegionIndex(), AIRPORTS);  // the session still follows the user
  EXPECT_EQ(fs.mutations, 0);
  EXPECT_EQ(fs.files[ON_POINT_STORE_PATH], newer);
  EXPECT_EQ(fs.files.size(), 1u);
}

TEST(OnPointStore, NewerSchemaWithoutSchema1FieldsMigratesInMemory) {
  FakeStorage fs;
  fs.files[ON_POINT_STORE_PATH] = "{\"version\":3,\"collections\":{\"current\":\"north-luzon\"}}";
  OnPointStore store(fs);
  load(store, Legacy(TRINOMA_CLARK));
  EXPECT_TRUE(store.readOnly());
  EXPECT_EQ(store.activeRegionIndex(), NORTH_LUZON);
  EXPECT_STREQ(store.defaultRouteId(NORTH_LUZON), TRINOMA_CLARK);
  EXPECT_EQ(store.setActiveRegion(BULACAN), Outcome::ReadOnly);
  EXPECT_EQ(fs.mutations, 0);
}

TEST(OnPointStore, NewerBackupIsNeverReplaced) {
  FakeStorage fs;
  fs.files[ON_POINT_STORE_PATH] = v1("bulacan", "", BALAGTAS);
  const std::string newerBackup = "{\"version\":2,\"activeRegion\":\"bulacan\"}";
  fs.files[ON_POINT_STORE_BAK_PATH] = newerBackup;
  OnPointStore store(fs);
  load(store, Legacy(BALAGTAS));
  EXPECT_EQ(store.setActiveRegion(NORTH_LUZON), Outcome::ReadOnly);
  EXPECT_EQ(fs.files[ON_POINT_STORE_BAK_PATH], newerBackup);
  EXPECT_EQ(fs.files[ON_POINT_STORE_PATH], v1("bulacan", "", BALAGTAS));
  EXPECT_FALSE(fs.exists(ON_POINT_STORE_TMP_PATH));
}

TEST(OnPointStore, NewerRecoveredCopyKeepsTheStoreReadOnly) {
  // No live file, and the copy recovery picks is newer: nothing may be written beside it.
  for (const char* path : {ON_POINT_STORE_TMP_PATH, ON_POINT_STORE_BAK_PATH}) {
    FakeStorage fs;
    fs.files[path] = "{\"version\":2,\"activeRegion\":\"north-luzon\"}";
    OnPointStore store(fs);
    load(store, Legacy(BALAGTAS));
    EXPECT_TRUE(store.readOnly()) << path;
    EXPECT_EQ(store.setActiveRegion(BULACAN), Outcome::ReadOnly) << path;
    EXPECT_EQ(fs.mutations, 0) << path;
    EXPECT_FALSE(fs.exists(ON_POINT_STORE_PATH)) << path;
  }
}

TEST(OnPointStore, OversizeFilesAreReadOnly) {
  FakeStorage fs;
  const std::string big = v1("north-luzon", "", BALAGTAS, (",\"pad\":\"" + std::string(5000, 'p') + "\"").c_str());
  fs.files[ON_POINT_STORE_PATH] = big;
  OnPointStore store(fs);
  load(store, Legacy(TRINOMA_CLARK));
  EXPECT_TRUE(store.readOnly());
  EXPECT_EQ(store.source(), Source::Migrated);  // can't be read, so the legacy route is used
  EXPECT_EQ(store.setDefaultRoute(BULACAN, CAYPOMBO), Outcome::ReadOnly);
  EXPECT_EQ(fs.files[ON_POINT_STORE_PATH], big);
  EXPECT_EQ(fs.mutations, 0);
}

TEST(OnPointStore, LiveFileReplacedByANewerOneBeforeSavingIsNotOverwritten) {
  FakeStorage fs;
  OnPointStore store(fs);
  load(store, Legacy(BALAGTAS));
  fs.files[ON_POINT_STORE_PATH] = "{\"version\":2}";  // written by newer firmware meanwhile
  EXPECT_EQ(store.setActiveRegion(AIRPORTS), Outcome::ReadOnly);
  EXPECT_EQ(fs.files[ON_POINT_STORE_PATH], "{\"version\":2}");
  EXPECT_TRUE(store.readOnly());
}

// ---- Unknown content: kept within the bounds ----

TEST(OnPointStore, UnknownKeysAndFutureCollectionsAreKept) {
  FakeStorage fs;
  fs.files[ON_POINT_STORE_PATH] = v1("bulacan", "\"bulacan\":\"p2p-balagtas-trinoma\",\"laguna\":\"p2p-laguna-route\"",
                                     BALAGTAS, ",\"future\":{\"theme\":\"dark\",\"n\":[1,2,3]}");
  OnPointStore store(fs);
  load(store, Legacy(BALAGTAS));
  ASSERT_EQ(store.setDefaultRoute(BULACAN, CAYPOMBO), Outcome::Saved);
  const JsonDocument doc = parse(fs.files[ON_POINT_STORE_PATH]);
  EXPECT_STREQ(doc["defaults"]["laguna"].as<const char*>(), "p2p-laguna-route");
  EXPECT_STREQ(doc["future"]["theme"].as<const char*>(), "dark");
  EXPECT_EQ(doc["future"]["n"][2].as<int>(), 3);
  EXPECT_STREQ(doc["defaults"]["bulacan"].as<const char*>(), CAYPOMBO);
}

TEST(OnPointStore, DefaultsBeyondTheEntryBoundDropUnknownOnesFirst) {
  std::string entries;
  for (int i = 0; i < 20; ++i)
    entries += "\"future-" + std::to_string(i) + "\":\"p2p-route-" + std::to_string(i) + "\",";
  FakeStorage fs;
  fs.files[ON_POINT_STORE_PATH] =
      v1("bulacan", (entries + "\"north-luzon\":\"p2p-cubao-clark-airport\"").c_str(), BALAGTAS);
  OnPointStore store(fs);
  load(store, Legacy(BALAGTAS));
  ASSERT_EQ(store.setDefaultRoute(BULACAN, CAYPOMBO), Outcome::Saved);
  const JsonDocument doc = parse(fs.files[ON_POINT_STORE_PATH]);
  EXPECT_EQ(doc["defaults"].as<JsonObjectConst>().size(), 16u);
  EXPECT_STREQ(doc["defaults"]["bulacan"].as<const char*>(), CAYPOMBO);
  EXPECT_STREQ(doc["defaults"]["north-luzon"].as<const char*>(), CUBAO_CLARK);
  EXPECT_FALSE(doc["defaults"]["future-0"].isNull());  // the earliest unknown ones are kept
}

TEST(OnPointStore, MergedContentOverTheSizeBoundKeepsOnlyKnownFields) {
  FakeStorage fs;
  // Under 4096 bytes as stored; over it once a longer default is merged in.
  const std::string pad(4096 - 120, 'p');
  fs.files[ON_POINT_STORE_PATH] = v1("bulacan", "", BALAGTAS, (",\"pad\":\"" + pad + "\"").c_str());
  ASSERT_LT(fs.files[ON_POINT_STORE_PATH].size(), on_point::ON_POINT_STORE_MAX_BYTES);
  OnPointStore store(fs);
  load(store, Legacy(BALAGTAS));
  ASSERT_FALSE(store.readOnly());
  ASSERT_EQ(store.setDefaultRoute(NORTH_LUZON, TRINOMA_CLARK), Outcome::Saved);
  ASSERT_EQ(store.setDefaultRoute(AIRPORTS, NAIA_PITX), Outcome::Saved);
  const JsonDocument doc = parse(fs.files[ON_POINT_STORE_PATH]);
  EXPECT_TRUE(doc["pad"].isNull());
  EXPECT_STREQ(doc["defaults"]["north-luzon"].as<const char*>(), TRINOMA_CLARK);
  EXPECT_LT(fs.files[ON_POINT_STORE_PATH].size(), on_point::ON_POINT_STORE_MAX_BYTES);
}

// ---- Rollback to v1.2.x and re-upgrade ----

// The card after Stage 2 saved: north-luzon default changed, baseline = the legacy value then.
FakeStorage afterStage2Save() {
  FakeStorage fs;
  OnPointStore store(fs);
  load(store, Legacy(BALAGTAS));
  EXPECT_EQ(store.setDefaultRoute(NORTH_LUZON, TRINOMA_CLARK), Outcome::Saved);
  EXPECT_EQ(store.setDefaultRoute(AIRPORTS, NAIA_PITX), Outcome::Saved);
  fs.mutations = 0;
  return fs;
}

TEST(OnPointStore, UnchangedLegacyRouteMeansNoReconciliation) {
  FakeStorage fs = afterStage2Save();
  OnPointStore store(fs);
  load(store, Legacy(BALAGTAS));
  EXPECT_FALSE(store.reconciledLegacyRoute());
  EXPECT_STREQ(store.defaultRouteId(NORTH_LUZON), TRINOMA_CLARK);
  EXPECT_EQ(fs.mutations, 0);
}

TEST(OnPointStore, LegacyRouteChangedUnderOlderFirmwareIsReconciledWithoutAWrite) {
  FakeStorage fs = afterStage2Save();
  const std::string card = fs.files[ON_POINT_STORE_PATH];
  // Under v1.2.x the user held CLARK -> NAIA T3 as main: only settings.json changed.
  OnPointStore store(fs);
  load(store, Legacy(CLARK_NAIA));
  EXPECT_TRUE(store.reconciledLegacyRoute());
  EXPECT_EQ(store.activeRegionIndex(), AIRPORTS);
  EXPECT_STREQ(store.defaultRouteId(AIRPORTS), CLARK_NAIA);
  EXPECT_STREQ(store.defaultRouteId(NORTH_LUZON), TRINOMA_CLARK);  // unrelated defaults untouched
  EXPECT_STREQ(store.defaultRouteId(BULACAN), BALAGTAS);
  EXPECT_FALSE(store.hasUnsavedChanges());  // not a user change
  EXPECT_EQ(fs.mutations, 0);
  EXPECT_EQ(fs.files[ON_POINT_STORE_PATH], card);

  // Each launch reapplies it until the next user change records the new baseline.
  OnPointStore again(fs);
  load(again, Legacy(CLARK_NAIA));
  EXPECT_TRUE(again.reconciledLegacyRoute());
  ASSERT_EQ(again.setActiveRegion(BULACAN), Outcome::Saved);
  EXPECT_STREQ(parse(fs.files[ON_POINT_STORE_PATH])["legacyBaseline"].as<const char*>(), CLARK_NAIA);
  OnPointStore later(fs);
  load(later, Legacy(CLARK_NAIA));
  EXPECT_FALSE(later.reconciledLegacyRoute());
  EXPECT_EQ(later.activeRegionIndex(), BULACAN);
  EXPECT_STREQ(later.defaultRouteId(AIRPORTS), CLARK_NAIA);
}

TEST(OnPointStore, AmbiguousLegacyChangesAreIgnored) {
  for (const char* value : {"", "p2p-retired-route"}) {
    FakeStorage fs = afterStage2Save();
    OnPointStore store(fs);
    load(store, Legacy(value));
    EXPECT_FALSE(store.reconciledLegacyRoute()) << value;
    EXPECT_EQ(store.activeRegionIndex(), BULACAN) << value;
    EXPECT_STREQ(store.defaultRouteId(NORTH_LUZON), TRINOMA_CLARK);
    EXPECT_EQ(fs.mutations, 0);
  }
  // No baseline in the file: the file wins.
  FakeStorage fs;
  fs.files[ON_POINT_STORE_PATH] = v1("north-luzon", "\"north-luzon\":\"p2p-trinoma-clark-airport\"");
  OnPointStore store(fs);
  load(store, Legacy(CLARK_NAIA));
  EXPECT_FALSE(store.reconciledLegacyRoute());
  EXPECT_EQ(store.activeRegionIndex(), NORTH_LUZON);
}

TEST(OnPointStore, ReconciliationInReadOnlyModeStaysInMemory) {
  FakeStorage fs;
  const std::string newer =
      "{\"version\":2,\"activeRegion\":\"bulacan\",\"defaults\":{},\"legacyBaseline\":\"p2p-balagtas-trinoma\"}";
  fs.files[ON_POINT_STORE_PATH] = newer;
  OnPointStore store(fs);
  load(store, Legacy(NAIA_PITX));
  EXPECT_TRUE(store.readOnly());
  EXPECT_TRUE(store.reconciledLegacyRoute());
  EXPECT_EQ(store.activeRegionIndex(), AIRPORTS);
  EXPECT_STREQ(store.defaultRouteId(AIRPORTS), NAIA_PITX);
  EXPECT_EQ(store.setActiveRegion(BULACAN), Outcome::ReadOnly);
  EXPECT_EQ(fs.files[ON_POINT_STORE_PATH], newer);
}

TEST(OnPointStore, LegacyBufferIsNeverWritten) {
  Legacy legacy(TRINOMA_CLARK);
  const Legacy before = legacy;
  FakeStorage fs;
  OnPointStore store(fs);
  load(store, legacy);
  store.setDefaultRoute(NORTH_LUZON, CUBAO_CLARK);
  store.setActiveRegion(AIRPORTS);
  load(store, legacy);
  EXPECT_EQ(memcmp(legacy.value, before.value, sizeof(legacy.value)), 0);
}

}  // namespace
