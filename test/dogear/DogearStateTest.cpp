// Which DOGEAR screen shows (DogearState.h), and the status screens' exact copy.
#include <gtest/gtest.h>

#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include "activities/dogear/DogearState.h"

namespace {
using dogear_state::choose;
using dogear_state::Manifest;
using dogear_state::Screen;

const Manifest kManifests[] = {Manifest::NotTried, Manifest::Ok, Manifest::NothingPublished, Manifest::Failed};

TEST(DogearState, EveryCombinationFollowsTheThreeRules) {
  int combinations = 0;
  for (const bool wifi : {false, true}) {
    for (const Manifest m : kManifests) {
      for (const bool issue : {false, true}) {
        for (const bool cache : {false, true}) {
          ++combinations;
          const Screen s = choose(wifi, m, issue, cache);
          if (issue) {
            EXPECT_EQ(s, Screen::Issue);  // rule 1
          } else if (cache) {
            EXPECT_EQ(s, Screen::Cached);  // rule 2, even when nothing is published
          } else if (!wifi) {
            EXPECT_EQ(s, Screen::NoIssueOnDevice);
          } else if (m == Manifest::NothingPublished) {
            EXPECT_EQ(s, Screen::NoIssueYet);
          } else {
            EXPECT_EQ(s, Screen::CantLoad);
          }
          // A status screen appears only when no readable issue exists anywhere.
          EXPECT_EQ(dogear_state::isStatus(s), !issue && !cache);
          if (s == Screen::NoIssueYet) EXPECT_EQ(m, Manifest::NothingPublished);
          if (s == Screen::NoIssueOnDevice) EXPECT_FALSE(wifi);
        }
      }
    }
  }
  EXPECT_EQ(combinations, 32);
}

TEST(DogearState, TheCasesThatMatter) {
  EXPECT_EQ(choose(true, Manifest::Ok, true, true), Screen::Issue);  // a current issue, older week or not
  EXPECT_EQ(choose(true, Manifest::NothingPublished, false, true), Screen::Cached);
  EXPECT_EQ(choose(true, Manifest::Failed, false, true), Screen::Cached);  // outage with a cache
  EXPECT_EQ(choose(false, Manifest::NotTried, false, true), Screen::Cached);
  EXPECT_EQ(choose(true, Manifest::NothingPublished, false, false), Screen::NoIssueYet);  // E1
  EXPECT_EQ(choose(false, Manifest::NotTried, false, false), Screen::NoIssueOnDevice);    // E2
  EXPECT_EQ(choose(true, Manifest::Failed, false, false), Screen::CantLoad);              // E3
  EXPECT_EQ(choose(true, Manifest::Ok, false, false), Screen::CantLoad);                  // manifest fine, issue failed
}

enum class FakeId {
  None,
  STR_DOGEAR_NO_ISSUE_YET,
  STR_DOGEAR_NO_ISSUE_YET_NOTE,
  STR_DOGEAR_NO_ISSUE_ON_DEVICE,
  STR_DOGEAR_NO_ISSUE_ON_DEVICE_NOTE,
  STR_DOGEAR_CANT_LOAD,
  STR_DOGEAR_CANT_LOAD_NOTE,
};

TEST(DogearState, EachStatusScreenHasItsOwnHeadingAndNote) {
  const struct {
    Screen screen;
    FakeId heading, note;
  } cases[] = {
      {Screen::NoIssueYet, FakeId::STR_DOGEAR_NO_ISSUE_YET, FakeId::STR_DOGEAR_NO_ISSUE_YET_NOTE},
      {Screen::NoIssueOnDevice, FakeId::STR_DOGEAR_NO_ISSUE_ON_DEVICE, FakeId::STR_DOGEAR_NO_ISSUE_ON_DEVICE_NOTE},
      {Screen::CantLoad, FakeId::STR_DOGEAR_CANT_LOAD, FakeId::STR_DOGEAR_CANT_LOAD_NOTE},
  };
  for (const auto& c : cases) {
    FakeId h = FakeId::None, n = FakeId::None;
    EXPECT_TRUE(dogear_state::statusCopy(c.screen, h, n));
    EXPECT_EQ(h, c.heading);
    EXPECT_EQ(n, c.note);
  }
  for (const Screen s : {Screen::Issue, Screen::Cached}) {
    FakeId h = FakeId::None, n = FakeId::None;
    EXPECT_FALSE(dogear_state::statusCopy(s, h, n));
    EXPECT_EQ(h, FakeId::None);
  }
}

// The owner-approved copy, byte for byte (curly apostrophes, U+2019).
TEST(DogearState, StatusCopyIsExactlyTheApprovedCopy) {
  std::ifstream f(std::string(DOGEAR_REPO_ROOT) + "/lib/I18n/translations/english.yaml", std::ios::binary);
  ASSERT_TRUE(f.good());
  std::map<std::string, std::string> strings;
  std::string line;
  while (std::getline(f, line)) {
    const size_t colon = line.find(": \"");
    if (colon == std::string::npos || line.back() != '"') continue;
    strings[line.substr(0, colon)] = line.substr(colon + 3, line.size() - colon - 4);
  }
  const std::map<std::string, std::string> approved = {
      {"STR_DOGEAR", "DOGEAR"},
      {"STR_DOGEAR_STRAPLINE", "a limited weekly reading digest"},
      {"STR_DOGEAR_NO_ISSUE_YET", "NO ISSUE YET"},
      {"STR_DOGEAR_NO_ISSUE_YET_NOTE", "DOGEAR hasn’t published an issue yet. Check back soon."},
      {"STR_DOGEAR_NO_ISSUE_ON_DEVICE", "NO ISSUE ON THIS DEVICE"},
      {"STR_DOGEAR_NO_ISSUE_ON_DEVICE_NOTE", "Connect to Wi-Fi to check for an issue."},
      {"STR_DOGEAR_CANT_LOAD", "CAN’T LOAD AN ISSUE"},
      {"STR_DOGEAR_CANT_LOAD_NOTE", "DOGEAR couldn’t load an issue. Try again later, or check for a firmware update."},
  };
  for (const auto& kv : approved) EXPECT_EQ(strings[kv.first], kv.second) << kv.first;
  EXPECT_EQ(strings.count("STR_DOGEAR_NO_ISSUE"), 0u) << "the old one-line message is gone";
}

}  // namespace
