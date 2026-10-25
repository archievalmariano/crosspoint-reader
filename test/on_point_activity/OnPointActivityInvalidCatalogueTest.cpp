// The production OnPointActivity over a catalogue whose timetables are all invalid
// (InvalidCatalogue.cpp): direct launch shows TIMETABLE UNAVAILABLE, below CLOCK NOT SET.

#include "OnPointActivityHarness.h"

namespace {

using namespace harness;

class OnPointActivityInvalidCatalogueTest : public ActivityTest {};

TEST_F(OnPointActivityInvalidCatalogueTest, LaunchShowsTimetableUnavailable) {
  enter();
  EXPECT_TRUE(inDepartureView());
  EXPECT_TRUE(drew("STR_ON_POINT_TIMETABLE_UNAVAILABLE"));
  EXPECT_FALSE(drew("STR_ON_POINT_NOT_SET"));
}

TEST_F(OnPointActivityInvalidCatalogueTest, ClockNotSetTakesPrecedence) {
  clockReady = false;
  enter();
  EXPECT_TRUE(inDepartureView());
  EXPECT_TRUE(drew("STR_ON_POINT_NOT_SET"));
  EXPECT_FALSE(drew("STR_ON_POINT_TIMETABLE_UNAVAILABLE"));
}

TEST_F(OnPointActivityInvalidCatalogueTest, TheListStillListsTheRoutes) {
  enter();
  send(Event::Released, Button::Back);
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "BALAGTAS");
  EXPECT_EQ(textAt(ROW_Y[0] + 8), "CAYPOMBO");
  EXPECT_TRUE(collectionRowShown());
}

}  // namespace
