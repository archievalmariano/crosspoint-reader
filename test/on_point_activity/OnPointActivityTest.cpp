#include <gtest/gtest.h>

#include <cstring>
#include <memory>
#include <string>

#include "CrossPointSettings.h"
#include "activities/on_point/OnPointActivity.h"
#include "activities/on_point/OnPointScheduleData.h"

// Integration tests for the production OnPointActivity (entry, selection,
// persistence, input, no-main state) over the shipped catalogue, with the
// host stand-ins in test/on_point_activity/stubs for renderer, input,
// settings and translations. tr(STR_X) draws "STR_X".

ActivityManager activityManager;
bool RenderLock::peekResult = false;

namespace {

unsigned long nowMs = 0;
bool clockReady = true;
on_point::CivilDateTime clockUtc{2026, 10, 21, 0, 30, 0};  // Wed 08:30 Manila

}  // namespace

unsigned long millis() { return nowMs; }

bool on_point::HalClockSource::nowUtc(CivilDateTime& value) const {
  if (!clockReady) return false;
  value = clockUtc;
  return true;
}

namespace {

using Button = MappedInputManager::Button;
using Event = MappedInputManager::Event;
using SwipeDir = MappedInputManager::SwipeDir;

// v1.2.0 geometry (OnPointActivity.cpp / OnPointRouteList.h).
constexpr int HEADING_Y = 76;
constexpr int PINNED_ORIGIN_Y = 106;
constexpr int FOOTER_Y = 744;
constexpr int ROUTE_ORIGIN_Y = 72;
constexpr int ROW_Y[] = {316, 442, 568};
// Comparator order of the shipped routes, and the display order with
// Balagtas (index 0) pinned.
constexpr size_t DISPLAY_MAIN_0[] = {0, 1, 5, 3, 2, 6, 4};

const char* origin(size_t route) { return on_point::scheduleForRoute(route, false).origin; }

class OnPointActivityTest : public ::testing::Test {
 protected:
  GfxRenderer renderer;
  MappedInputManager input;
  std::unique_ptr<OnPointActivity> activity;

  void SetUp() override {
    SETTINGS.reset();
    activityManager = ActivityManager{};
    RenderLock::peekResult = false;
    clockReady = true;
    nowMs = 1000;
  }

  void enter() {
    activity = std::make_unique<OnPointActivity>(renderer, input);
    activity->onEnter();
    draw();
  }
  void draw() { activity->render(RenderLock{}); }
  void send(Event event, Button button = Button::Back) {
    input.event = event;
    input.button = button;
    activity->loop();
    input.clear();
    draw();
  }
  void touch(Event event, int x, int y) {
    input.event = event;
    input.x = x;
    input.y = y;
    activity->loop();
    input.clear();
    draw();
  }
  void swipe(SwipeDir dir) {
    input.event = Event::Swipe;
    input.swipe = dir;
    activity->loop();
    input.clear();
    draw();
  }

  std::string textAt(int y) const {
    for (const auto& t : renderer.texts)
      if (t.y == y) return t.text;
    return "";
  }
  bool drew(const std::string& text) const {
    for (const auto& t : renderer.texts)
      if (t.text == text) return true;
    return false;
  }
  // Selection bar widths: pinned card 10 (selected) / 4, list rows 8 / 3.
  bool pinnedSelected() const {
    for (const auto& r : renderer.rects)
      if (r.y == 102 && r.height == 150) return r.width == 10;
    return false;
  }
  int selectedRow() const {
    for (const auto& r : renderer.rects)
      for (int row = 0; row < 3; ++row)
        if (r.y == ROW_Y[row] && r.height == 104 && r.width == 8) return row;
    return -1;
  }
  bool inDepartureView() const { return !textAt(ROUTE_ORIGIN_Y).empty(); }
  std::string openRouteOrigin() {
    send(Event::Released, Button::Confirm);
    EXPECT_TRUE(inDepartureView());
    const std::string shown = textAt(ROUTE_ORIGIN_Y);
    send(Event::Released, Button::Back);
    EXPECT_FALSE(inDepartureView());
    return shown;
  }
};

TEST_F(OnPointActivityTest, EntryPinsAndSelectsTheSavedMainRoute) {
  enter();
  EXPECT_EQ(textAt(HEADING_Y), "STR_ON_POINT_MAIN_ROUTE");
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "BALAGTAS");
  EXPECT_TRUE(pinnedSelected());
  EXPECT_EQ(textAt(FOOTER_Y), "STR_ON_POINT_MAIN_ROUTE");
  EXPECT_EQ(textAt(ROW_Y[0] + 8), origin(DISPLAY_MAIN_0[1]));
  EXPECT_EQ(openRouteOrigin(), "BALAGTAS");
  EXPECT_EQ(SETTINGS.saves, 0);
}

TEST_F(OnPointActivityTest, EntryPinsAnotherSavedRoute) {
  strcpy(SETTINGS.onPointMainRouteId, "p2p-naia-pitx");
  enter();
  EXPECT_EQ(textAt(HEADING_Y), "STR_ON_POINT_MAIN_ROUTE");
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "NAIA T3");
  EXPECT_EQ(textAt(ROW_Y[0] + 8), "BALAGTAS");  // the rest stays in sort order
  EXPECT_EQ(openRouteOrigin(), "NAIA T3");
}

TEST_F(OnPointActivityTest, NavNextWalksTheListInOrderAndNeverReturnsToThePinnedSlot) {
  enter();
  for (size_t position = 1; position < 7; ++position) {
    send(Event::Pressed, Button::NavNext);
    EXPECT_FALSE(pinnedSelected());
    EXPECT_EQ(selectedRow(), static_cast<int>((position - 1) % 3));
    EXPECT_EQ(textAt(FOOTER_Y), "STR_ON_POINT_HOLD_SET_MAIN");
    EXPECT_EQ(openRouteOrigin(), origin(DISPLAY_MAIN_0[position])) << position;
  }
  send(Event::Pressed, Button::NavNext);  // wraps to position 1
  EXPECT_EQ(selectedRow(), 0);
  EXPECT_EQ(openRouteOrigin(), origin(DISPLAY_MAIN_0[1]));
}

TEST_F(OnPointActivityTest, NavPreviousFromThePinnedSlotGoesToTheLastRoute) {
  enter();
  send(Event::Pressed, Button::NavPrevious);
  EXPECT_EQ(openRouteOrigin(), origin(DISPLAY_MAIN_0[6]));
  EXPECT_EQ(textAt(ROW_Y[2] + 8), origin(DISPLAY_MAIN_0[6]));  // second page, third row
}

TEST_F(OnPointActivityTest, HoldingConfirmSavesTheSelectedRouteOnce) {
  enter();
  send(Event::Pressed, Button::NavNext);  // CAYPOMBO
  send(Event::LongPressed, Button::Confirm);
  EXPECT_STREQ(SETTINGS.onPointMainRouteId, "p2p-caypombo-sm-north-edsa");
  EXPECT_EQ(SETTINGS.saves, 1);
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "CAYPOMBO");
  EXPECT_TRUE(pinnedSelected());
  EXPECT_EQ(textAt(FOOTER_Y), "STR_ON_POINT_MAIN_ROUTE");
  send(Event::LongPressed, Button::Confirm);  // already the main route
  EXPECT_EQ(SETTINGS.saves, 1);
}

TEST_F(OnPointActivityTest, SavedMainRoutePersistsIntoTheNextEntry) {
  enter();
  send(Event::Pressed, Button::NavPrevious);
  send(Event::LongPressed, Button::Confirm);
  const std::string saved = SETTINGS.onPointMainRouteId;
  enter();  // a fresh activity reads the same settings
  EXPECT_EQ(SETTINGS.onPointMainRouteId, saved);
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), origin(DISPLAY_MAIN_0[6]));
  EXPECT_EQ(textAt(HEADING_Y), "STR_ON_POINT_MAIN_ROUTE");
}

TEST_F(OnPointActivityTest, UnknownSavedRouteEntersTheNoMainStateWithoutRewritingIt) {
  strcpy(SETTINGS.onPointMainRouteId, "p2p-retired-route");
  enter();
  EXPECT_EQ(textAt(HEADING_Y), "STR_ON_POINT_NO_MAIN_ROUTE");
  EXPECT_EQ(textAt(PINNED_ORIGIN_Y), "BALAGTAS");  // first in sort order
  EXPECT_TRUE(pinnedSelected());
  EXPECT_EQ(textAt(FOOTER_Y), "STR_ON_POINT_HOLD_SET_MAIN");
  for (int i = 0; i < 7; ++i) {
    send(Event::Pressed, Button::NavNext);
    EXPECT_EQ(textAt(FOOTER_Y), "STR_ON_POINT_HOLD_SET_MAIN");
  }
  send(Event::Released, Button::Back);  // leave ON POINT
  EXPECT_EQ(activityManager.goToAppsCalls, 1);
  EXPECT_STREQ(SETTINGS.onPointMainRouteId, "p2p-retired-route");
  EXPECT_EQ(SETTINGS.saves, 0);
}

TEST_F(OnPointActivityTest, HoldingThePinnedRouteInTheNoMainStateSavesIt) {
  strcpy(SETTINGS.onPointMainRouteId, "P2P-BALAGTAS-TRINOMA");  // case mismatch: Unknown
  enter();
  EXPECT_EQ(textAt(HEADING_Y), "STR_ON_POINT_NO_MAIN_ROUTE");
  send(Event::LongPressed, Button::Confirm);
  EXPECT_STREQ(SETTINGS.onPointMainRouteId, "p2p-balagtas-trinoma");
  EXPECT_EQ(SETTINGS.saves, 1);
  EXPECT_EQ(textAt(HEADING_Y), "STR_ON_POINT_MAIN_ROUTE");
  EXPECT_EQ(textAt(FOOTER_Y), "STR_ON_POINT_MAIN_ROUTE");
}

TEST_F(OnPointActivityTest, ExplicitlyEmptySavedRouteIsTheNoMainState) {
  SETTINGS.onPointMainRouteId[0] = '\0';
  enter();
  EXPECT_EQ(textAt(HEADING_Y), "STR_ON_POINT_NO_MAIN_ROUTE");
  EXPECT_EQ(SETTINGS.saves, 0);
}

TEST_F(OnPointActivityTest, TouchSelectsOpensAndSetsWithGapsIgnored) {
  enter();
  touch(Event::ScreenTouchDown, 200, 330);  // row 0
  EXPECT_EQ(selectedRow(), 0);
  EXPECT_FALSE(inDepartureView());
  touch(Event::ScreenTap, 200, 430);  // gap between rows 0 and 1
  EXPECT_FALSE(inDepartureView());
  touch(Event::ScreenTap, -5, 330);  // off screen
  EXPECT_FALSE(inDepartureView());
  touch(Event::ScreenLongPress, 200, 500);  // row 1
  EXPECT_STREQ(SETTINGS.onPointMainRouteId, on_point::routeAt(DISPLAY_MAIN_0[2]).routeId);
  EXPECT_EQ(SETTINGS.saves, 1);
  touch(Event::ScreenTap, 200, 150);  // pinned card opens the new main route
  EXPECT_TRUE(inDepartureView());
  EXPECT_EQ(textAt(ROUTE_ORIGIN_Y), origin(DISPLAY_MAIN_0[2]));
}

TEST_F(OnPointActivityTest, SwipePagesThroughTheList) {
  enter();
  swipe(SwipeDir::Up);  // to the second page
  EXPECT_EQ(selectedRow(), 0);
  EXPECT_EQ(textAt(ROW_Y[0] + 8), origin(DISPLAY_MAIN_0[4]));
  swipe(SwipeDir::Up);  // no third page
  EXPECT_EQ(textAt(ROW_Y[0] + 8), origin(DISPLAY_MAIN_0[4]));
  swipe(SwipeDir::Down);
  EXPECT_EQ(textAt(ROW_Y[0] + 8), origin(DISPLAY_MAIN_0[1]));
  EXPECT_EQ(selectedRow(), 0);
}

TEST_F(OnPointActivityTest, DepartureViewReversesAndBackReturnsToTheList) {
  enter();
  send(Event::Released, Button::Confirm);
  ASSERT_EQ(textAt(ROUTE_ORIGIN_Y), "BALAGTAS");
  send(Event::Pressed, Button::NavNext);  // reverse direction
  EXPECT_EQ(textAt(ROUTE_ORIGIN_Y), "TRINOMA");
  send(Event::Released, Button::Back);
  EXPECT_FALSE(inDepartureView());
  EXPECT_EQ(activityManager.goToAppsCalls, 0);
  send(Event::Released, Button::Back);
  EXPECT_EQ(activityManager.goToAppsCalls, 1);
  EXPECT_EQ(activityManager.lastFocus, AppId::OnPoint);
}

TEST_F(OnPointActivityTest, UnsetClockShowsTheClockScreenNotACountdown) {
  clockReady = false;
  enter();
  send(Event::Released, Button::Confirm);
  EXPECT_TRUE(drew("STR_ON_POINT_NOT_SET"));
  EXPECT_TRUE(drew("STR_ON_POINT_NO_COUNTDOWN"));
}

TEST_F(OnPointActivityTest, InputWaitsWhileARenderIsInProgressExceptBack) {
  enter();
  RenderLock::peekResult = true;
  send(Event::Pressed, Button::NavNext);
  EXPECT_TRUE(pinnedSelected());
  send(Event::LongPressed, Button::Confirm);
  EXPECT_EQ(SETTINGS.saves, 0);
  send(Event::Released, Button::Back);
  EXPECT_EQ(activityManager.goToAppsCalls, 1);
}

}  // namespace
