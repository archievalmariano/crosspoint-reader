// Comprehensive differential acceptance (ON_POINT_DIFFERENTIAL builds only): every shipped
// schedule every 7 minutes over 2026-2027, plus every minute around its overrides and effective
// date and around each month boundary. One test per schedule so ctest runs them in parallel.

#include <gtest/gtest.h>

#include "DepartureDiff.h"
#include "activities/on_point/OnPointScheduleData.h"

namespace {

using departure_diff::epochFromLocal;
using departure_diff::sweep;

constexpr int64_t MANILA = 8 * 3600;

class FullShippedDiff : public ::testing::TestWithParam<size_t> {};

TEST_P(FullShippedDiff, EveryScheduleAcrossTwoYears) {
  const on_point::Schedule& s = on_point::scheduleAt(GetParam());
  EXPECT_GT(sweep(s, epochFromLocal(2026, 1, 1) - MANILA, epochFromLocal(2028, 1, 1) - MANILA, 7 * 60, "shipped full"),
            150000u);
  const int64_t effective = epochFromLocal(s.effectiveDate.year, s.effectiveDate.month, s.effectiveDate.day) - MANILA;
  sweep(s, effective - 7200, effective + 7200, 60, "shipped effective");
  for (size_t o = 0; o < s.dateOverrideCount; ++o) {
    const on_point::CivilDate& d = s.dateOverrides[o].date;
    const int64_t start = epochFromLocal(d.year, d.month, d.day) - MANILA;
    sweep(s, start - 7200, start + 7200, 60, "shipped override start");
    sweep(s, start + 86400 - 7200, start + 86400 + 7200, 60, "shipped override end");
  }
  for (int year = 2026; year <= 2027; ++year)
    for (unsigned month = 1; month <= 12; ++month) {
      const int64_t start = epochFromLocal(year, month, 1) - MANILA;
      sweep(s, start - 7200, start + 7200, 60, "shipped month");
    }
}

INSTANTIATE_TEST_SUITE_P(Schedules, FullShippedDiff, ::testing::Range<size_t>(0, on_point::SCHEDULE_COUNT));

}  // namespace
