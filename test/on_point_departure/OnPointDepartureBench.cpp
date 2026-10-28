// Host benchmark (not ctest): getDepartureState, the frozen reference vs the current code, per
// schedule. Each schedule is timed at 42 instants across one week (every 4 h, offset by 17 min);
// a sample is the median of 15 calls. Prints JSON lines: schedule, reference and current
// median/max over the instants, in microseconds.

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>

#include "DepartureDiff.h"
#include "activities/on_point/OnPointScheduleData.h"

#ifndef BENCH_LABEL
#define BENCH_LABEL "unlabelled"
#endif

namespace {

volatile size_t g_sink = 0;

template <typename Fn>
double medianMicros(Fn&& fn) {
  std::vector<double> samples;
  for (int i = 0; i < 15; ++i) {
    const auto start = std::chrono::steady_clock::now();
    fn();
    samples.push_back(std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count());
  }
  std::sort(samples.begin(), samples.end());
  return samples[samples.size() / 2];
}

struct Stats {
  double median = 0;
  double max = 0;
};

Stats summarise(std::vector<double> v) {
  std::sort(v.begin(), v.end());
  return {v[v.size() / 2], v.back()};
}

}  // namespace

int main() {
  const size_t count = on_point::routeCount() * 2;
  const int64_t weekStart = departure_diff::epochFromLocal(2026, 10, 19) - 8 * 3600;  // Monday
  double worstReference = 0, worstCurrent = 0;
  for (size_t i = 0; i < count; ++i) {
    const on_point::Schedule& s = on_point::scheduleAt(i);
    std::vector<double> reference, current;
    for (int k = 0; k < 42; ++k) {
      const auto now = departure_diff::civilFromEpoch(weekStart + k * 4 * 3600 + 17 * 60);
      reference.push_back(
          medianMicros([&] { g_sink = g_sink + on_point::legacy::getDepartureState(s, now).followingCount; }));
      current.push_back(medianMicros([&] { g_sink = g_sink + on_point::getDepartureState(s, now).followingCount; }));
    }
    const Stats r = summarise(reference), c = summarise(current);
    worstReference = std::max(worstReference, r.max);
    worstCurrent = std::max(worstCurrent, c.max);
    printf(
        "{\"label\":\"%s\",\"schedule\":\"%s\",\"reference_median_us\":%.2f,\"reference_max_us\":%.2f,"
        "\"current_median_us\":%.2f,\"current_max_us\":%.2f}\n",
        BENCH_LABEL, s.routeId, r.median, r.max, c.median, c.max);
  }
  printf(
      "{\"label\":\"%s\",\"summary\":true,\"schedules\":%zu,\"worst_reference_us\":%.2f,\"worst_current_us\":%.2f}\n",
      BENCH_LABEL, count, worstReference, worstCurrent);
  return 0;
}
