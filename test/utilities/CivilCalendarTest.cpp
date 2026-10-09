#include <gtest/gtest.h>

#include "util/CivilCalendar.h"

namespace {

TEST(CivilCalendar, LeapYears) {
  EXPECT_TRUE(civil_cal::isLeapYear(2024));
  EXPECT_FALSE(civil_cal::isLeapYear(2026));
  EXPECT_FALSE(civil_cal::isLeapYear(2100));
  EXPECT_TRUE(civil_cal::isLeapYear(2000));
}

TEST(CivilCalendar, DaysInMonth) {
  EXPECT_EQ(civil_cal::daysInMonth(2026, 2), 28);
  EXPECT_EQ(civil_cal::daysInMonth(2028, 2), 29);
  EXPECT_EQ(civil_cal::daysInMonth(2026, 9), 30);
  EXPECT_EQ(civil_cal::daysInMonth(2026, 12), 31);
  EXPECT_EQ(civil_cal::daysInMonth(2026, 13), 0);
}

TEST(CivilCalendar, Weekdays) {
  EXPECT_EQ(civil_cal::weekday(1970, 1, 1), 4);    // Thursday
  EXPECT_EQ(civil_cal::weekday(2026, 9, 29), 2);   // Tuesday
  EXPECT_EQ(civil_cal::weekday(2026, 11, 1), 0);   // Sunday
  EXPECT_EQ(civil_cal::weekday(2000, 2, 29), 2);   // Tuesday
  EXPECT_EQ(civil_cal::weekday(1969, 12, 31), 3);  // Wednesday, before the epoch
}

TEST(CivilCalendar, DaysRoundTrip) {
  for (int32_t days = -800000; days <= 800000; days += 997) {
    const civil_cal::Date date = civil_cal::civilFromDays(days);
    EXPECT_TRUE(civil_cal::isValid(date)) << days;
    EXPECT_EQ(civil_cal::daysFromCivil(date.year, date.month, date.day), days);
  }
}

TEST(CivilCalendar, AddMonthsWrapsYears) {
  int16_t year = 2026;
  uint8_t month = 12;
  civil_cal::addMonths(year, month, 1);
  EXPECT_EQ(year, 2027);
  EXPECT_EQ(month, 1);
  civil_cal::addMonths(year, month, -1);
  EXPECT_EQ(year, 2026);
  EXPECT_EQ(month, 12);
  civil_cal::addMonths(year, month, -24);
  EXPECT_EQ(year, 2024);
  EXPECT_EQ(month, 12);
}

TEST(CivilCalendar, MonthGrid) {
  // September 2026 starts on a Tuesday and fits in 5 rows.
  EXPECT_EQ(civil_cal::leadingBlankDays(2026, 9), 2);
  EXPECT_EQ(civil_cal::weekRows(2026, 9), 5);
  // February 2026 starts on a Sunday: exactly 4 rows.
  EXPECT_EQ(civil_cal::weekRows(2026, 2), 4);
  // August 2026 starts on a Saturday: 6 rows.
  EXPECT_EQ(civil_cal::weekRows(2026, 8), 6);
}

}  // namespace
