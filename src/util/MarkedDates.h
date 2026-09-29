#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

// Canonical marked dates (no device deps, host-testable): Philippine official
// holidays for specific years, and a short list of annual observances. The
// Calendar app shows them; ON POINT takes the non-working official days for
// its holiday timetables.
namespace marked_dates {

enum class Kind : uint8_t {
  RegularHoliday,           // official, non-working
  SpecialNonWorking,        // official, non-working
  SpecialWorking,           // official, but offices and schools stay open
  InternationalObservance,  // recognized by the UN, UNESCO or another intergovernmental body
  CulturalObservance,       // popular or cultural, with no institutional recognition
};

constexpr bool isOfficial(const Kind kind) {
  return kind == Kind::RegularHoliday || kind == Kind::SpecialNonWorking || kind == Kind::SpecialWorking;
}
constexpr bool isNonWorking(const Kind kind) { return kind == Kind::RegularHoliday || kind == Kind::SpecialNonWorking; }

// Observances repeat every year; official days are listed per year.
constexpr uint16_t EVERY_YEAR = 0;

struct MarkedDate {
  uint16_t year;  // EVERY_YEAR for annual dates
  uint8_t month;
  uint8_t day;
  Kind kind;
  const char* name;
  const char* note;  // optional one-line description, may be nullptr
};

// 2026: Proclamation No. 1006 (Official Gazette, nationwide holidays), plus the
// NCR special non-working days for the ASEAN Summit (November 16-18). Sorted by
// date. Add each year's list once it is proclaimed.
inline constexpr std::array<MarkedDate, 22> PHILIPPINES_2026 = {{
    {2026, 1, 1, Kind::RegularHoliday, "New Year's Day", nullptr},
    {2026, 2, 17, Kind::SpecialNonWorking, "Chinese New Year", nullptr},
    {2026, 2, 25, Kind::SpecialWorking, "EDSA People Power Anniversary", "Offices and schools stay open."},
    {2026, 4, 2, Kind::RegularHoliday, "Maundy Thursday", nullptr},
    {2026, 4, 3, Kind::RegularHoliday, "Good Friday", nullptr},
    {2026, 4, 4, Kind::SpecialNonWorking, "Black Saturday", nullptr},
    {2026, 4, 9, Kind::RegularHoliday, "Araw ng Kagitingan", "Day of Valor."},
    {2026, 5, 1, Kind::RegularHoliday, "Labor Day", nullptr},
    {2026, 6, 12, Kind::RegularHoliday, "Independence Day", nullptr},
    {2026, 8, 21, Kind::SpecialNonWorking, "Ninoy Aquino Day", nullptr},
    {2026, 8, 31, Kind::RegularHoliday, "National Heroes Day", "Last Monday of August."},
    {2026, 11, 1, Kind::SpecialNonWorking, "All Saints' Day", nullptr},
    {2026, 11, 2, Kind::SpecialNonWorking, "All Souls' Day", nullptr},
    {2026, 11, 16, Kind::SpecialNonWorking, "ASEAN Summit", "Metro Manila only."},
    {2026, 11, 17, Kind::SpecialNonWorking, "ASEAN Summit", "Metro Manila only."},
    {2026, 11, 18, Kind::SpecialNonWorking, "ASEAN Summit", "Metro Manila only."},
    {2026, 11, 30, Kind::RegularHoliday, "Bonifacio Day", nullptr},
    {2026, 12, 8, Kind::SpecialNonWorking, "Feast of the Immaculate Conception", nullptr},
    {2026, 12, 24, Kind::SpecialNonWorking, "Christmas Eve", nullptr},
    {2026, 12, 25, Kind::RegularHoliday, "Christmas Day", nullptr},
    {2026, 12, 30, Kind::RegularHoliday, "Rizal Day", nullptr},
    {2026, 12, 31, Kind::SpecialNonWorking, "Last Day of the Year", nullptr},
}};

// Annual observances, kept deliberately selective: a few international days
// (names as on the UN list of international days) and a small set of popular
// cultural dates, which make no claim to recognition. Sorted by date.
inline constexpr std::array<MarkedDate, 13> OBSERVANCES = {{
    {EVERY_YEAR, 1, 4, Kind::InternationalObservance, "World Braille Day",
     "UN day recognizing braille as a means of communication."},
    {EVERY_YEAR, 2, 14, Kind::CulturalObservance, "Valentine's Day", nullptr},
    {EVERY_YEAR, 3, 8, Kind::InternationalObservance, "International Women's Day",
     "UN day for women's rights and gender equality."},
    {EVERY_YEAR, 3, 14, Kind::CulturalObservance, "Pi Day", "Named for 3.14, the first digits of pi."},
    {EVERY_YEAR, 4, 22, Kind::InternationalObservance, "International Mother Earth Day", "Also known as Earth Day."},
    {EVERY_YEAR, 4, 23, Kind::InternationalObservance, "World Book and Copyright Day",
     "UNESCO day for books, reading and authors."},
    {EVERY_YEAR, 5, 4, Kind::CulturalObservance, "Star Wars Day", "From the pun \"May the Fourth be with you.\""},
    {EVERY_YEAR, 7, 20, Kind::InternationalObservance, "International Moon Day",
     "UN day marking the first Moon landing, in 1969."},
    {EVERY_YEAR, 9, 8, Kind::InternationalObservance, "International Literacy Day",
     "UNESCO day for literacy and learning."},
    {EVERY_YEAR, 10, 1, Kind::InternationalObservance, "International Coffee Day", nullptr},
    {EVERY_YEAR, 10, 5, Kind::InternationalObservance, "World Teachers' Day", "UNESCO day honoring teachers."},
    {EVERY_YEAR, 10, 31, Kind::CulturalObservance, "Halloween", nullptr},
    {EVERY_YEAR, 11, 11, Kind::CulturalObservance, "Singles' Day",
     "An informal celebration associated with being single, originating in China."},
}};

template <size_t N>
constexpr size_t countNonWorking(const std::array<MarkedDate, N>& dates) {
  return static_cast<size_t>(
      std::count_if(dates.begin(), dates.end(), [](const MarkedDate& date) { return isNonWorking(date.kind); }));
}

constexpr bool matches(const MarkedDate& date, const int year, const int month, const int day) {
  return (date.year == EVERY_YEAR || date.year == year) && date.month == month && date.day == day;
}

// Collects up to `max` entries for a date into `out`, official days first.
// Returns how many were written.
inline size_t datesOn(const int year, const int month, const int day, const MarkedDate** out, const size_t max) {
  size_t count = 0;
  for (const MarkedDate& date : PHILIPPINES_2026) {
    if (count < max && matches(date, year, month, day)) out[count++] = &date;
  }
  for (const MarkedDate& date : OBSERVANCES) {
    if (count < max && matches(date, year, month, day)) out[count++] = &date;
  }
  return count;
}

// How a Calendar cell is marked: official days take precedence.
enum class Mark : uint8_t { None, Official, Observance };

inline Mark markOn(const int year, const int month, const int day) {
  const MarkedDate* found[2] = {};
  const size_t count = datesOn(year, month, day, found, 2);
  if (count == 0) return Mark::None;
  return isOfficial(found[0]->kind) ? Mark::Official : Mark::Observance;
}

}  // namespace marked_dates
