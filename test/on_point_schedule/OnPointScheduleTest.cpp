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

const on_point::Schedule& balagtasSchedule() { return on_point::scheduleAt(0); }
const on_point::Schedule& trinomaSchedule() { return on_point::scheduleAt(1); }

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
