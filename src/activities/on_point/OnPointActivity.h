#pragma once

#include <cstddef>
#include <cstdint>

#include "OnPointClock.h"
#include "OnPointSchedule.h"
#include "activities/Activity.h"

class OnPointActivity final : public Activity {
  on_point::HalClockSource clock;
  on_point::DepartureState state;
  size_t scheduleIndex = 0;
  unsigned long nextRefreshAtMs = 0;
  uint8_t fastRefreshCount = 0;
  bool clockReady = false;
  bool cleanRefresh = true;

  void updateState();
  void scheduleNextRefresh();
  void drawPrimaryScreen();
  void drawEndedScreen();
  void drawHourglass(int x, int y, int width, int height) const;
  void drawLargeNumber(int value, int x, int y, int maxWidth, int height) const;

 public:
  OnPointActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("OnPoint", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
};
