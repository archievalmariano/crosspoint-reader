#include <gtest/gtest.h>

#include "activities/goto/GotoNav.h"

namespace {

// A 10-story GOTO edition: story indices 0..9, terminal edition page at index 10.
TEST(GotoNav, TerminalPageIsOutsideStoryPagination) {
  const int stories = 10;
  EXPECT_EQ(goto_nav::terminalIndex(stories), 10);
  EXPECT_EQ(goto_nav::ringSize(stories), 11);  // 10 stories + 1 terminal page
  // The terminal index is NOT one of the story pages (0..9 -> pager 1/10..10/10).
  for (int i = 0; i < stories; ++i) EXPECT_FALSE(goto_nav::isTerminal(i, stories)) << i;
  EXPECT_TRUE(goto_nav::isTerminal(10, stories));
}

TEST(GotoNav, LastStoryNextGoesToTerminal) {
  const int stories = 10;
  EXPECT_EQ(goto_nav::nextIndex(9, stories), 10);  // last story -> terminal
  EXPECT_TRUE(goto_nav::isTerminal(goto_nav::nextIndex(9, stories), stories));
}

TEST(GotoNav, TerminalPreviousGoesToLastStory) {
  const int stories = 10;
  EXPECT_EQ(goto_nav::prevIndex(10, stories), 9);  // terminal -> last story
}

TEST(GotoNav, TerminalNextWrapsToFirstStory) {
  const int stories = 10;
  EXPECT_EQ(goto_nav::nextIndex(10, stories), 0);  // terminal -> story 1
}

TEST(GotoNav, FirstStoryPreviousGoesToTerminal_CircularConsistency) {
  const int stories = 10;
  EXPECT_EQ(goto_nav::prevIndex(0, stories), 10);  // circular: story 1 <- terminal
}

TEST(GotoNav, SixStoryTogoTransitions) {
  const int stories = 6;  // a 6-story TOGO edition
  EXPECT_EQ(goto_nav::terminalIndex(stories), 6);
  EXPECT_EQ(goto_nav::nextIndex(5, stories), 6);   // final article 6/6 -> terminal
  EXPECT_EQ(goto_nav::prevIndex(6, stories), 5);   // terminal -> article 6
  EXPECT_EQ(goto_nav::nextIndex(6, stories), 0);   // terminal -> story 1
}

TEST(GotoNav, FullForwardCycleReturnsToStart) {
  const int stories = 3;  // 0,1,2 stories + terminal(3)
  int i = 0;
  for (int step = 0; step < goto_nav::ringSize(stories); ++step) i = goto_nav::nextIndex(i, stories);
  EXPECT_EQ(i, 0);  // one full lap around the ring returns to story 1
}

}  // namespace
