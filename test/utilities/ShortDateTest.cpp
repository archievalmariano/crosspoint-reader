#include <gtest/gtest.h>

#include "util/CivilCalendar.h"
#include "util/ShortDate.h"

namespace {

TEST(ShortDate, FormatsWeekdayDayMonth) {
  char buf[short_date::MIN_BUFFER];
  ASSERT_TRUE(short_date::format(buf, sizeof(buf), "Wed", 30, "Sep"));
  EXPECT_STREQ(buf, "Wed 30 Sep");
}

TEST(ShortDate, SingleDigitDayHasNoPadding) {
  char buf[short_date::MIN_BUFFER];
  ASSERT_TRUE(short_date::format(buf, sizeof(buf), "Thu", 1, "Oct"));
  EXPECT_STREQ(buf, "Thu 1 Oct");
}

TEST(ShortDate, TooSmallBufferLeavesItEmpty) {
  char buf[8] = "xxxxxxx";
  EXPECT_FALSE(short_date::format(buf, sizeof(buf), "Wed", 30, "Sep"));
  EXPECT_STREQ(buf, "");
}

TEST(ShortDate, RejectsMissingNamesAndBadDays) {
  char buf[short_date::MIN_BUFFER] = "x";
  EXPECT_FALSE(short_date::format(buf, sizeof(buf), nullptr, 30, "Sep"));
  EXPECT_STREQ(buf, "");
  EXPECT_FALSE(short_date::format(buf, sizeof(buf), "Wed", 30, ""));
  EXPECT_FALSE(short_date::format(buf, sizeof(buf), "Wed", 0, "Sep"));
  EXPECT_FALSE(short_date::format(buf, sizeof(buf), "Wed", 32, "Sep"));
  EXPECT_FALSE(short_date::format(nullptr, 12, "Wed", 30, "Sep"));
}

// The local date feeding the short date: 23:30 UTC on 30 Sep is already
// Thursday 1 Oct at UTC+8 (Manila), and still Wednesday 30 Sep at UTC-5.
TEST(ShortDate, LocalDateCrossesMidnightWithTheOffset) {
  const civil_cal::Date utc{2026, 9, 30};
  const auto manila = civil_cal::shift(utc, 23, 30, 0, 8 * 60);
  EXPECT_EQ(manila.date.month, 10);
  EXPECT_EQ(manila.date.day, 1);
  EXPECT_EQ(manila.weekday, 4);  // Thursday
  const auto newYork = civil_cal::shift(utc, 23, 30, 0, -5 * 60);
  EXPECT_EQ(newYork.date.month, 9);
  EXPECT_EQ(newYork.date.day, 30);
  EXPECT_EQ(newYork.weekday, 3);  // Wednesday
}

}  // namespace
