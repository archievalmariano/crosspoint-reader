#pragma once

#include <cstddef>
#include <cstdint>

#include "OnPointRouteResolution.h"

namespace on_point {

// Upper bound on route pairs the list orders; route indices fit uint8_t.
constexpr size_t MAX_CATALOGUE_ROUTES = 128;
constexpr size_t ROUTE_LIST_VISIBLE_ROWS = 3;

// Route-list geometry (portrait), shared by drawing and touch hit-testing.
namespace list_geometry {
constexpr int MAIN_ROUTE_TOP = 102;
constexpr int MAIN_ROUTE_HEIGHT = 150;
constexpr int ROW_TOP = 316;
constexpr int ROW_HEIGHT = 104;
constexpr int ROW_GAP = 22;
}  // namespace list_geometry

// Orders route pairs by outbound origin, then destination, then pair ID
// (strcmp: byte-wise, case-sensitive). Negative, zero or positive like strcmp.
int compareRoutes(size_t a, size_t b);

// Heading above the pinned route.
enum class PinnedHeading : uint8_t { MainRoute, NoMainRoute };
PinnedHeading pinnedHeading(RouteResolutionState state);

// The route list's order and pinned slot, computed once per entry so input and
// rendering never re-sort. Display position 0 is the pinned slot: the resolved
// main route, or the first route in sort order when no main route resolves.
// Positions 1..count-1 are the other routes in sort order.
class RouteListModel {
 public:
  void build(const RouteResolution& resolution);

  size_t count() const { return count_; }
  const RouteResolution& resolution() const { return resolution_; }
  bool hasMainRoute() const { return resolution_.hasRoute(); }
  size_t pinnedRoute() const { return pinned_; }
  bool isMainRoute(size_t route) const { return hasMainRoute() && route == pinned_; }

  size_t routeAtPosition(size_t position) const;
  size_t positionOfRoute(size_t route) const;

  // First non-pinned row (0-based among the other routes) of the page showing
  // position, and how many rows that page holds.
  size_t pageStart(size_t position) const;
  size_t visibleRows(size_t pageStart) const;

  // Front-button navigation: Next never returns to the pinned slot.
  size_t nextPosition(size_t position) const;
  size_t previousPosition(size_t position) const;
  // Swipe paging from the page showing position; false when there is no page.
  bool swipeTarget(size_t position, bool towardsLaterPage, size_t& target) const;

  // Route under (x, y) with the page showing selectedPosition; false for gaps,
  // empty rows and points off the screen.
  bool routeAtPoint(int x, int y, int screenWidth, size_t selectedPosition, size_t& route) const;

 private:
  uint8_t sorted_[MAX_CATALOGUE_ROUTES] = {};
  uint8_t positionOf_[MAX_CATALOGUE_ROUTES] = {};
  uint8_t count_ = 0;
  uint8_t pinned_ = 0;
  uint8_t pinnedRank_ = 0;  // index of pinned_ in sorted_
  RouteResolution resolution_;
};

}  // namespace on_point
