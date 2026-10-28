#pragma once

#include <cstddef>
#include <cstdint>

#include "OnPointClock.h"
#include "OnPointRouteList.h"
#include "OnPointSchedule.h"
#include "OnPointStore.h"
#include "activities/Activity.h"

class OnPointActivity final : public Activity {
  enum class View : uint8_t { RouteList, Departure, CollectionPicker };

  on_point::HalClockSource clock;
  on_point::DepartureState state;
  on_point::OnPointStore store{on_point::onPointStorage()};
  // The active collection's members, rebuilt on entry, a collection switch or a main-route change.
  on_point::RouteListModel routeList;
  View view = View::RouteList;
  size_t region = 0;  // active collection (regionAt)
  size_t routeIndex = 0;
  size_t pickerCursor = 0;
  bool collectionFocused = false;  // the SELECT COLLECTION row, not a route, has the focus
  unsigned long nextRefreshAtMs = 0;
  uint8_t fastRefreshCount = 0;
  bool reversed = false;
  bool clockReady = false;
  bool cleanRefresh = true;

  const on_point::Schedule& selectedSchedule() const;
  void buildRouteList();
  bool hasCollectionRow() const;
  const char* routeListFooterLabel() const;
  void moveRouteFocus(bool forward);
  void openCollectionPicker();
  void closeCollectionPicker();
  void selectCollection(size_t index);
  void handleRouteListInput();
  void handleCollectionPickerInput();
  size_t selectedDisplayIndex() const;
  size_t routeListFirstPosition() const;
  void selectDisplayIndex(size_t displayIndex);
  void setSelectedAsMainRoute();
  void openSelectedRoute();
  void reverseDirection();
  void updateState();
  void scheduleNextRefresh();
  void drawRouteList() const;
  void drawCollectionRow() const;
  void drawCollectionPicker() const;
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
