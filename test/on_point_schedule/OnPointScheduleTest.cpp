#include <gtest/gtest.h>

#include <cstddef>

#include "activities/on_point/OnPointSchedule.h"
#include "activities/on_point/OnPointScheduleData.h"

namespace {

using on_point::CivilDateTime;
using on_point::DepartureState;
using on_point::ServiceStatus;

// All fixture dates in this suite are away from a month boundary.
CivilDateTime manilaTime(const int year, const int month, const int day, const int hour, const int minute,
                         const int second = 0) {
  if (hour >= 8)
    return {static_cast<int16_t>(year),     static_cast<uint8_t>(month),  static_cast<uint8_t>(day),
            static_cast<uint8_t>(hour - 8), static_cast<uint8_t>(minute), static_cast<uint8_t>(second)};
  return {static_cast<int16_t>(year),      static_cast<uint8_t>(month),  static_cast<uint8_t>(day - 1),
          static_cast<uint8_t>(hour + 16), static_cast<uint8_t>(minute), static_cast<uint8_t>(second)};
}

// Engine tests run against this fixed weekday timetable (every 30 minutes,
// 04:30-20:00 out, 06:00-22:30 back) so they never depend on catalog data.
constexpr uint8_t FIXTURE_WEEKDAYS = 0b0111110;
constexpr uint16_t FIXTURE_OUTBOUND[] = {270, 300, 330, 360,  390,  420,  450,  480,  510,  540, 570,
                                         600, 630, 660, 690,  720,  750,  780,  810,  840,  870, 900,
                                         930, 960, 990, 1020, 1050, 1080, 1110, 1140, 1170, 1200};
constexpr uint16_t FIXTURE_RETURN[] = {360,  390,  420,  450,  480,  510,  540,  570,  600,  630, 660,  690,
                                       720,  750,  780,  810,  840,  870,  900,  930,  960,  990, 1020, 1050,
                                       1080, 1110, 1140, 1170, 1200, 1230, 1260, 1290, 1320, 1350};
constexpr on_point::ServiceRule FIXTURE_OUTBOUND_RULES[] = {{FIXTURE_WEEKDAYS, {FIXTURE_OUTBOUND, 32}}};
constexpr on_point::ServiceRule FIXTURE_RETURN_RULES[] = {{FIXTURE_WEEKDAYS, {FIXTURE_RETURN, 34}}};

const on_point::Schedule& balagtasSchedule() {
  static const on_point::Schedule schedule = {"fixture-out",
                                              "Fixture",
                                              "A",
                                              "B",
                                              "Asia/Manila",
                                              480,
                                              {2026, 9, 1},
                                              FIXTURE_OUTBOUND_RULES,
                                              1,
                                              nullptr,
                                              0,
                                              "Test",
                                              on_point::SourceStatus::Provisional};
  return schedule;
}
const on_point::Schedule& trinomaSchedule() {
  static const on_point::Schedule schedule = {"fixture-back",
                                              "Fixture",
                                              "B",
                                              "A",
                                              "Asia/Manila",
                                              480,
                                              {2026, 9, 1},
                                              FIXTURE_RETURN_RULES,
                                              1,
                                              nullptr,
                                              0,
                                              "Test",
                                              on_point::SourceStatus::Provisional};
  return schedule;
}

TEST(OnPointScheduleData, RoutePairGroupsBothDirections) {
  const on_point::RoutePair& route = on_point::routeAt(0);
  const on_point::Schedule& outbound = on_point::scheduleForRoute(0, false);
  const on_point::Schedule& returning = on_point::scheduleForRoute(0, true);

  EXPECT_EQ(route.outboundScheduleIndex, 0u);
  EXPECT_EQ(route.returnScheduleIndex, 1u);
  EXPECT_STREQ(outbound.origin, "BALAGTAS");
  EXPECT_STREQ(outbound.destination, "TRINOMA");
  EXPECT_STREQ(returning.origin, outbound.destination);
  EXPECT_STREQ(returning.destination, outbound.origin);
  EXPECT_STREQ(route.originArea, "BULACAN");
  EXPECT_STREQ(route.destinationArea, "QUEZON CITY");
}

TEST(OnPointScheduleData, EveryRoutePairIsValidAndMirrored) {
  ASSERT_EQ(on_point::ROUTE_COUNT, 7u);
  ASSERT_EQ(on_point::routeCount(), on_point::ROUTE_COUNT);
  for (size_t index = 0; index < on_point::ROUTE_COUNT; ++index) {
    const on_point::Schedule& outbound = on_point::scheduleForRoute(index, false);
    const on_point::Schedule& returning = on_point::scheduleForRoute(index, true);
    EXPECT_TRUE(on_point::isValidSchedule(outbound)) << outbound.routeId;
    EXPECT_TRUE(on_point::isValidSchedule(returning)) << returning.routeId;
    EXPECT_STREQ(returning.origin, outbound.destination) << outbound.routeId;
    EXPECT_STREQ(returning.destination, outbound.origin) << outbound.routeId;
  }
}

TEST(OnPointScheduleData, BalagtasTrinomaHasWeekdayAndWeekendTimetables) {
  EXPECT_EQ(on_point::scheduleForRoute(0, false).sourceStatus, on_point::SourceStatus::Verified);

  const DepartureState friday =
      on_point::getDepartureState(on_point::scheduleForRoute(0, false), manilaTime(2026, 9, 25, 4, 0));
  EXPECT_EQ(friday.next.local.hour, 4);
  EXPECT_EQ(friday.next.local.minute, 30);

  const DepartureState saturday =
      on_point::getDepartureState(on_point::scheduleForRoute(0, false), manilaTime(2026, 9, 26, 4, 0));
  EXPECT_EQ(saturday.next.local.hour, 5);
  EXPECT_EQ(saturday.next.local.minute, 0);
  EXPECT_EQ(saturday.last.local.hour, 20);

  const DepartureState sundayReturn =
      on_point::getDepartureState(on_point::scheduleForRoute(0, true), manilaTime(2026, 9, 27, 6, 0));
  EXPECT_EQ(sundayReturn.next.local.hour, 6);
  EXPECT_EQ(sundayReturn.next.local.minute, 30);
  EXPECT_EQ(sundayReturn.last.local.hour, 22);
  EXPECT_EQ(sundayReturn.last.local.minute, 30);

  const DepartureState mondayReturn =
      on_point::getDepartureState(on_point::scheduleForRoute(0, true), manilaTime(2026, 9, 28, 5, 50));
  EXPECT_EQ(mondayReturn.next.local.hour, 6);
  EXPECT_EQ(mondayReturn.next.local.minute, 0);
}

TEST(OnPointScheduleData, CaypomboRunsDailyEveryThirtyMinutes) {
  EXPECT_EQ(on_point::scheduleForRoute(1, false).sourceStatus, on_point::SourceStatus::Verified);
  const DepartureState sunday =
      on_point::getDepartureState(on_point::scheduleForRoute(1, false), manilaTime(2026, 9, 27, 4, 50));
  EXPECT_EQ(sunday.next.local.hour, 5);
  EXPECT_EQ(sunday.next.local.minute, 0);
  EXPECT_EQ(sunday.last.local.hour, 20);
  EXPECT_EQ(sunday.last.local.minute, 0);

  const DepartureState weekdayReturn =
      on_point::getDepartureState(on_point::scheduleForRoute(1, true), manilaTime(2026, 9, 28, 22, 10));
  EXPECT_EQ(weekdayReturn.next.local.hour, 22);
  EXPECT_EQ(weekdayReturn.next.local.minute, 30);
  EXPECT_TRUE(weekdayReturn.isLastDeparture);
}

TEST(OnPointScheduleData, GenesisRoutesFollowTheOperatorSchedule) {
  const on_point::Schedule& clarkToNaia = on_point::scheduleForRoute(3, false);
  EXPECT_EQ(clarkToNaia.sourceStatus, on_point::SourceStatus::Verified);
  const DepartureState clark = on_point::getDepartureState(clarkToNaia, manilaTime(2026, 9, 26, 18, 20));
  EXPECT_EQ(clark.next.local.hour, 20);
  EXPECT_EQ(clark.next.local.minute, 0);
  EXPECT_EQ(clark.last.local.hour, 22);
  EXPECT_EQ(clark.last.local.minute, 30);

  const DepartureState naia =
      on_point::getDepartureState(on_point::scheduleForRoute(3, true), manilaTime(2026, 9, 26, 0, 0));
  EXPECT_EQ(naia.next.local.hour, 0);
  EXPECT_EQ(naia.minutesToNext, 0);

  const DepartureState trinoma =
      on_point::getDepartureState(on_point::scheduleForRoute(4, false), manilaTime(2026, 9, 26, 21, 40));
  EXPECT_EQ(trinoma.next.local.hour, 23);
  EXPECT_EQ(trinoma.next.local.minute, 0);
  EXPECT_TRUE(trinoma.isLastDeparture);

  const DepartureState clarkToTrinoma =
      on_point::getDepartureState(on_point::scheduleForRoute(4, true), manilaTime(2026, 9, 26, 5, 0));
  EXPECT_EQ(clarkToTrinoma.next.local.hour, 5);
  EXPECT_EQ(clarkToTrinoma.next.local.minute, 15);
}

TEST(OnPointScheduleData, EveryScheduleComesFromItsOperator) {
  for (size_t index = 0; index < on_point::SCHEDULE_COUNT; ++index) {
    EXPECT_EQ(on_point::scheduleAt(index).sourceStatus, on_point::SourceStatus::Verified)
        << on_point::scheduleAt(index).routeId;
  }
}

TEST(OnPointScheduleData, BalagtasHolidaysUseTheWeekendTimetable) {
  // Bonifacio Day (Monday) and an NCR ASEAN Summit day (Tuesday) start at 5:00.
  const DepartureState bonifacio =
      on_point::getDepartureState(on_point::scheduleForRoute(0, false), manilaTime(2026, 11, 30, 4, 0));
  EXPECT_EQ(bonifacio.next.local.hour, 5);
  EXPECT_EQ(bonifacio.next.local.minute, 0);
  const DepartureState asean =
      on_point::getDepartureState(on_point::scheduleForRoute(0, true), manilaTime(2026, 11, 17, 6, 0));
  EXPECT_EQ(asean.next.local.hour, 6);
  EXPECT_EQ(asean.next.local.minute, 30);

  // An ordinary Tuesday keeps the weekday timetable.
  const DepartureState tuesday =
      on_point::getDepartureState(on_point::scheduleForRoute(0, false), manilaTime(2026, 11, 24, 4, 0));
  EXPECT_EQ(tuesday.next.local.hour, 4);
  EXPECT_EQ(tuesday.next.local.minute, 30);
}

TEST(OnPointScheduleData, NewOperatorTimetables) {
  const DepartureState cubao =
      on_point::getDepartureState(on_point::scheduleForRoute(2, false), manilaTime(2026, 9, 28, 12, 0));
  EXPECT_EQ(cubao.next.local.hour, 13);
  EXPECT_EQ(cubao.next.local.minute, 10);

  const DepartureState baguio =
      on_point::getDepartureState(on_point::scheduleForRoute(5, true), manilaTime(2026, 9, 28, 10, 0));
  EXPECT_EQ(baguio.next.local.hour, 10);
  EXPECT_EQ(baguio.next.local.minute, 15);

  const DepartureState naia =
      on_point::getDepartureState(on_point::scheduleForRoute(6, false), manilaTime(2026, 9, 28, 19, 0));
  EXPECT_EQ(naia.serviceStatus, ServiceStatus::ServiceEnded);

  const DepartureState pitx =
      on_point::getDepartureState(on_point::scheduleForRoute(6, true), manilaTime(2026, 9, 28, 12, 0));
  EXPECT_EQ(pitx.next.local.hour, 14);
  EXPECT_EQ(pitx.next.local.minute, 15);
}

TEST(OnPointScheduleData, RouteCatalogHasStableIdsAndAlphabeticalPresentationOrder) {
  EXPECT_EQ(on_point::routeIndexForId("p2p-balagtas-trinoma"), 0u);
  EXPECT_EQ(on_point::routeIndexForId("p2p-caypombo-sm-north-edsa"), 1u);
  EXPECT_EQ(on_point::routeIndexForId("p2p-cubao-clark-airport"), 2u);
  EXPECT_EQ(on_point::routeIndexForId("p2p-clark-airport-naia-t3"), 3u);
  EXPECT_EQ(on_point::routeIndexForId("p2p-trinoma-clark-airport"), 4u);
  EXPECT_EQ(on_point::routeIndexForId("p2p-clark-airport-baguio"), 5u);
  EXPECT_EQ(on_point::routeIndexForId("p2p-naia-pitx"), 6u);
  // Removed routes (and unknown ids) fall back to the first pair.
  EXPECT_EQ(on_point::routeIndexForId("p2p-araneta-city-naia"), 0u);
  EXPECT_EQ(on_point::routeIndexForId("missing-route"), 0u);

  const char* expected[][2] = {
      {"BALAGTAS", "TRINOMA"},      {"CAYPOMBO", "SM NORTH EDSA"}, {"CLARK AIRPORT", "BAGUIO"},
      {"CLARK AIRPORT", "NAIA T3"}, {"CUBAO", "CLARK AIRPORT"},    {"NAIA T3", "PITX"},
      {"TRINOMA", "CLARK AIRPORT"},
  };
  for (size_t position = 0; position < on_point::ROUTE_COUNT; ++position) {
    const size_t routeIndex = on_point::routeIndexInAlphabeticalOrder(position);
    EXPECT_STREQ(on_point::scheduleForRoute(routeIndex, false).origin, expected[position][0]);
    EXPECT_STREQ(on_point::scheduleForRoute(routeIndex, false).destination, expected[position][1]);
  }
}

TEST(OnPointScheduleData, InvalidRouteIndexFallsBackToFirstPair) {
  EXPECT_EQ(&on_point::routeAt(on_point::ROUTE_COUNT), &on_point::routeAt(0));
  EXPECT_EQ(&on_point::scheduleForRoute(on_point::ROUTE_COUNT, false), &on_point::scheduleAt(0));
}

TEST(OnPointSchedule, BeforeFirstDeparture) {
  const DepartureState state = on_point::getDepartureState(balagtasSchedule(), manilaTime(2026, 9, 16, 3, 50));
  EXPECT_EQ(state.serviceStatus, ServiceStatus::BeforeFirst);
  ASSERT_TRUE(state.next.valid);
  EXPECT_EQ(state.next.local.hour, 4);
  EXPECT_EQ(state.next.local.minute, 30);
  EXPECT_EQ(state.minutesToNext, 40);
}

TEST(OnPointSchedule, OneMinuteBeforeDeparture) {
  const DepartureState state = on_point::getDepartureState(balagtasSchedule(), manilaTime(2026, 9, 16, 4, 29));
  EXPECT_EQ(state.minutesToNext, 1);
  EXPECT_EQ(state.next.local.hour, 4);
  EXPECT_EQ(state.next.local.minute, 30);
}

TEST(OnPointSchedule, ExactDepartureIsCurrentForThatSecond) {
  const DepartureState state = on_point::getDepartureState(balagtasSchedule(), manilaTime(2026, 9, 16, 17, 30));
  EXPECT_EQ(state.serviceStatus, ServiceStatus::InService);
  EXPECT_EQ(state.minutesToNext, 0);
  EXPECT_EQ(state.next.local.hour, 17);
  EXPECT_EQ(state.next.local.minute, 30);
}

TEST(OnPointSchedule, RollsForwardOneSecondAfterExactDeparture) {
  const DepartureState state = on_point::getDepartureState(balagtasSchedule(), manilaTime(2026, 9, 16, 17, 30, 1));
  EXPECT_EQ(state.minutesToNext, 30);
  EXPECT_EQ(state.next.local.hour, 18);
  EXPECT_EQ(state.next.local.minute, 0);
}

TEST(OnPointSchedule, OneMinuteAfterDeparture) {
  const DepartureState state = on_point::getDepartureState(balagtasSchedule(), manilaTime(2026, 9, 16, 17, 31));
  EXPECT_EQ(state.minutesToNext, 29);
  EXPECT_EQ(state.next.local.hour, 18);
  EXPECT_EQ(state.next.local.minute, 0);
}

TEST(OnPointSchedule, MiddleOfServiceIncludesFollowingDepartures) {
  const DepartureState state = on_point::getDepartureState(balagtasSchedule(), manilaTime(2026, 9, 16, 17, 12));
  EXPECT_EQ(state.minutesToNext, 18);
  ASSERT_EQ(state.followingCount, 3u);
  EXPECT_EQ(state.following[0].local.hour, 18);
  EXPECT_EQ(state.following[0].local.minute, 0);
  EXPECT_EQ(state.following[1].local.hour, 18);
  EXPECT_EQ(state.following[1].local.minute, 30);
  EXPECT_EQ(state.following[2].local.hour, 19);
  EXPECT_EQ(state.following[2].local.minute, 0);
  EXPECT_EQ(state.first.local.hour, 4);
  EXPECT_EQ(state.first.local.minute, 30);
  EXPECT_EQ(state.last.local.hour, 20);
  EXPECT_EQ(state.minutesToLast, 168);
}

TEST(OnPointSchedule, CurrentDepartureCanBeLast) {
  const DepartureState state = on_point::getDepartureState(balagtasSchedule(), manilaTime(2026, 9, 16, 20, 0));
  EXPECT_EQ(state.serviceStatus, ServiceStatus::InService);
  EXPECT_EQ(state.minutesToNext, 0);
  EXPECT_TRUE(state.isLastDeparture);
  EXPECT_EQ(state.minutesToLast, 0);
}

TEST(OnPointSchedule, AfterFinalDepartureShowsNextService) {
  const DepartureState state = on_point::getDepartureState(balagtasSchedule(), manilaTime(2026, 9, 16, 20, 1));
  EXPECT_EQ(state.serviceStatus, ServiceStatus::ServiceEnded);
  ASSERT_TRUE(state.next.valid);
  EXPECT_EQ(state.next.local.day, 17);
  EXPECT_EQ(state.next.local.hour, 4);
  EXPECT_EQ(state.next.local.minute, 30);
}

TEST(OnPointSchedule, WeekendRollsToMonday) {
  const DepartureState state = on_point::getDepartureState(balagtasSchedule(), manilaTime(2026, 9, 18, 20, 1));
  EXPECT_EQ(state.serviceStatus, ServiceStatus::ServiceEnded);
  ASSERT_TRUE(state.next.valid);
  EXPECT_EQ(state.next.local.day, 21);
  EXPECT_EQ(state.next.local.hour, 4);
  EXPECT_EQ(state.next.local.minute, 30);
}

TEST(OnPointSchedule, WeekendHasNoServiceToday) {
  const DepartureState state = on_point::getDepartureState(balagtasSchedule(), manilaTime(2026, 9, 19, 12, 0));
  EXPECT_EQ(state.serviceStatus, ServiceStatus::NoServiceToday);
  ASSERT_TRUE(state.next.valid);
  EXPECT_EQ(state.next.local.day, 21);
  EXPECT_EQ(state.next.local.hour, 4);
  EXPECT_EQ(state.next.local.minute, 30);
}

TEST(OnPointSchedule, BothFixtureDirectionsRemainIndependent) {
  const DepartureState outbound = on_point::getDepartureState(balagtasSchedule(), manilaTime(2026, 9, 16, 5, 0));
  const DepartureState inbound = on_point::getDepartureState(trinomaSchedule(), manilaTime(2026, 9, 16, 5, 0));
  EXPECT_EQ(outbound.next.local.hour, 5);
  EXPECT_EQ(outbound.minutesToNext, 0);
  EXPECT_EQ(inbound.next.local.hour, 6);
  EXPECT_EQ(inbound.minutesToNext, 60);
}

TEST(OnPointSchedule, EmptyAndMalformedSchedulesFailClosed) {
  on_point::Schedule empty;
  EXPECT_EQ(on_point::getDepartureState(empty, manilaTime(2026, 9, 16, 12, 0)).serviceStatus,
            ServiceStatus::InvalidSchedule);

  static constexpr uint16_t unsorted[] = {600, 570};
  static constexpr on_point::ServiceRule rules[] = {{0b0111110, {unsorted, 2}}};
  const on_point::Schedule malformed = {"bad",
                                        "Bad",
                                        "A",
                                        "B",
                                        "Asia/Manila",
                                        480,
                                        {2026, 9, 1},
                                        rules,
                                        1,
                                        nullptr,
                                        0,
                                        "Malformed",
                                        on_point::SourceStatus::Provisional};
  EXPECT_FALSE(on_point::isValidSchedule(malformed));
  EXPECT_EQ(on_point::getDepartureState(malformed, manilaTime(2026, 9, 16, 12, 0)).serviceStatus,
            ServiceStatus::InvalidSchedule);
}

TEST(OnPointSchedule, CrossMidnightServiceBelongsToPreviousServiceDay) {
  static constexpr uint16_t times[] = {23 * 60 + 30, 24 * 60 + 30};
  static constexpr on_point::ServiceRule rules[] = {{1u << 3, {times, 2}}};  // Wednesday service day.
  const on_point::Schedule schedule = {"night",
                                       "Night route",
                                       "A",
                                       "B",
                                       "Asia/Manila",
                                       480,
                                       {2026, 9, 1},
                                       rules,
                                       1,
                                       nullptr,
                                       0,
                                       "Test",
                                       on_point::SourceStatus::Verified};
  const DepartureState state = on_point::getDepartureState(schedule, manilaTime(2026, 9, 17, 0, 15));
  EXPECT_EQ(state.serviceStatus, ServiceStatus::InService);
  EXPECT_EQ(state.minutesToNext, 15);
  EXPECT_EQ(state.first.local.day, 16);
  EXPECT_EQ(state.first.local.hour, 23);
  EXPECT_EQ(state.last.local.day, 17);
  EXPECT_EQ(state.last.local.hour, 0);
  EXPECT_EQ(state.last.local.minute, 30);
}

}  // namespace
