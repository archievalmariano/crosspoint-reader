// TEST ONLY: a route catalogue with no routes, for OnPointActivity's
// zero-route guards. Same contract as OnPointScheduleData.cpp.
#include "activities/on_point/OnPointScheduleData.h"

namespace on_point {
namespace {
const Schedule EMPTY_SCHEDULE{};
const RoutePair EMPTY_ROUTE{};
}  // namespace

const Schedule& scheduleAt(size_t) { return EMPTY_SCHEDULE; }
size_t routeCount() { return 0; }
const RoutePair& routeAt(size_t) { return EMPTY_ROUTE; }
const Schedule& scheduleForRoute(size_t, bool) { return EMPTY_SCHEDULE; }
size_t routeIndexInAlphabeticalOrder(size_t) { return 0; }
size_t routeIndexForId(const char*) { return 0; }
}  // namespace on_point
