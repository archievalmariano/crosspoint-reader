#include "OnPointRouteList.h"

#include <Logging.h>

#include <algorithm>
#include <cstring>
#include <iterator>

#include "OnPointScheduleData.h"

#ifdef ON_POINT_COUNT_COMPARISONS
namespace on_point {
size_t routeComparisonCount = 0;  // host benchmarks only
}
#endif

namespace on_point {

int compareRoutes(const size_t a, const size_t b) {
#ifdef ON_POINT_COUNT_COMPARISONS
  ++routeComparisonCount;
#endif
  const RoutePair& routeA = routeAt(a);
  const RoutePair& routeB = routeAt(b);
  const Schedule& outboundA = scheduleAt(routeA.outboundScheduleIndex);
  const Schedule& outboundB = scheduleAt(routeB.outboundScheduleIndex);
  int comparison = strcmp(outboundA.origin, outboundB.origin);
  if (comparison == 0) comparison = strcmp(outboundA.destination, outboundB.destination);
  if (comparison == 0) comparison = strcmp(routeA.routeId, routeB.routeId);
  return comparison;
}

PinnedHeading pinnedHeading(const RouteResolutionState state) {
  return state == RouteResolutionState::Canonical || state == RouteResolutionState::Aliased
             ? PinnedHeading::MainRoute
             : PinnedHeading::NoMainRoute;
}

void RouteListModel::build(const RouteResolution& resolution) {
  size_t total = routeCount();
  if (total > MAX_CATALOGUE_ROUTES) {
    LOG_ERR("ONPT", "Route catalogue has %u routes; ordering the first %u", static_cast<unsigned>(total),
            static_cast<unsigned>(MAX_CATALOGUE_ROUTES));
    total = MAX_CATALOGUE_ROUTES;
  }
  uint8_t all[MAX_CATALOGUE_ROUTES];
  for (size_t i = 0; i < total; ++i) all[i] = static_cast<uint8_t>(i);
  build(all, total, resolution);
}

void RouteListModel::build(const uint8_t* routes, const size_t entryCount, const RouteResolution& resolution) {
  const size_t catalogue = std::min(routeCount(), MAX_CATALOGUE_ROUTES);
  std::fill(std::begin(positionOf_), std::end(positionOf_), NOT_LISTED);
  std::fill(std::begin(sorted_), std::end(sorted_), 0);
  count_ = 0;
  pinned_ = 0;
  pinnedRank_ = 0;
  resolution_ = resolution;

  // Insertion sort, once per build. positionOf_ marks routes already taken.
  for (size_t k = 0; routes != nullptr && k < entryCount; ++k) {
    const size_t route = routes[k];
    if (route >= catalogue || positionOf_[route] != NOT_LISTED) {
      LOG_ERR("ONPT", "Skipping route list entry %u (route %u)", static_cast<unsigned>(k),
              static_cast<unsigned>(route));
      continue;
    }
    positionOf_[route] = 0;
    size_t j = count_;
    while (j > 0 && compareRoutes(route, sorted_[j - 1]) < 0) {
      sorted_[j] = sorted_[j - 1];
      --j;
    }
    sorted_[j] = static_cast<uint8_t>(route);
    ++count_;
  }

  if (resolution_.hasRoute() && !contains(resolution_.index)) resolution_.state = RouteResolutionState::Unknown;
  pinned_ = count_ == 0 ? 0 : (resolution_.hasRoute() ? static_cast<uint8_t>(resolution_.index) : sorted_[0]);

  size_t position = 1;
  for (size_t i = 0; i < count_; ++i) {
    const uint8_t route = sorted_[i];
    if (route == pinned_) {
      pinnedRank_ = static_cast<uint8_t>(i);
      positionOf_[route] = 0;
    } else {
      positionOf_[route] = static_cast<uint8_t>(position++);
    }
  }
}

bool RouteListModel::contains(const size_t route) const {
  return route < MAX_CATALOGUE_ROUTES && positionOf_[route] != NOT_LISTED;
}

size_t RouteListModel::routeAtPosition(const size_t position) const {
  if (position == 0 || position >= count_) return pinned_;
  // Positions after the pinned slot walk the sort order with the pinned route removed.
  const size_t rank = position - 1 < pinnedRank_ ? position - 1 : position;
  return sorted_[rank];
}

size_t RouteListModel::positionOfRoute(const size_t route) const { return contains(route) ? positionOf_[route] : 0; }

size_t RouteListModel::pageStart(const size_t position) {
  if (position == 0) return 0;
  return (position - 1) / ROUTE_LIST_VISIBLE_ROWS * ROUTE_LIST_VISIBLE_ROWS;
}

size_t RouteListModel::visibleRows(const size_t start) const {
  const size_t others = count_ > 0 ? count_ - 1 : 0;
  if (start >= others) return 0;
  return std::min(ROUTE_LIST_VISIBLE_ROWS, others - start);
}

size_t RouteListModel::nextPosition(const size_t position) const {
  if (count_ <= 1) return position;
  return position == 0 || position + 1 >= count_ ? 1 : position + 1;
}

size_t RouteListModel::previousPosition(const size_t position) const {
  if (count_ <= 1) return position;
  return position <= 1 ? count_ - 1 : position - 1;
}

bool RouteListModel::swipeTarget(const size_t position, const bool towardsLaterPage, size_t& target) const {
  if (count_ == 0) return false;
  const size_t first = pageStart(position);
  const size_t others = count_ - 1;
  if (towardsLaterPage && first + ROUTE_LIST_VISIBLE_ROWS < others) {
    target = 1 + first + ROUTE_LIST_VISIBLE_ROWS;
    return true;
  }
  if (!towardsLaterPage && first >= ROUTE_LIST_VISIBLE_ROWS) {
    target = 1 + first - ROUTE_LIST_VISIBLE_ROWS;
    return true;
  }
  return false;
}

bool RouteListModel::routeAtPoint(const int x, const int y, const int screenWidth, const size_t selectedPosition,
                                  size_t& route) const {
  using namespace list_geometry;
  if (count_ == 0 || x < 0 || x >= screenWidth) return false;
  if (y >= MAIN_ROUTE_TOP && y < MAIN_ROUTE_TOP + MAIN_ROUTE_HEIGHT) {
    route = pinned_;
    return true;
  }
  if (y < ROW_TOP) return false;
  const int rowStep = ROW_HEIGHT + ROW_GAP;
  const int row = (y - ROW_TOP) / rowStep;
  const size_t first = pageStart(selectedPosition);
  if (row >= static_cast<int>(visibleRows(first)) || (y - ROW_TOP) % rowStep >= ROW_HEIGHT) return false;
  route = routeAtPosition(1 + first + static_cast<size_t>(row));
  return true;
}

}  // namespace on_point
