// ON POINT route-list benchmark (TEST ONLY, not a ctest). Built twice: over the
// shipped 7 routes (OnPointRouteListBench7) and the synthetic 60-route fixture
// (OnPointRouteListBench60). Prints one JSON object per measurement.
//
// Layers: one-time model construction, steady-state lookups for one list input
// plus one render (the exact v1.2.0 call sequence), legacy v1.2.0 lookups for
// the same sequence, and getDepartureState. Comparator calls are counted (the
// machine-independent metric); times are steady_clock ns, median/p95/min/max
// over repetitions after a warm-up.

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>

#include "activities/on_point/OnPointRouteList.h"
#include "activities/on_point/OnPointSchedule.h"
#include "activities/on_point/OnPointScheduleData.h"
#include "test/fixtures/on_point/LegacyRouteList.h"

namespace on_point {
extern size_t routeComparisonCount;
}

namespace {

using Clock = std::chrono::steady_clock;
volatile size_t g_sink = 0;

struct Stats {
  double median, p95, min, max;
};

Stats stats(std::vector<double> v) {
  std::sort(v.begin(), v.end());
  const auto at = [&](double q) { return v[std::min(v.size() - 1, static_cast<size_t>(q * (v.size() - 1) + 0.5))]; };
  return {at(0.5), at(0.95), v.front(), v.back()};
}

// Runs fn `iterations` times per repetition; returns ns per call for each repetition.
template <typename Fn>
std::vector<double> time(Fn&& fn, int repetitions, int iterations, int warmup = 3) {
  for (int i = 0; i < warmup; ++i) fn();
  std::vector<double> samples;
  for (int r = 0; r < repetitions; ++r) {
    const auto start = Clock::now();
    for (int i = 0; i < iterations; ++i) fn();
    const auto ns = std::chrono::duration<double, std::nano>(Clock::now() - start).count();
    samples.push_back(ns / iterations);
  }
  return samples;
}

void print(const char* layer, const char* impl, size_t comparisons, const std::vector<double>& samples) {
  const Stats s = stats(samples);
  printf(
      "{\"dataset\":\"%s\",\"routes\":%zu,\"layer\":\"%s\",\"impl\":\"%s\",\"comparisons\":%zu,"
      "\"reps\":%zu,\"ns_median\":%.0f,\"ns_p95\":%.0f,\"ns_min\":%.0f,\"ns_max\":%.0f}\n",
      BENCH_LABEL, on_point::routeCount(), layer, impl, comparisons, samples.size(), s.median, s.p95, s.min, s.max);
}

// The v1.2.0 sequence for one list input + render, on the new model.
size_t newInputAndRender(const on_point::RouteListModel& model, const size_t routeIndex, const int touchY) {
  const size_t position = model.positionOfRoute(routeIndex);
  size_t sum = position;
  const size_t first = model.pageStart(position);
  const size_t visible = model.visibleRows(first);
  sum += first + visible + model.routeAtPosition(0);
  for (size_t row = 0; row < visible; ++row) sum += model.routeAtPosition(1 + first + row);
  sum += model.nextPosition(position);
  size_t touched = 0;
  if (model.routeAtPoint(100, touchY, 480, position, touched)) sum += touched;
  return sum;
}

}  // namespace

int main() {
  const size_t n = on_point::routeCount();
  const size_t main = n / 2;
  const size_t selected = on_point::routeIndexInAlphabeticalOrder(n - 1);  // a route on the last page
  const int touchY = 330;

  // One-time construction.
  {
    on_point::RouteListModel model;
    on_point::routeComparisonCount = 0;
    model.build({on_point::RouteResolutionState::Canonical, main});
    const size_t comparisons = on_point::routeComparisonCount;
    print("model_build", "new", comparisons,
          time([&] { model.build({on_point::RouteResolutionState::Canonical, main}); }, 30, 200));
  }

  // Steady state: one list input + render.
  {
    on_point::RouteListModel model;
    model.build({on_point::RouteResolutionState::Canonical, main});
    on_point::routeComparisonCount = 0;
    g_sink = newInputAndRender(model, selected, touchY);
    const size_t comparisons = on_point::routeComparisonCount;
    print("input_render", "new", comparisons,
          time([&] { g_sink = g_sink + newInputAndRender(model, selected, touchY); }, 30, 100000));
  }
  {
    legacy_route_list::comparisonCount() = 0;
    g_sink = legacy_route_list::oneInputAndRender(main, selected, touchY);
    const size_t comparisons = legacy_route_list::comparisonCount();
    // The 60-route legacy sequence takes tens of millions of comparisons; keep its run short.
    const bool large = n > 16;
    print("input_render", "legacy_v1.2.0", comparisons,
          time([&] { g_sink = g_sink + legacy_route_list::oneInputAndRender(main, selected, touchY); }, large ? 7 : 30,
               large ? 1 : 2000, large ? 1 : 3));
  }

  // getDepartureState on the selected route's outbound schedule at a weekday morning.
  {
    const on_point::Schedule& schedule = on_point::scheduleForRoute(selected, false);
    const on_point::CivilDateTime nowUtc{2026, 10, 21, 0, 30, 0};  // Wed 08:30 Manila
    print("departure_state", "shared", 0,
          time([&] { g_sink = g_sink + on_point::getDepartureState(schedule, nowUtc).followingCount; }, 30, 20));
  }
  return 0;
}
