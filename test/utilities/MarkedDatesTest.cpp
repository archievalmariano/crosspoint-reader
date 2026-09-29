#include <gtest/gtest.h>

#include <cstring>

#include "util/CivilCalendar.h"
#include "util/MarkedDates.h"

namespace {

using marked_dates::Kind;
using marked_dates::Mark;
using marked_dates::MarkedDate;

template <size_t N>
void expectValidAndSorted(const std::array<MarkedDate, N>& dates, const bool annual) {
  for (size_t i = 0; i < N; ++i) {
    const MarkedDate& date = dates[i];
    ASSERT_NE(date.name, nullptr);
    EXPECT_GT(std::strlen(date.name), 0u);
    EXPECT_EQ(date.year == marked_dates::EVERY_YEAR, annual) << date.name;
    const int year = annual ? 2028 : date.year;  // a leap year admits Feb 29
    EXPECT_TRUE(civil_cal::isValid({static_cast<int16_t>(year), date.month, date.day})) << date.name;
    if (i > 0) {
      const MarkedDate& previous = dates[i - 1];
      EXPECT_LE(previous.month * 32 + previous.day, date.month * 32 + date.day) << date.name;
    }
  }
}

TEST(MarkedDates, OfficialDatesAreValidAndSorted) {
  expectValidAndSorted(marked_dates::PHILIPPINES_2026, false);
  for (const MarkedDate& date : marked_dates::PHILIPPINES_2026) EXPECT_TRUE(marked_dates::isOfficial(date.kind));
}

TEST(MarkedDates, ObservancesAreAnnualValidAndSorted) {
  expectValidAndSorted(marked_dates::OBSERVANCES, true);
  for (const MarkedDate& date : marked_dates::OBSERVANCES) {
    EXPECT_FALSE(marked_dates::isOfficial(date.kind)) << date.name;
    EXPECT_FALSE(marked_dates::isNonWorking(date.kind)) << date.name;
  }
}

// Curation target: a small set, mostly international days.
TEST(MarkedDates, ObservanceTiersStaySmall) {
  size_t international = 0, cultural = 0;
  for (const MarkedDate& date : marked_dates::OBSERVANCES) {
    international += date.kind == Kind::InternationalObservance ? 1 : 0;
    cultural += date.kind == Kind::CulturalObservance ? 1 : 0;
  }
  EXPECT_EQ(international, 8u);
  EXPECT_EQ(cultural, 5u);
}

TEST(MarkedDates, ObservancesRepeatEveryYear) {
  EXPECT_EQ(marked_dates::markOn(2026, 4, 23), Mark::Observance);
  EXPECT_EQ(marked_dates::markOn(2031, 4, 23), Mark::Observance);
  const MarkedDate* found[4] = {};
  ASSERT_EQ(marked_dates::datesOn(2030, 11, 11, found, 4), 1u);
  EXPECT_EQ(found[0]->kind, Kind::CulturalObservance);
}

TEST(MarkedDates, Proclamation1006Counts) {
  size_t regular = 0, specialNonWorking = 0, specialWorking = 0;
  for (const MarkedDate& date : marked_dates::PHILIPPINES_2026) {
    regular += date.kind == Kind::RegularHoliday ? 1 : 0;
    specialNonWorking += date.kind == Kind::SpecialNonWorking ? 1 : 0;
    specialWorking += date.kind == Kind::SpecialWorking ? 1 : 0;
  }
  EXPECT_EQ(regular, 10u);
  EXPECT_EQ(specialNonWorking, 11u);  // 8 nationwide + 3 NCR ASEAN Summit days
  EXPECT_EQ(specialWorking, 1u);      // EDSA anniversary
  EXPECT_EQ(marked_dates::countNonWorking(marked_dates::PHILIPPINES_2026), 21u);
}

TEST(MarkedDates, LookupByDate) {
  const MarkedDate* found[4];
  ASSERT_EQ(marked_dates::datesOn(2026, 12, 25, found, 4), 1u);
  EXPECT_STREQ(found[0]->name, "Christmas Day");
  EXPECT_EQ(found[0]->kind, Kind::RegularHoliday);
  EXPECT_EQ(marked_dates::markOn(2026, 12, 25), Mark::Official);
  EXPECT_EQ(marked_dates::markOn(2026, 2, 25), Mark::Official);  // special working day is still official
  EXPECT_EQ(marked_dates::datesOn(2026, 9, 29, found, 4), 0u);
  EXPECT_EQ(marked_dates::markOn(2026, 9, 29), Mark::None);
}

TEST(MarkedDates, OfficialDatesBelongToTheirYear) {
  // 2027 has not been proclaimed: no official mark, only annual observances.
  const MarkedDate* found[4];
  const size_t count = marked_dates::datesOn(2027, 12, 25, found, 4);
  for (size_t i = 0; i < count; ++i) EXPECT_FALSE(marked_dates::isOfficial(found[i]->kind)) << found[i]->name;
  EXPECT_NE(marked_dates::markOn(2027, 12, 25), Mark::Official);
}

TEST(MarkedDates, LookupRespectsMax) {
  const MarkedDate* found[1];
  EXPECT_EQ(marked_dates::datesOn(2026, 11, 17, found, 1), 1u);
  EXPECT_EQ(marked_dates::datesOn(2026, 11, 17, found, 0), 0u);
}

}  // namespace
