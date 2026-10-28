#pragma once

// TEST/BENCH ONLY. The v1.2.0 ON POINT route-list logic (tag v1.2.0, commit
// 85bad542: OnPointScheduleData.cpp routeIndexInAlphabeticalOrder and the
// OnPointActivity helpers), copied as free functions with the main route as a
// parameter. It is the reference for the golden tables, the equivalence tests
// and the old-vs-new benchmarks. Never compiled into a release build.

#include <algorithm>
#include <cstddef>
#include <cstring>

#include "activities/on_point/OnPointScheduleData.h"

namespace legacy_route_list {

inline size_t& comparisonCount() {
  static size_t count = 0;
  return count;
}

constexpr size_t VISIBLE_ROWS = 3;
constexpr int MAIN_ROUTE_TOP = 102;
constexpr int MAIN_ROUTE_HEIGHT = 150;
constexpr int ROW_TOP = 316;
constexpr int ROW_HEIGHT = 104;
constexpr int ROW_GAP = 22;

// OnPointScheduleData.cpp: routeIndexInAlphabeticalOrder (v1.2.0), with one
// count per route-pair comparison.
inline size_t routeIndexInAlphabeticalOrder(const size_t position) {
  const size_t total = on_point::routeCount();
  if (position >= total) return 0;
  for (size_t candidate = 0; candidate < total; ++candidate) {
    const on_point::Schedule& candidateSchedule =
        on_point::scheduleAt(on_point::routeAt(candidate).outboundScheduleIndex);
    size_t earlierRoutes = 0;
    for (size_t other = 0; other < total; ++other) {
      if (other == candidate) continue;
      ++comparisonCount();
      const on_point::Schedule& otherSchedule = on_point::scheduleAt(on_point::routeAt(other).outboundScheduleIndex);
      int comparison = strcmp(otherSchedule.origin, candidateSchedule.origin);
      if (comparison == 0) comparison = strcmp(otherSchedule.destination, candidateSchedule.destination);
      if (comparison == 0) comparison = strcmp(on_point::routeAt(other).routeId, on_point::routeAt(candidate).routeId);
      if (comparison < 0) ++earlierRoutes;
    }
    if (earlierRoutes == position) return candidate;
  }
  return 0;
}

// OnPointActivity::displayRouteIndex
inline size_t displayRouteIndex(const size_t mainIndex, const size_t displayIndex) {
  if (displayIndex == 0 || displayIndex >= on_point::routeCount()) return mainIndex;
  size_t visiblePosition = 1;
  for (size_t position = 0; position < on_point::routeCount(); ++position) {
    const size_t index = routeIndexInAlphabeticalOrder(position);
    if (index == mainIndex) continue;
    if (visiblePosition == displayIndex) return index;
    ++visiblePosition;
  }
  return mainIndex;
}

// OnPointActivity::selectedDisplayIndex
inline size_t selectedDisplayIndex(const size_t mainIndex, const size_t routeIndex) {
  for (size_t displayIndex = 0; displayIndex < on_point::routeCount(); ++displayIndex) {
    if (displayRouteIndex(mainIndex, displayIndex) == routeIndex) return displayIndex;
  }
  return 0;
}

// OnPointActivity::routeListFirstPosition
inline size_t routeListFirstPosition(const size_t mainIndex, const size_t routeIndex) {
  const size_t displayIndex = selectedDisplayIndex(mainIndex, routeIndex);
  if (displayIndex == 0) return 0;
  return (displayIndex - 1) / VISIBLE_ROWS * VISIBLE_ROWS;
}

// OnPointActivity::routeListVisibleCount
inline size_t routeListVisibleCount(const size_t mainIndex, const size_t routeIndex) {
  const size_t otherRouteCount = on_point::routeCount() > 0 ? on_point::routeCount() - 1 : 0;
  return std::min(VISIBLE_ROWS, otherRouteCount - routeListFirstPosition(mainIndex, routeIndex));
}

// OnPointActivity::routeAtPoint
inline bool routeAtPoint(const size_t mainIndex, const size_t routeIndex, const int x, const int y,
                         const int screenWidth, size_t& index) {
  if (x < 0 || x >= screenWidth) return false;
  if (y >= MAIN_ROUTE_TOP && y < MAIN_ROUTE_TOP + MAIN_ROUTE_HEIGHT) {
    index = mainIndex;
    return true;
  }
  if (y < ROW_TOP) return false;
  const int rowStep = ROW_HEIGHT + ROW_GAP;
  const int row = (y - ROW_TOP) / rowStep;
  if (row < 0 || row >= static_cast<int>(routeListVisibleCount(mainIndex, routeIndex)) ||
      (y - ROW_TOP) % rowStep >= ROW_HEIGHT)
    return false;
  index = displayRouteIndex(mainIndex, 1 + routeListFirstPosition(mainIndex, routeIndex) + static_cast<size_t>(row));
  return true;
}

// OnPointActivity::loop, NavNext: the display index selected next.
inline size_t navNext(const size_t mainIndex, const size_t routeIndex) {
  const size_t routeCount = on_point::routeCount();
  const size_t selected = selectedDisplayIndex(mainIndex, routeIndex);
  if (routeCount <= 1) return selected;
  return selected == 0 || selected + 1 >= routeCount ? 1 : selected + 1;
}

// OnPointActivity::loop, NavPrevious.
inline size_t navPrevious(const size_t mainIndex, const size_t routeIndex) {
  const size_t routeCount = on_point::routeCount();
  const size_t selected = selectedDisplayIndex(mainIndex, routeIndex);
  if (routeCount <= 1) return selected;
  return selected <= 1 ? routeCount - 1 : selected - 1;
}

// OnPointActivity::loop, swipe paging: the route selected, or false.
inline bool swipe(const size_t mainIndex, const size_t routeIndex, const bool up, size_t& newRoute) {
  const size_t first = routeListFirstPosition(mainIndex, routeIndex);
  const size_t otherRouteCount = on_point::routeCount() - 1;
  if (up && first + VISIBLE_ROWS < otherRouteCount) {
    newRoute = displayRouteIndex(mainIndex, 1 + first + VISIBLE_ROWS);
    return true;
  }
  if (!up && first >= VISIBLE_ROWS) {
    newRoute = displayRouteIndex(mainIndex, 1 + first - VISIBLE_ROWS);
    return true;
  }
  return false;
}

// The calls one list render and one input make in v1.2.0 (drawRouteList +
// a NavNext press + a touch probe), for benchmarks. Returns a checksum so the
// work can't be optimised away.
inline size_t oneInputAndRender(const size_t mainIndex, const size_t routeIndex, const int touchY) {
  size_t sum = selectedDisplayIndex(mainIndex, routeIndex);
  const size_t first = routeListFirstPosition(mainIndex, routeIndex);
  const size_t visible = routeListVisibleCount(mainIndex, routeIndex);
  sum += first + visible + displayRouteIndex(mainIndex, 0);
  for (size_t row = 0; row < visible; ++row) sum += displayRouteIndex(mainIndex, 1 + first + row);
  sum += navNext(mainIndex, routeIndex);
  size_t touched = 0;
  if (routeAtPoint(mainIndex, routeIndex, 100, touchY, 480, touched)) sum += touched;
  return sum;
}

}  // namespace legacy_route_list
