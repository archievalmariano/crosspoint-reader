#include <gtest/gtest.h>

#include <cstdint>

#include "activities/dogear/DogearLimits.h"
#include "activities/dogear/DogearPages.h"
#include "activities/dogear/DogearText.h"

namespace {

using dogear_pages::Kind;
using dogear_pages::Screen;

// The 23-29 Nov sample: six one-screen items -> cover + 6 + final = 8 pages.
TEST(DogearPages, OneScreenItemsNumberLikeAPrintedIssue) {
  const uint8_t parts[] = {1, 1, 1, 1, 1, 1};
  Screen s[dogear_pages::kMaxScreens];
  const int n = dogear_pages::build(parts, 6, s, dogear_pages::kMaxScreens);
  ASSERT_EQ(n, 8);
  EXPECT_EQ(s[0].kind, Kind::Cover);
  EXPECT_EQ(s[0].number, 1);
  for (int i = 1; i <= 6; ++i) {
    EXPECT_EQ(s[i].kind, Kind::Item);
    EXPECT_EQ(s[i].entry, i - 1);
    EXPECT_EQ(s[i].number, i + 1);
    EXPECT_TRUE(dogear_pages::showsAction(s[i]));
  }
  EXPECT_EQ(s[7].kind, Kind::Final);
  EXPECT_EQ(s[7].number, 8);
  EXPECT_EQ(dogear_pages::pageCount(6), 8);
}

// A two-screen item shares its page number; only its second screen carries the action.
TEST(DogearPages, SecondScreenSharesThePageNumber) {
  const uint8_t parts[] = {1, 2, 1};
  Screen s[dogear_pages::kMaxScreens];
  const int n = dogear_pages::build(parts, 3, s, dogear_pages::kMaxScreens);
  ASSERT_EQ(n, 6);  // cover, item 1, item 2 (x2), item 3, final
  EXPECT_EQ(s[2].number, 3);
  EXPECT_EQ(s[3].number, 3);
  EXPECT_EQ(s[3].part, 1);
  EXPECT_FALSE(dogear_pages::showsAction(s[2]));
  EXPECT_TRUE(dogear_pages::showsAction(s[3]));
  EXPECT_EQ(s[5].number, dogear_pages::pageCount(3));  // final is page 5 of 5 on 6 screens
}

TEST(DogearPages, PartsAreClampedToTwo) {
  const uint8_t parts[] = {0, 7};
  Screen s[dogear_pages::kMaxScreens];
  EXPECT_EQ(dogear_pages::build(parts, 2, s, dogear_pages::kMaxScreens), 5);
}

TEST(DogearPages, RefusesWhatDoesNotFit) {
  const uint8_t parts[] = {2, 2, 2};
  Screen s[4];
  EXPECT_EQ(dogear_pages::build(parts, 3, s, 4), 0);
}

TEST(DogearPages, NavigationIsCircular) {
  EXPECT_EQ(dogear_pages::nextIndex(7, 8), 0);  // final -> cover
  EXPECT_EQ(dogear_pages::prevIndex(0, 8), 7);  // cover -> final
  EXPECT_EQ(dogear_pages::nextIndex(3, 8), 4);
  EXPECT_EQ(dogear_pages::nextIndex(0, 0), 0);
}

TEST(DogearPages, CoverAndFinalCarryNoItemAction) {
  const uint8_t parts[] = {1};
  Screen s[dogear_pages::kMaxScreens];
  ASSERT_EQ(dogear_pages::build(parts, 1, s, dogear_pages::kMaxScreens), 3);
  EXPECT_FALSE(dogear_pages::showsAction(s[0]));
  EXPECT_FALSE(dogear_pages::showsAction(s[2]));
}

TEST(DogearPages, PressGateIgnoresAReleaseItDidNotSeePressed) {
  dogear_pages::PressGate gate;
  EXPECT_FALSE(gate.update(false, true));  // the Wi-Fi picker's press
  EXPECT_FALSE(gate.update(true, false));
  EXPECT_TRUE(gate.update(false, true));
  EXPECT_TRUE(gate.update(true, true));  // touch back gesture: one frame
}

TEST(DogearPages, BackTapRectCoversTheLabelToTheBottomEdge) {
  // margin 24, label 60 wide, footer rule at 740 on an 800-high screen
  const dogear_pages::TapRect r = dogear_pages::backTapRect(24, 60, 740, 800);
  EXPECT_EQ(r.x, 0);
  EXPECT_EQ(r.y, 740);
  EXPECT_EQ(r.x + r.w, 24 + 60 + 24);  // past the label's end, by a fingertip
  EXPECT_EQ(r.y + r.h, 800);           // to the bottom edge
}

TEST(DogearLimits, IssueIdsAreDatedAndPathSafe) {
  EXPECT_TRUE(dogear_limits::isSafeIssueId("dogear-2026-11-23"));
  EXPECT_TRUE(dogear_limits::isSafeIssueId("dogear-2026-12-28"));
  EXPECT_FALSE(dogear_limits::isSafeIssueId("dogear-2026-11-2"));
  EXPECT_FALSE(dogear_limits::isSafeIssueId("dogear-2026/11-23"));
  EXPECT_FALSE(dogear_limits::isSafeIssueId("dogear-../../x.js"));
  EXPECT_FALSE(dogear_limits::isSafeIssueId("goto-2026-11-23xxx"));
  EXPECT_FALSE(dogear_limits::isSafeIssueId("dogear-2026-11-23.json"));
  EXPECT_FALSE(dogear_limits::isSafeIssueId(""));
}

TEST(DogearLimits, QrUrlsAreHttpWithinCapacity) {
  EXPECT_TRUE(dogear_limits::isQrUrl("https://example.org/ebooks/12345"));
  EXPECT_TRUE(dogear_limits::isQrUrl("http://192.168.1.50:8080/2026-11-23/"));
  EXPECT_FALSE(dogear_limits::isQrUrl(""));
  EXPECT_FALSE(dogear_limits::isQrUrl("javascript:alert(1)"));
  EXPECT_FALSE(dogear_limits::isQrUrl("https://" + std::string(3000, 'a')));
}

// ---- cover titles: word-boundary ellipsis (the approved mock's rule) ----------

// A line "fits" when it has at most `n` bytes counting the ellipsis as one column.
auto fitsColumns(size_t n) {
  return [n](const std::string& s) {
    size_t cols = 0;
    for (size_t i = 0; i < s.size(); ++i)
      if ((static_cast<unsigned char>(s[i]) & 0xC0) != 0x80) ++cols;
    return cols <= n;
  };
}

TEST(DogearText, CutsAtAWordWithTheEllipsisRightAfterIt) {
  using dogear_text::ellipsizeAtWord;
  // As the X4 Pro wrapped "The Collected Letters of a Staging Writer, Volume Two" (lead, 2 lines).
  auto lead = ellipsizeAtWord({"The Collected Letters of a", "Staging Writer, Volume", "Two"}, 2, fitsColumns(26));
  ASSERT_EQ(lead.size(), 2u);
  EXPECT_EQ(lead[0], "The Collected Letters of a");
  EXPECT_EQ(lead[1], "Staging Writer, Volume\xE2\x80\xA6");  // not "Volume ...", not "Volume …"
  // One-line rows: never mid-word ("Title Th…", "Titl…"), never a space before the ellipsis.
  EXPECT_EQ(ellipsizeAtWord({"A Very Long Staging Work Title That", "Wraps Onto Two Lines"}, 1, fitsColumns(33))[0],
            "A Very Long Staging Work Title\xE2\x80\xA6");
  EXPECT_EQ(ellipsizeAtWord({"Another Staging Work With a Title", "Long Enough to Wrap"}, 1, fitsColumns(30))[0],
            "Another Staging Work With a\xE2\x80\xA6");
  EXPECT_EQ(ellipsizeAtWord({"Yet Another Long Staging Title for", "the Contents List"}, 1, fitsColumns(34))[0],
            "Yet Another Long Staging Title\xE2\x80\xA6");
  // Punctuation stays with its word, as in the mock ("a Staging Writer,…").
  EXPECT_EQ(ellipsizeAtWord({"a Staging Writer, Volume", "Two"}, 1, fitsColumns(18))[0],
            "a Staging Writer,\xE2\x80\xA6");
}

TEST(DogearText, LeavesTitlesWithinTheLimitAlone) {
  using dogear_text::ellipsizeAtWord;
  const std::vector<std::string> two = {"The Collected Letters of", "a Staging Writer"};
  EXPECT_EQ(ellipsizeAtWord(two, 2, fitsColumns(1)), two);  // no cut, whatever the width
  EXPECT_EQ(ellipsizeAtWord({"Staging Writer Oct 8"}, 1, fitsColumns(1)).at(0), "Staging Writer Oct 8");
  EXPECT_TRUE(ellipsizeAtWord({"a", "b"}, 0, fitsColumns(9)).empty());
}

TEST(DogearText, OnlyAWordWiderThanTheLineIsCutByCharactersUtf8Safely) {
  using dogear_text::ellipsizeAtWord;
  auto cut = ellipsizeAtWord({"Pneumonoultramicroscopic", "x"}, 1, fitsColumns(6));
  EXPECT_EQ(cut[0], "Pneum\xE2\x80\xA6");
  auto accented = ellipsizeAtWord({"\xC3\x89\xC3\x89\xC3\x89\xC3\x89", "x"}, 1, fitsColumns(3));  // "ÉÉÉÉ"
  EXPECT_EQ(accented[0], "\xC3\x89\xC3\x89\xE2\x80\xA6");                                         // whole "É"s kept
}

}  // namespace
