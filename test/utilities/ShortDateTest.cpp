#include <gtest/gtest.h>

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

}  // namespace
