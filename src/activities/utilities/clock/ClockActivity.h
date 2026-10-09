#pragma once

#include <cstdint>

#include "activities/Activity.h"
#include "util/CivilCalendar.h"

// Clock utility (boards with an RTC): the local time as the dominant element,
// hours over minutes, with the weekday and date beneath it. No seconds. Redraws at each minute boundary
// while open, driven from loop() like ON POINT's departure board, so leaving
// the app stops all refresh work.
class ClockActivity final : public Activity {
  civil_cal::DateTime now;
  bool clockReady = false;
  bool cleanRefresh = true;
  uint8_t fastRefreshCount = 0;
  unsigned long nextRefreshAtMs = 0;

  void updateTime();
  int drawTime(int top) const;
  void drawDate(int top) const;
  void drawNotSet() const;

 public:
  ClockActivity(GfxRenderer& renderer, MappedInputManager& mappedInput) : Activity("Clock", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
};
