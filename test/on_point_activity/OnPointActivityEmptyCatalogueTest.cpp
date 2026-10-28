// The production OnPointActivity with no routes at all (EmptyCatalogue.cpp): a notice, no
// collection row, every list input ignored, nothing written.

#include "OnPointActivityHarness.h"

namespace {

using namespace harness;

class OnPointActivityEmptyCatalogue : public ActivityTest {};

TEST_F(OnPointActivityEmptyCatalogue, ShowsANoticeAndIgnoresRouteInput) {
  enter();
  EXPECT_FALSE(inDepartureView());
  EXPECT_TRUE(drew("STR_ON_POINT_TIMETABLE_UNAVAILABLE"));
  EXPECT_FALSE(drew("STR_ON_POINT_MAIN_ROUTE"));
  EXPECT_FALSE(collectionRowShown());

  const struct {
    Event event;
    Button button;
  } inputs[] = {{Event::Pressed, Button::NavNext},     {Event::Pressed, Button::NavPrevious},
                {Event::Released, Button::Confirm},    {Event::LongPressed, Button::Confirm},
                {Event::ScreenTap, Button::Back},      {Event::ScreenLongPress, Button::Back},
                {Event::ScreenTouchDown, Button::Back}};
  for (const auto& in : inputs) {
    input.x = 100;
    input.y = 150;
    send(in.event, in.button);
    EXPECT_TRUE(drew("STR_ON_POINT_TIMETABLE_UNAVAILABLE"));
    EXPECT_FALSE(inPicker());
  }
  EXPECT_EQ(storage.mutations, 0);
  EXPECT_EQ(SETTINGS.saves, 0);
  send(Event::Released, Button::Back);
  EXPECT_EQ(activityManager.goToAppsCalls, 1);
}

}  // namespace
