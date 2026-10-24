// getDepartureState vs the frozen reference over the synthetic 60-route capacity fixture
// (120 schedules: dense timetables, overrides, cross-midnight trips).
//
// Normal suite: 20 schedules, sampled. With ON_POINT_DIFFERENTIAL_FULL: every schedule, one
// test per schedule so ctest can run them in parallel (the comprehensive acceptance job).

#include <gtest/gtest.h>

#include "DepartureDiff.h"
#include "activities/on_point/OnPointScheduleData.h"

namespace {

using departure_diff::epochFromLocal;
using departure_diff::sweep;

constexpr int64_t MANILA = 8 * 3600;

#ifndef ON_POINT_DIFFERENTIAL_FULL

size_t fixtureScheduleCount() { return on_point::routeCount() * 2; }

TEST(OnPointDepartureFixtureDiff, SampledSchedulesAcrossTwoYears) {
  ASSERT_EQ(fixtureScheduleCount(), 120u);
  const int64_t from = epochFromLocal(2026, 1, 1) - MANILA;
  const int64_t to = epochFromLocal(2028, 1, 1) - MANILA;
  for (size_t i = 0; i < fixtureScheduleCount(); i += 6) sweep(on_point::scheduleAt(i), from, to, 2999 * 60, "fixture");
}

#else

class FullFixtureDiff : public ::testing::TestWithParam<size_t> {};

TEST_P(FullFixtureDiff, EveryScheduleAcrossTwoYears) {
  const on_point::Schedule& s = on_point::scheduleAt(GetParam());
  // Every 241 minutes over 2026-2027.
  sweep(s, epochFromLocal(2026, 1, 1) - MANILA, epochFromLocal(2028, 1, 1) - MANILA, 241 * 60, "fixture full");
  // Every minute within 2 h of each override date's start and end, and of the effective date.
  const int64_t effective = epochFromLocal(s.effectiveDate.year, s.effectiveDate.month, s.effectiveDate.day) - MANILA;
  sweep(s, effective - 7200, effective + 7200, 60, "fixture effective");
  for (size_t o = 0; o < s.dateOverrideCount; ++o) {
    const on_point::CivilDate& d = s.dateOverrides[o].date;
    const int64_t start = epochFromLocal(d.year, d.month, d.day) - MANILA;
    sweep(s, start - 7200, start + 7200, 60, "fixture override start");
    sweep(s, start + 86400 - 7200, start + 86400 + 7200, 60, "fixture override end");
  }
  // Every minute within 2 h of each month boundary.
  for (int year = 2026; year <= 2027; ++year)
    for (unsigned month = 1; month <= 12; ++month) {
      const int64_t start = epochFromLocal(year, month, 1) - MANILA;
      sweep(s, start - 7200, start + 7200, 60, "fixture month");
    }
}

INSTANTIATE_TEST_SUITE_P(Schedules, FullFixtureDiff, ::testing::Range<size_t>(0, 120));

#endif

}  // namespace
