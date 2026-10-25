// The production OnPointActivity with test collection tables (TestRegions.cpp) over the shipped
// catalogue: one collection (exactly Stage 1's list), and a one-route collection beside a
// two-page one.

#include "OnPointActivityHarness.h"

namespace {

using namespace harness;

class OnPointActivityConfigTest : public ActivityTest {
 protected:
  void enterList() {
    enter();
    if (inDepartureView()) send(Event::Released, Button::Back);
  }
};

#ifdef ON_POINT_TEST_ONE_COLLECTION

// Comparator order with BALAGTAS (0) pinned: 0, 1, 5, 3, 2, 6, 4.
constexpr size_t DISPLAY[] = {0, 1, 5, 3, 2, 6, 4};

TEST_F(OnPointActivityConfigTest, OneCollectionHasNoRowAndStage1Wrapping) {
  enterList();
  EXPECT_FALSE(collectionRowShown());
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "BALAGTAS");
  for (size_t position = 1; position < 7; ++position) {
    send(Event::Pressed, Button::NavNext);
    EXPECT_EQ(selectedRow(), static_cast<int>((position - 1) % 3)) << position;
    EXPECT_FALSE(collectionRowShown());
  }
  send(Event::Pressed, Button::NavNext);  // Stage 1: the last route wraps to position 1
  EXPECT_EQ(selectedRow(), 0);
  EXPECT_EQ(textAt(ROW_Y[0] + 8), on_point::scheduleForRoute(DISPLAY[1], false).origin);
  send(Event::Pressed, Button::NavPrevious);  // position 1 wraps to the last
  EXPECT_EQ(textAt(ROW_Y[2] + 8), on_point::scheduleForRoute(DISPLAY[6], false).origin);
  touch(Event::ScreenTap, 200, COLLECTION_ROW_Y + 20);  // no row: nothing there
  EXPECT_FALSE(inPicker());
  EXPECT_FALSE(inDepartureView());
}

TEST_F(OnPointActivityConfigTest, OneCollectionSwipePagesAsStage1) {
  enterList();
  swipe(SwipeDir::Up);
  EXPECT_EQ(textAt(ROW_Y[0] + 8), on_point::scheduleForRoute(DISPLAY[4], false).origin);
  swipe(SwipeDir::Up);  // no third page
  EXPECT_EQ(textAt(ROW_Y[0] + 8), on_point::scheduleForRoute(DISPLAY[4], false).origin);
  swipe(SwipeDir::Down);
  EXPECT_EQ(textAt(ROW_Y[0] + 8), on_point::scheduleForRoute(DISPLAY[1], false).origin);
}

#else

TEST_F(OnPointActivityConfigTest, OneRouteCollectionTogglesBetweenTheRouteAndTheRow) {
  strcpy(SETTINGS.onPointMainRouteId, "p2p-clark-airport-naia-t3");
  enterList();  // "one-route": CLARK -> NAIA T3 only
  EXPECT_TRUE(collectionRowShown());
  EXPECT_TRUE(drewAt("STR_ON_POINT_COLLECTION 1", COLLECTION_TEXT_Y));  // unnamed: numbered
  EXPECT_TRUE(pinnedSelected());
  send(Event::Pressed, Button::NavNext);
  EXPECT_TRUE(collectionRowSelected());
  send(Event::Pressed, Button::NavNext);
  EXPECT_TRUE(pinnedSelected());
  send(Event::Pressed, Button::NavPrevious);
  EXPECT_TRUE(collectionRowSelected());
  send(Event::Pressed, Button::NavPrevious);
  EXPECT_TRUE(pinnedSelected());
}

TEST_F(OnPointActivityConfigTest, SwitchingCollectionsFromPage2ResetsTheList) {
  enterList();                                // "rest": BALAGTAS pinned; 5 others over two pages
  send(Event::Pressed, Button::NavPrevious);  // the last route, on page 2
  EXPECT_EQ(selectedRow(), 1);
  EXPECT_TRUE(drew("2 / 2"));
  send(Event::Pressed, Button::NavNext);  // the last route -> the row
  ASSERT_TRUE(collectionRowSelected());
  EXPECT_TRUE(drew("2 / 2"));  // the page stays while the row has focus
  send(Event::Released, Button::Confirm);
  ASSERT_TRUE(inPicker());
  send(Event::Released, Button::Back);  // cancel: page and focus as they were
  EXPECT_TRUE(collectionRowSelected());
  EXPECT_TRUE(drew("2 / 2"));
  send(Event::Released, Button::Confirm);
  send(Event::Pressed, Button::NavPrevious);  // "one-route"
  send(Event::Released, Button::Confirm);
  EXPECT_FALSE(inPicker());
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "CLARK AIRPORT");
  EXPECT_TRUE(pinnedSelected());
  EXPECT_FALSE(drew("2 / 2"));
  EXPECT_EQ(selectedRow(), -1);
  EXPECT_FALSE(collectionRowSelected());
  touch(Event::ScreenTap, 200, COLLECTION_ROW_Y + 20);  // and back to "rest": page 1, pinned
  touch(Event::ScreenTap, 200, PICKER_ROW_Y[1] + 30);
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "BALAGTAS");
  EXPECT_TRUE(pinnedSelected());
  EXPECT_TRUE(drew("1 / 2"));
}

#endif

}  // namespace
