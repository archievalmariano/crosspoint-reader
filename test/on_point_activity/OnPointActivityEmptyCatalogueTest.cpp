#include <gtest/gtest.h>

#include <cstring>
#include <memory>
#include <string>

#include "CrossPointSettings.h"
#include "activities/on_point/OnPointActivity.h"

// OnPointActivity over a catalogue with no routes (EmptyCatalogue.cpp): it
// shows a notice and ignores every input that would need a route.

ActivityManager activityManager;
bool RenderLock::peekResult = false;
unsigned long millis() { return 1000; }
bool on_point::HalClockSource::nowUtc(CivilDateTime& value) const {
  value = {2026, 10, 21, 0, 30, 0};
  return true;
}

namespace {

using Button = MappedInputManager::Button;
using Event = MappedInputManager::Event;

TEST(OnPointActivityEmptyCatalogue, ShowsANoticeAndIgnoresRouteInput) {
  SETTINGS.reset();
  GfxRenderer renderer;
  MappedInputManager input;
  OnPointActivity activity(renderer, input);
  activity.onEnter();
  activity.render(RenderLock{});
  const auto drew = [&](const std::string& text) {
    for (const auto& t : renderer.texts)
      if (t.text == text) return true;
    return false;
  };
  EXPECT_TRUE(drew("STR_ON_POINT_TIMETABLE_UNAVAILABLE"));
  EXPECT_FALSE(drew("STR_ON_POINT_MAIN_ROUTE"));

  const struct {
    Event event;
    Button button;
  } inputs[] = {{Event::Pressed, Button::NavNext},     {Event::Pressed, Button::NavPrevious},
                {Event::Released, Button::Confirm},    {Event::LongPressed, Button::Confirm},
                {Event::ScreenTap, Button::Back},      {Event::ScreenLongPress, Button::Back},
                {Event::ScreenTouchDown, Button::Back}};
  for (const auto& in : inputs) {
    input.event = in.event;
    input.button = in.button;
    input.x = 100;
    input.y = 150;
    activity.loop();
    input.clear();
    activity.render(RenderLock{});
    EXPECT_TRUE(drew("STR_ON_POINT_TIMETABLE_UNAVAILABLE"));
  }
  EXPECT_EQ(SETTINGS.saves, 0);
  input.event = Event::Released;
  input.button = Button::Back;
  activity.loop();
  EXPECT_EQ(activityManager.goToAppsCalls, 1);
}

}  // namespace
