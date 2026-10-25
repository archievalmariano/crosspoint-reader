// DEV/BENCH ONLY: ON POINT timing on the X4 Pro, logged over serial when ON
// POINT opens, over the real catalogue (x4pro-all-devtest) or the synthetic
// 60-route fixture (x4pro-all-capacity-bench, x4pro-all-capacity-devtest).
// Compiles to nothing unless ON_POINT_BENCH_TIMING is defined, which only those
// dev envs do (scripts/check_no_capacity_fixture.py).
#ifdef ON_POINT_BENCH_TIMING

#include <Logging.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>

#include "../../../test/fixtures/on_point/LegacyRouteList.h"
#include "activities/on_point/OnPointActivity.h"
#include "activities/on_point/OnPointRouteList.h"
#include "activities/on_point/OnPointSchedule.h"
#include "activities/on_point/OnPointScheduleData.h"

namespace on_point_bench {

// Heap-resident sizes, readable from the ELF and logged below.
extern const size_t ROUTE_LIST_MODEL_SIZE = sizeof(on_point::RouteListModel);
extern const size_t ON_POINT_ACTIVITY_SIZE = sizeof(OnPointActivity);

namespace {

constexpr int REPETITIONS = 30;
constexpr int DISCARD = 3;
volatile size_t sink = 0;

template <typename Fn>
void report(const char* label, Fn&& fn) {
  int64_t samples[REPETITIONS];
  for (int i = 0; i < REPETITIONS; ++i) {
    const int64_t start = esp_timer_get_time();
    fn();
    samples[i] = esp_timer_get_time() - start;
  }
  std::sort(samples + DISCARD, samples + REPETITIONS);
  const int kept = REPETITIONS - DISCARD;
  LOG_INF("ONPTB", "%s us median=%lld p95=%lld min=%lld max=%lld n=%d", label,
          static_cast<long long>(samples[DISCARD + kept / 2]),
          static_cast<long long>(samples[DISCARD + (kept * 95) / 100]), static_cast<long long>(samples[DISCARD]),
          static_cast<long long>(samples[REPETITIONS - 1]), kept);
}

}  // namespace

void logStacks(const char* where);

void runOnEnter() {
  const size_t n = on_point::routeCount();
  const size_t main = n / 2;
  const size_t selected = on_point::routeIndexInAlphabeticalOrder(n - 1);
  LOG_INF("ONPTB", "routes=%u model=%uB activity=%uB", static_cast<unsigned>(n),
          static_cast<unsigned>(ROUTE_LIST_MODEL_SIZE), static_cast<unsigned>(ON_POINT_ACTIVITY_SIZE));

  on_point::RouteListModel model;
  report("model_build", [&] { model.build({on_point::RouteResolutionState::Canonical, main}); });
  report("input_render_new", [&] {
    const size_t position = model.positionOfRoute(selected);
    const size_t first = model.pageStart(position);
    size_t sum = position + first + model.visibleRows(first) + model.routeAtPosition(0) + model.nextPosition(position);
    for (size_t row = 0; row < model.visibleRows(first); ++row) sum += model.routeAtPosition(1 + first + row);
    size_t touched = 0;
    if (model.routeAtPoint(100, 330, 480, position, touched)) sum += touched;
    sink = sink + sum;
  });
  // The full legacy sequence would run for seconds per input at 60 routes (and
  // trip the watchdog), so time one legacy ranking call (n*(n-1) comparisons)
  // and project with the host-counted comparisons per input.
  report("legacy_rank_call", [&] { sink = sink + legacy_route_list::routeIndexInAlphabeticalOrder(n / 2); });
  const on_point::Schedule& schedule = on_point::scheduleForRoute(selected, false);
  report("departure_state",
         [&] { sink = sink + on_point::getDepartureState(schedule, {2026, 10, 21, 0, 30, 0}).followingCount; });

  // Every schedule (both directions) on a Wednesday at 08:30 and at 22:30 Manila: the morning
  // case finds its trips on the day; the late one looks past midnight into the next day.
  constexpr on_point::CivilDateTime INSTANTS[] = {{2026, 10, 21, 0, 30, 0}, {2026, 10, 21, 14, 30, 0}};
  constexpr const char* INSTANT_NAMES[] = {"0830", "2230"};
  for (size_t route = 0; route < n; ++route) {
    for (const bool reversed : {false, true}) {
      const on_point::Schedule& s = on_point::scheduleForRoute(route, reversed);
      for (size_t i = 0; i < 2; ++i) {
        char label[80];
        snprintf(label, sizeof(label), "departure_state[%s@%s]", s.routeId, INSTANT_NAMES[i]);
        report(label, [&] { sink = sink + on_point::getDepartureState(s, INSTANTS[i]).followingCount; });
      }
    }
  }
  logStacks("enter");
}

void logStacks(const char* where) {
  // Free stack, in bytes, at its lowest so far, of the calling task.
  LOG_INF("ONPTB", "stack %s task=%s low_water_bytes=%u", where, pcTaskGetName(nullptr),
          static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr) * sizeof(StackType_t)));
}

}  // namespace on_point_bench

#endif  // ON_POINT_BENCH_TIMING
