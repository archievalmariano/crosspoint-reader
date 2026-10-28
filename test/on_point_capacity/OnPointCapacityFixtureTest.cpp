#include <gtest/gtest.h>

#include <chrono>
#include <cstring>
#include <vector>

#include "activities/on_point/OnPointRouteList.h"
#include "activities/on_point/OnPointSchedule.h"
#include "activities/on_point/OnPointScheduleData.h"
#include "test/fixtures/on_point/LegacyRouteList.h"

// The synthetic 60-route capacity fixture (TEST ONLY): its data is valid,
// unique and overnight-correct, and the route-list model handles it exactly
// like the v1.2.0 logic did.

namespace on_point {
extern size_t routeComparisonCount;
}

namespace {

using on_point::RouteListModel;
using on_point::RouteResolutionState;
using on_point::ServiceStatus;

constexpr size_t PAIRS = 60;
constexpr size_t SCHEDULES = 120;
constexpr int SCREEN_WIDTH = 480;

// Manila local time (UTC+8) as the UTC instant the schedule code expects.
on_point::CivilDateTime manila(const int year, const unsigned month, const unsigned day, const int hour,
                               const int minute) {
  using namespace std::chrono;
  const sys_time<minutes> local =
      sys_days{std::chrono::year{year} / std::chrono::month{month} / std::chrono::day{day}} + hours{hour} +
      minutes{minute};
  const sys_time<minutes> utc = local - hours{8};
  const year_month_day date{floor<days>(utc)};
  const auto time = hh_mm_ss<minutes>{utc - floor<days>(utc)};
  return {static_cast<int16_t>(static_cast<int>(date.year())),
          static_cast<uint8_t>(static_cast<unsigned>(date.month())),
          static_cast<uint8_t>(static_cast<unsigned>(date.day())),
          static_cast<uint8_t>(time.hours().count()),
          static_cast<uint8_t>(time.minutes().count()),
          0};
}

TEST(OnPointCapacityFixture, CatalogueShape) {
  ASSERT_EQ(on_point::routeCount(), PAIRS);
  EXPECT_LE(on_point::routeCount(), on_point::MAX_CATALOGUE_ROUTES);
  for (size_t k = 0; k < PAIRS; ++k) {
    EXPECT_EQ(strncmp(on_point::routeAt(k).routeId, "fixture-", 8), 0);
    EXPECT_LE(strlen(on_point::routeAt(k).routeId), 47u);
  }
}

TEST(OnPointCapacityFixture, EveryScheduleIsValid) {
  for (size_t s = 0; s < SCHEDULES; ++s) EXPECT_TRUE(on_point::isValidSchedule(on_point::scheduleAt(s))) << s;
}

TEST(OnPointCapacityFixture, EveryDepartureArrayIsUnique) {
  std::vector<const on_point::DepartureList*> lists;
  size_t total = 0;
  for (size_t s = 0; s < SCHEDULES; ++s) {
    const auto& schedule = on_point::scheduleAt(s);
    ASSERT_EQ(schedule.serviceRuleCount, 3u);
    for (size_t r = 0; r < 3; ++r) {
      lists.push_back(&schedule.serviceRules[r].departures);
      total += schedule.serviceRules[r].departures.count;
    }
  }
  EXPECT_EQ(lists.size(), 360u);
  EXPECT_EQ(total, 42960u);
  for (size_t a = 0; a < lists.size(); ++a) {
    for (size_t b = a + 1; b < lists.size(); ++b) {
      ASSERT_NE(lists[a]->minutes, lists[b]->minutes) << a << " " << b;  // distinct storage
      const bool sameContent = lists[a]->count == lists[b]->count &&
                               memcmp(lists[a]->minutes, lists[b]->minutes, lists[a]->count * sizeof(uint16_t)) == 0;
      ASSERT_FALSE(sameContent) << a << " " << b;
    }
  }
}

TEST(OnPointCapacityFixture, WeekdayTripsAfterMidnightBelongToTheWeekday) {
  // Fri 2026-10-23 01:00: Thursday's list runs to about 01:38.
  const auto state = on_point::getDepartureState(on_point::scheduleAt(0), manila(2026, 10, 23, 1, 0));
  EXPECT_EQ(state.serviceStatus, ServiceStatus::InService);
  ASSERT_TRUE(state.next.valid);
  EXPECT_GE(state.next.serviceMinute, 1440);
  EXPECT_EQ(state.next.local.day, 23);
}

TEST(OnPointCapacityFixture, SaturdayTripsAfterMidnightBelongToSaturday) {
  // Sun 2026-10-25 00:30: Saturday's list runs to about 00:58.
  const auto state = on_point::getDepartureState(on_point::scheduleAt(7), manila(2026, 10, 25, 0, 30));
  EXPECT_EQ(state.serviceStatus, ServiceStatus::InService);
  ASSERT_TRUE(state.next.valid);
  EXPECT_GE(state.next.serviceMinute, 1440);
}

TEST(OnPointCapacityFixture, SundayEndsBeforeMidnightAndMondayStartsAtFour) {
  // Mon 2026-10-26 00:30: Sunday's last trip was before midnight.
  const auto state = on_point::getDepartureState(on_point::scheduleAt(13), manila(2026, 10, 26, 0, 30));
  ASSERT_TRUE(state.next.valid);
  EXPECT_LT(state.next.serviceMinute, 1440);
  EXPECT_EQ(state.next.local.day, 26);
  EXPECT_EQ(state.next.local.hour, 4);
  EXPECT_TRUE(state.serviceStatus == ServiceStatus::BeforeFirst || state.serviceStatus == ServiceStatus::ServiceEnded)
      << static_cast<int>(state.serviceStatus);
}

TEST(OnPointCapacityFixture, EmptyOverrideIsANonServiceDay) {
  // Sun 2026-03-15 is an explicit non-service day.
  const auto state = on_point::getDepartureState(on_point::scheduleAt(42), manila(2026, 3, 15, 12, 0));
  EXPECT_EQ(state.serviceStatus, ServiceStatus::NoServiceToday);
  ASSERT_TRUE(state.next.valid);
  EXPECT_EQ(state.next.local.day, 16);
  EXPECT_EQ(state.next.local.hour, 4);
}

TEST(OnPointCapacityFixture, NonEmptyOverrideUsesTheSundayList) {
  // Wed 2026-04-01 at 05:00: the override's Sunday list starts at 06:0x, not 04:0x.
  const auto state = on_point::getDepartureState(on_point::scheduleAt(99), manila(2026, 4, 1, 5, 0));
  ASSERT_TRUE(state.next.valid);
  EXPECT_EQ(state.next.local.day, 1);
  EXPECT_EQ(state.next.local.hour, 6);
}

TEST(OnPointCapacityFixture, TieGroupsBreakOnTheRouteId) {
  // Pairs 0 and 1 share origin and destination; only the ID orders them.
  const auto& a = on_point::scheduleForRoute(0, false);
  const auto& b = on_point::scheduleForRoute(1, false);
  EXPECT_STREQ(a.origin, b.origin);
  EXPECT_STREQ(a.destination, b.destination);
  EXPECT_LT(on_point::compareRoutes(0, 1), 0);
  EXPECT_GT(on_point::compareRoutes(1, 0), 0);
  // Pair 2 shares only the origin.
  EXPECT_STREQ(a.origin, on_point::scheduleForRoute(2, false).origin);
  EXPECT_STRNE(a.destination, on_point::scheduleForRoute(2, false).destination);
}

TEST(OnPointCapacityFixture, ModelOrderMatchesTheV120Ranking) {
  RouteListModel model;
  model.build({RouteResolutionState::Unknown, 0});
  // No main route: the pinned slot is the first ranked route and the rest follow.
  for (size_t position = 0; position < PAIRS; ++position) {
    EXPECT_EQ(model.routeAtPosition(position), on_point::routeIndexInAlphabeticalOrder(position)) << position;
  }
}

TEST(OnPointCapacityFixture, ModelMatchesLegacyDisplayForSampledMains) {
  // Legacy display lookups are O(n^3) each, so sample mains: first, last,
  // middle, the first ranked route and both members of a tie group.
  for (const size_t main : {size_t{0}, size_t{1}, size_t{29}, size_t{59}, on_point::routeIndexInAlphabeticalOrder(0)}) {
    RouteListModel model;
    model.build({RouteResolutionState::Canonical, main});
    for (size_t d = 0; d < PAIRS; ++d) {
      EXPECT_EQ(model.routeAtPosition(d), legacy_route_list::displayRouteIndex(main, d)) << main << " " << d;
    }
  }
}

TEST(OnPointCapacityFixture, LastPageIsPartialAndItsEmptyRowIsNotATarget) {
  RouteListModel model;
  model.build({RouteResolutionState::Canonical, 0});
  const size_t last = PAIRS - 1;  // position on the last page
  const size_t start = model.pageStart(last);
  EXPECT_EQ(start, 57u);  // 59 other routes: pages of 3, the last holds 2
  EXPECT_EQ(model.visibleRows(start), 2u);
  size_t route = 0;
  EXPECT_TRUE(model.routeAtPoint(10, 316, SCREEN_WIDTH, last, route));   // row 1
  EXPECT_TRUE(model.routeAtPoint(10, 442, SCREEN_WIDTH, last, route));   // row 2
  EXPECT_FALSE(model.routeAtPoint(10, 568, SCREEN_WIDTH, last, route));  // no row 3
  size_t target = 0;
  EXPECT_FALSE(model.swipeTarget(last, true, target));
  EXPECT_TRUE(model.swipeTarget(last, false, target));
}

TEST(OnPointCapacityFixture, LookupsCostNoComparisonsAfterTheOneTimeSort) {
  RouteListModel model;
  on_point::routeComparisonCount = 0;
  model.build({RouteResolutionState::Canonical, 17});
  const size_t buildComparisons = on_point::routeComparisonCount;
  EXPECT_LE(buildComparisons, PAIRS * (PAIRS - 1) / 2);  // insertion sort worst case
  on_point::routeComparisonCount = 0;
  size_t sink = 0;
  for (size_t route = 0; route < PAIRS; ++route) {
    const size_t position = model.positionOfRoute(route);
    const size_t start = model.pageStart(position);
    sink +=
        position + start + model.visibleRows(start) + model.nextPosition(position) + model.routeAtPosition(position);
    size_t touched = 0;
    if (model.routeAtPoint(10, 330, SCREEN_WIDTH, position, touched)) sink += touched;
  }
  EXPECT_EQ(on_point::routeComparisonCount, 0u);
  EXPECT_GT(sink, 0u);
}

}  // namespace
