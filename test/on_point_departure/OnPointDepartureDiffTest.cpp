// getDepartureState must equal the frozen reference (every field) on the shipped timetables and
// on hand-built boundary schedules. Codex's requested cases are tagged C1-C7.

#include <gtest/gtest.h>

#include <memory>

#include "DepartureDiff.h"
#include "activities/on_point/OnPointScheduleData.h"

namespace {

using departure_diff::ALL_DAYS;
using departure_diff::Built;
using departure_diff::check;
using departure_diff::epochFromLocal;
using departure_diff::sweep;
using departure_diff::utcForLocal;
using on_point::ServiceStatus;

constexpr int64_t MANILA = 8 * 3600;

// Every minute at :00, :01 and :59 seconds over a local window (UTC epochs under +08:00).
size_t denseSweep(const Built& built, const int64_t localFrom, const int64_t localTo, const char* context) {
  size_t count = 0;
  for (const int64_t second : {0, 1, 59})
    count += sweep(built.schedule, localFrom - MANILA + second, localTo - MANILA, 60, context);
  return count;
}

// ---- Shipped timetables (sampled) ----

TEST(OnPointDepartureDiff, ShippedSchedulesSampledAcrossTwoYears) {
  // Every 997 minutes (a prime, so the time of day drifts) from 2026-01-01 to 2027-12-31.
  const int64_t from = epochFromLocal(2026, 1, 1) - MANILA;
  const int64_t to = epochFromLocal(2028, 1, 1) - MANILA;
  for (size_t i = 0; i < on_point::SCHEDULE_COUNT; ++i) {
    const size_t n = sweep(on_point::scheduleAt(i), from, to, 997 * 60, "shipped");
    EXPECT_GT(n, 1000u);
  }
}

TEST(OnPointDepartureDiff, ShippedSchedulesAroundTheirOverridesAndEffectiveDates) {
  for (size_t i = 0; i < on_point::SCHEDULE_COUNT; ++i) {
    const on_point::Schedule& s = on_point::scheduleAt(i);
    const int64_t effective = epochFromLocal(s.effectiveDate.year, s.effectiveDate.month, s.effectiveDate.day) - MANILA;
    // Every 5 min (every minute in the comprehensive job).
    sweep(s, effective - 2 * 3600, effective + 2 * 3600, 300, "shipped effective date");
    for (size_t o = 0; o < s.dateOverrideCount; ++o) {
      const on_point::CivilDate& d = s.dateOverrides[o].date;
      const int64_t start = epochFromLocal(d.year, d.month, d.day) - MANILA;
      sweep(s, start - 2 * 3600, start + 26 * 3600, 300, "shipped override");
    }
  }
}

// ---- C1: effective-date midnight and the day before ----

TEST(OnPointDepartureDiff, C1_EffectiveDateMidnightAndTheDayBefore) {
  const Built built("c1", {2026, 3, 10}, {{ALL_DAYS, {360, 720, 1439, 1440, 1500, 2879}}});
  denseSweep(built, epochFromLocal(2026, 3, 8), epochFromLocal(2026, 3, 12), "C1");

  // The day before: nothing yet (that service day, and its trips >= 1440, aren't in effect).
  auto state = check(built.schedule, utcForLocal(built.schedule, 2026, 3, 9, 23, 59, 59), "C1 day before");
  EXPECT_EQ(state.serviceStatus, ServiceStatus::NoServiceToday);
  EXPECT_EQ(state.next.local.day, 10);
  EXPECT_EQ(state.next.local.hour, 6);  // 03-10 06:00, not 03-10 00:00 from 03-09's minute 1440
  // Exactly midnight on the effective date.
  state = check(built.schedule, utcForLocal(built.schedule, 2026, 3, 10), "C1 midnight");
  EXPECT_EQ(state.serviceStatus, ServiceStatus::BeforeFirst);
  EXPECT_EQ(state.next.serviceMinute, 360);
}

// ---- C2: overrides, empty overrides and the UTC/local-day boundary ----

TEST(OnPointDepartureDiff, C2_OverridesAndEmptyOverridesAtUtcLocalBoundaries) {
  const Built built("c2", {2026, 1, 1}, {{ALL_DAYS, {480, 1020}}},
                    {{{2026, 4, 1}, {600, 700, 1500}}, {{2026, 4, 2}, {}}, {{2026, 4, 3}, {}}, {{2026, 4, 4}, {}}});
  denseSweep(built, epochFromLocal(2026, 3, 31, 12), epochFromLocal(2026, 4, 6), "C2");

  // Local 04-01 00:00:30 is UTC 03-31 16:00:30: the 04-01 override applies by local date.
  auto state = check(built.schedule, utcForLocal(built.schedule, 2026, 4, 1, 0, 0, 30), "C2 local override");
  EXPECT_EQ(state.next.local.day, 1);
  EXPECT_EQ(state.next.serviceMinute, 600);
  // Local 04-02 01:00:01 is UTC 04-01 17:00:01: 04-01's override doesn't make 04-02 a service day.
  state = check(built.schedule, utcForLocal(built.schedule, 2026, 4, 2, 1, 0, 1), "C2 UTC date only");
  EXPECT_EQ(state.next.local.month, 4);
  EXPECT_EQ(state.next.local.day, 5);  // after three empty overrides
  EXPECT_EQ(state.next.serviceMinute, 480);
  // Cross-midnight trip from the replacement list, on the next calendar day.
  state = check(built.schedule, utcForLocal(built.schedule, 2026, 4, 2, 0, 59, 0), "C2 cross-midnight");
  EXPECT_EQ(state.next.serviceMinute, 1500);
}

// ---- C3 and C5: service minutes 1439, 1440, 2879, and duplicate epochs ----

TEST(OnPointDepartureDiff, C3_ServiceMinutes1439_1440_2879) {
  // Starts 05-10, so 05-10's 23:59 is minute 1439 alone (no 05-09 minute 2879 at the same instant).
  const Built built("c3", {2026, 5, 10}, {{ALL_DAYS, {1439, 1440, 2879}}});
  denseSweep(built, epochFromLocal(2026, 5, 9), epochFromLocal(2026, 5, 13), "C3");

  auto state = check(built.schedule, utcForLocal(built.schedule, 2026, 5, 10, 23, 58, 30), "C3 1439");
  EXPECT_EQ(state.next.serviceMinute, 1439);
  state = check(built.schedule, utcForLocal(built.schedule, 2026, 5, 10, 23, 59, 30), "C3 1440");
  EXPECT_EQ(state.next.serviceMinute, 1440);
  EXPECT_EQ(state.next.local.day, 11);
  // 05-11 23:59 is both 05-11 minute 1439 and 05-10 minute 2879; the earlier service day wins.
  state = check(built.schedule, utcForLocal(built.schedule, 2026, 5, 11, 23, 58, 30), "C3 2879 duplicate");
  EXPECT_EQ(state.next.serviceMinute, 2879);
}

TEST(OnPointDepartureDiff, C5_DuplicateEpochAtNextDayMidnight) {
  // Day d minute 1440 and day d+1 minute 0 are the same instant.
  const Built built("c5", {2026, 1, 1}, {{ALL_DAYS, {0, 600, 1440}}});
  denseSweep(built, epochFromLocal(2026, 6, 1), epochFromLocal(2026, 6, 4), "C5");
  const auto state = check(built.schedule, utcForLocal(built.schedule, 2026, 6, 1, 23, 0), "C5");
  EXPECT_EQ(state.next.serviceMinute, 1440);  // first visited (06-01) kept
  EXPECT_EQ(state.next.local.day, 2);
  EXPECT_EQ(state.following[0].serviceMinute, 600);  // the duplicate 06-02 00:00 was suppressed
}

// ---- C4: the fourth candidate exactly at the next service day's start ----

TEST(OnPointDepartureDiff, C4_FourthCandidateAtTheNextServiceDayBoundary) {
  // Now 21:00: candidates 21:40, 22:20, 23:00 and 24:00 (= next day 00:00, minute 1440). The next
  // day also departs at minute 0, the same instant: no early exit, and the duplicate is dropped.
  const Built equal("c4-equal", {2026, 1, 1}, {{ALL_DAYS, {0, 1300, 1340, 1380, 1440}}});
  denseSweep(equal, epochFromLocal(2026, 7, 1), epochFromLocal(2026, 7, 4), "C4 equal");
  auto state = check(equal.schedule, utcForLocal(equal.schedule, 2026, 7, 1, 21, 0), "C4 equal");
  ASSERT_EQ(state.followingCount, 3u);
  EXPECT_EQ(state.following[2].serviceMinute, 1440);
  EXPECT_EQ(state.following[2].local.day, 2);

  // The fourth one minute before midnight: the scan may stop at the next day.
  const Built before("c4-before", {2026, 1, 1}, {{ALL_DAYS, {0, 1300, 1340, 1380, 1439}}});
  denseSweep(before, epochFromLocal(2026, 7, 1), epochFromLocal(2026, 7, 4), "C4 before");
  state = check(before.schedule, utcForLocal(before.schedule, 2026, 7, 1, 21, 0), "C4 before");
  EXPECT_EQ(state.following[2].serviceMinute, 1439);
}

// ---- C6: no future service at the 370-day search boundary ----

TEST(OnPointDepartureDiff, C6_SearchHorizon369And370) {
  const int64_t now = epochFromLocal(2026, 1, 1, 12);
  const auto dayAfter = [&](const int days) {
    const auto t = departure_diff::civilFromEpoch(epochFromLocal(2026, 1, 1) + days * 86400LL);
    return on_point::CivilDate{t.year, t.month, t.day};
  };
  const Built inside("c6-369", dayAfter(369), {{ALL_DAYS, {600}}});
  const Built outside("c6-370", dayAfter(370), {{ALL_DAYS, {600}}});
  const Built insideLate("c6-369-late", dayAfter(369), {{ALL_DAYS, {1500}}});
  const Built outsideLate("c6-370-late", dayAfter(370), {{ALL_DAYS, {1500}}});

  const auto at = departure_diff::civilFromEpoch(now - MANILA);
  EXPECT_EQ(check(inside.schedule, at, "C6 369").serviceStatus, ServiceStatus::NoServiceToday);
  EXPECT_TRUE(check(inside.schedule, at, "C6 369").next.valid);
  EXPECT_EQ(check(inside.schedule, at, "C6 exactly one").followingCount, 0u);  // day 370 isn't searched
  const Built two("c6-369-two", dayAfter(369), {{ALL_DAYS, {600, 660}}});
  EXPECT_EQ(check(two.schedule, at, "C6 fewer than four").followingCount, 1u);
  EXPECT_EQ(check(outside.schedule, at, "C6 370").serviceStatus, ServiceStatus::NoFutureService);
  EXPECT_TRUE(check(insideLate.schedule, at, "C6 369 late").next.valid);
  EXPECT_EQ(check(outsideLate.schedule, at, "C6 370 late").serviceStatus, ServiceStatus::NoFutureService);
  for (const Built* b : {&inside, &outside, &insideLate, &outsideLate})
    sweep(b->schedule, now - MANILA - 86400, now - MANILA + 86400, 601, "C6");
}

// ---- C7: invalid schedules and unchanged status fields ----

TEST(OnPointDepartureDiff, C7_InvalidSchedulesAndInstants) {
  const auto now = utcForLocal(Built("probe", {2026, 1, 1}, {{ALL_DAYS, {600}}}).schedule, 2026, 8, 1, 9);
  std::vector<std::unique_ptr<Built>> bad;
  bad.push_back(std::make_unique<Built>("c7-bad-date", on_point::CivilDate{2026, 2, 30},
                                        std::vector<Built::Rule>{{ALL_DAYS, {600}}}));
  bad.push_back(std::make_unique<Built>("c7-no-rules", on_point::CivilDate{2026, 1, 1}, std::vector<Built::Rule>{}));
  bad.push_back(
      std::make_unique<Built>("c7-zero-mask", on_point::CivilDate{2026, 1, 1}, std::vector<Built::Rule>{{0, {600}}}));
  bad.push_back(std::make_unique<Built>("c7-overlap", on_point::CivilDate{2026, 1, 1},
                                        std::vector<Built::Rule>{{0x3F, {600}}, {0x41, {700}}}));
  bad.push_back(std::make_unique<Built>("c7-unsorted", on_point::CivilDate{2026, 1, 1},
                                        std::vector<Built::Rule>{{ALL_DAYS, {700, 600}}}));
  bad.push_back(std::make_unique<Built>("c7-repeated", on_point::CivilDate{2026, 1, 1},
                                        std::vector<Built::Rule>{{ALL_DAYS, {600, 600}}}));
  bad.push_back(std::make_unique<Built>("c7-2880", on_point::CivilDate{2026, 1, 1},
                                        std::vector<Built::Rule>{{ALL_DAYS, {600, 2880}}}));
  bad.push_back(std::make_unique<Built>("c7-empty-rule", on_point::CivilDate{2026, 1, 1},
                                        std::vector<Built::Rule>{{ALL_DAYS, {}}}));
  bad.push_back(std::make_unique<Built>("c7-dup-override", on_point::CivilDate{2026, 1, 1},
                                        std::vector<Built::Rule>{{ALL_DAYS, {600}}},
                                        std::vector<Built::Override>{{{2026, 8, 1}, {700}}, {{2026, 8, 1}, {}}}));
  bad.push_back(std::make_unique<Built>("c7-offset", on_point::CivilDate{2026, 1, 1},
                                        std::vector<Built::Rule>{{ALL_DAYS, {600}}}, std::vector<Built::Override>{},
                                        int16_t{900}));
  for (const auto& b : bad) EXPECT_EQ(check(b->schedule, now, "C7").serviceStatus, ServiceStatus::InvalidSchedule);

  // Field-level corruptions the builder can't express.
  const Built good("c7-good", {2026, 1, 1}, {{ALL_DAYS, {600}}}, {{{2026, 8, 2}, {}}});
  for (int field = 0; field < 7; ++field) {
    on_point::Schedule s = good.schedule;
    on_point::DateOverride corrupt[] = {{{2026, 8, 2}, {s.serviceRules[0].departures.minutes, 0}}};
    switch (field) {
      case 0:
        s.routeId = nullptr;
        break;
      case 1:
        s.routeName = "";
        break;
      case 2:
        s.origin = nullptr;
        break;
      case 3:
        s.destination = "";
        break;
      case 4:
        s.timezone = nullptr;
        break;
      case 5:
        s.dateOverrides = nullptr;
        break;  // count 1, no array
      case 6:
        s.dateOverrides = corrupt;
        break;  // an empty list with non-null minutes
    }
    EXPECT_EQ(check(s, now, "C7 field").serviceStatus, ServiceStatus::InvalidSchedule) << field;
  }
  // Invalid instants on a valid schedule.
  for (const on_point::CivilDateTime t :
       {on_point::CivilDateTime{2026, 13, 1, 0, 0, 0}, on_point::CivilDateTime{2026, 2, 29, 0, 0, 0},
        on_point::CivilDateTime{2026, 8, 1, 24, 0, 0}, on_point::CivilDateTime{1969, 12, 31, 23, 0, 0}})
    EXPECT_EQ(check(good.schedule, t, "C7 instant").serviceStatus, ServiceStatus::InvalidSchedule);
}

// ---- The rest of the boundary matrix ----

TEST(OnPointDepartureDiff, DepartureMinuteDayEdgesAndWeekdayGaps) {
  // Weekdays only (Mon-Fri), with a last trip and an interval change.
  const Built weekdays("weekdays", {2026, 1, 1}, {{0x3E, {300, 330, 400, 1380}}});
  denseSweep(weekdays, epochFromLocal(2026, 9, 3), epochFromLocal(2026, 9, 9), "weekdays");  // Thu..Tue
  auto state = check(weekdays.schedule, utcForLocal(weekdays.schedule, 2026, 9, 4, 5, 30, 0), "on the minute");
  EXPECT_EQ(state.minutesToNext, 0);
  EXPECT_EQ(state.intervalMinutes, 30);
  state = check(weekdays.schedule, utcForLocal(weekdays.schedule, 2026, 9, 4, 23, 0, 30), "after last");
  EXPECT_EQ(state.serviceStatus, ServiceStatus::ServiceEnded);
  EXPECT_EQ(state.next.local.day, 7);  // Monday
}

TEST(OnPointDepartureDiff, SparseFuturesAndCalendarEdges) {
  // A single future trip (an override after the rules stop applying via empty overrides).
  const Built sparse("sparse", {2027, 12, 30}, {{ALL_DAYS, {700}}},
                     {{{2027, 12, 31}, {}}, {{2028, 1, 1}, {1439}}, {{2028, 2, 29}, {10, 1450}}});
  denseSweep(sparse, epochFromLocal(2027, 12, 29), epochFromLocal(2028, 1, 3), "year boundary");
  denseSweep(sparse, epochFromLocal(2028, 2, 27), epochFromLocal(2028, 3, 3), "leap day");
  const auto state = check(sparse.schedule, utcForLocal(sparse.schedule, 2027, 12, 31, 12), "sparse");
  EXPECT_EQ(state.next.local.year, 2028);
  EXPECT_EQ(state.next.serviceMinute, 1439);
}

TEST(OnPointDepartureDiff, OtherUtcOffsets) {
  for (const int16_t offset : {int16_t{-720}, int16_t{-300}, int16_t{0}, int16_t{345}, int16_t{840}}) {
    const Built built("offset", {2026, 1, 1}, {{ALL_DAYS, {0, 720, 1440, 2879}}}, {{{2026, 10, 2}, {}}}, offset);
    const int64_t from = epochFromLocal(2026, 9, 30);
    sweep(built.schedule, from, from + 4 * 86400, 59, "offset");
  }
}

}  // namespace
