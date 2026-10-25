// TEST ONLY: the shipped route IDs and names, every timetable invalid (no service rules), for
// TIMETABLE UNAVAILABLE and its precedence below CLOCK NOT SET. Same contract as
// OnPointScheduleData.cpp; the shipped collections index these routes.
#include "activities/on_point/OnPointScheduleData.h"

namespace on_point {
namespace {
constexpr const char* IDS[ROUTE_COUNT] = {"p2p-balagtas-trinoma",
                                          "p2p-caypombo-sm-north-edsa",
                                          "p2p-cubao-clark-airport",
                                          "p2p-clark-airport-naia-t3",
                                          "p2p-trinoma-clark-airport",
                                          "p2p-clark-airport-baguio",
                                          "p2p-naia-pitx"};
constexpr const char* ORIGINS[ROUTE_COUNT] = {"BALAGTAS", "CAYPOMBO", "CUBAO", "CLARK", "TRINOMA", "CLARK B", "NAIA"};
Schedule schedules[SCHEDULE_COUNT];
RoutePair routes[ROUTE_COUNT];
bool ready = false;
void prepare() {
  if (ready) return;
  for (size_t r = 0; r < ROUTE_COUNT; ++r) {
    for (size_t d = 0; d < 2; ++d) {
      Schedule& s = schedules[2 * r + d];
      s.routeId = IDS[r];
      s.routeName = "TEST";
      s.origin = d == 0 ? ORIGINS[r] : "BACK";
      s.destination = d == 0 ? "THERE" : ORIGINS[r];
      s.timezone = "Asia/Manila";
      s.utcOffsetMinutes = 480;
      s.effectiveDate = {2026, 1, 1};  // no serviceRules: invalid
    }
    routes[r] = {IDS[r], 2 * r, 2 * r + 1, "A", "B"};
  }
  ready = true;
}
}  // namespace

const Schedule& scheduleAt(const size_t index) {
  prepare();
  return schedules[index < SCHEDULE_COUNT ? index : 0];
}
size_t routeCount() { return ROUTE_COUNT; }
const RoutePair& routeAt(const size_t index) {
  prepare();
  return routes[index < ROUTE_COUNT ? index : 0];
}
const Schedule& scheduleForRoute(const size_t routeIndex, const bool reversed) {
  const RoutePair& route = routeAt(routeIndex);
  return scheduleAt(reversed ? route.returnScheduleIndex : route.outboundScheduleIndex);
}
size_t routeIndexInAlphabeticalOrder(const size_t position) { return position; }
size_t routeIndexForId(const char*) { return 0; }
}  // namespace on_point
