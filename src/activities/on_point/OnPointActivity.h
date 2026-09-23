#pragma once

#include <cstddef>
#include <cstdint>

#include "OnPointClock.h"
#include "OnPointSchedule.h"
#include "activities/Activity.h"

class OnPointActivity final : public Activity {
  enum class View : uint8_t { RouteList, Departure };

  on_point::HalClockSource clock;
  on_point::DepartureState state;
  View view = View::RouteList;
  size_t routeIndex = 0;
  unsigned long nextRefreshAtMs = 0;
  uint8_t fastRefreshCount = 0;
  bool reversed = false;
  bool clockReady = false;
  bool cleanRefresh = true;

  const on_point::Schedule& selectedSchedule() const;
  size_t mainRouteIndex() const;
  size_t displayRouteIndex(size_t displayIndex) const;
  size_t selectedDisplayIndex() const;
  size_t routeListFirstPosition() const;
  size_t routeListVisibleCount() const;
  bool routeAtPoint(int x, int y, size_t& index) const;
  void selectDisplayIndex(size_t displayIndex);
  void setSelectedAsMainRoute();
  void openSelectedRoute();
  void reverseDirection();
  void updateState();
  void scheduleNextRefresh();
  void drawRouteList() const;
  void drawMainRoute(const on_point::Schedule& route, bool selected) const;
  void drawRouteListRow(const on_point::Schedule& route, bool selected, int y) const;
  void drawHeaderAndRoute(const on_point::Schedule& schedule) const;
  void drawReverseAffordance() const;
  void drawFooter(const on_point::Schedule& schedule, const char* leftLabel) const;
  void drawBadge(const char* label) const;
  void drawPrimaryScreen(const on_point::Schedule& schedule);
  void drawEndedScreen(const on_point::Schedule& schedule);
  void drawClockUnavailableScreen(const on_point::Schedule& schedule) const;
  void drawTimetableUnavailableScreen(const on_point::Schedule& schedule) const;
  void drawHourglass(int x, int y, int width, int height) const;
  void drawLargeNumber(int value, int x, int y, int maxWidth, int height) const;

 public:
  OnPointActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("OnPoint", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
};
