#pragma once

#include <cstdint>

// Pure proleptic-Gregorian date arithmetic (no device deps, host-testable).
// Weeks start on Sunday: weekday 0 = Sunday ... 6 = Saturday.
namespace civil_cal {

struct Date {
  int16_t year = 2000;
  uint8_t month = 1;
  uint8_t day = 1;
};

struct DateTime {
  Date date;
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint8_t second = 0;
  uint8_t weekday = 0;
};

constexpr bool isLeapYear(const int year) { return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0; }

constexpr uint8_t daysInMonth(const int year, const int month) {
  constexpr uint8_t DAYS[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) return 0;
  return month == 2 && isLeapYear(year) ? 29 : DAYS[month - 1];
}

constexpr bool isValid(const Date& date) {
  return date.month >= 1 && date.month <= 12 && date.day >= 1 && date.day <= daysInMonth(date.year, date.month);
}

// Days since 1970-01-01 (Howard Hinnant's days_from_civil).
constexpr int32_t daysFromCivil(int year, const int month, const int day) {
  year -= month <= 2 ? 1 : 0;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const int yoe = year - era * 400;
  const int doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

constexpr Date civilFromDays(int32_t days) {
  days += 719468;
  const int32_t era = (days >= 0 ? days : days - 146096) / 146097;
  const int32_t doe = days - era * 146097;
  const int32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  const int32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const int32_t mp = (5 * doy + 2) / 153;
  const int32_t day = doy - (153 * mp + 2) / 5 + 1;
  const int32_t month = mp < 10 ? mp + 3 : mp - 9;
  const int32_t year = yoe + era * 400 + (month <= 2 ? 1 : 0);
  return Date{static_cast<int16_t>(year), static_cast<uint8_t>(month), static_cast<uint8_t>(day)};
}

constexpr uint8_t weekday(const int year, const int month, const int day) {
  const int32_t days = daysFromCivil(year, month, day);  // 1970-01-01 was a Thursday (4).
  return static_cast<uint8_t>(days >= -4 ? (days + 4) % 7 : (days + 5) % 7 + 6);
}

// Moves year/month by `delta` months, keeping month in 1..12.
constexpr void addMonths(int16_t& year, uint8_t& month, const int delta) {
  const int index = year * 12 + (month - 1) + delta;
  const int newYear = index >= 0 ? index / 12 : (index - 11) / 12;
  year = static_cast<int16_t>(newYear);
  month = static_cast<uint8_t>(index - newYear * 12 + 1);
}

// Month grid with weeks starting on Sunday: blank cells before day 1, and the
// number of week rows the month occupies (4 to 6).
constexpr uint8_t leadingBlankDays(const int year, const int month) { return weekday(year, month, 1); }
constexpr uint8_t weekRows(const int year, const int month) {
  return static_cast<uint8_t>((leadingBlankDays(year, month) + daysInMonth(year, month) + 6) / 7);
}

// Local wall-clock time for a UTC date/time shifted by `offsetMinutes`.
constexpr DateTime shift(const Date& utcDate, const int hour, const int minute, const int second,
                         const int offsetMinutes) {
  const int32_t total = hour * 60 + minute + offsetMinutes;
  const int32_t dayShift = total >= 0 ? total / 1440 : (total - 1439) / 1440;  // floor division
  const int32_t minutes = total - dayShift * 1440;
  DateTime local;
  local.date = civilFromDays(daysFromCivil(utcDate.year, utcDate.month, utcDate.day) + dayShift);
  local.hour = static_cast<uint8_t>(minutes / 60);
  local.minute = static_cast<uint8_t>(minutes % 60);
  local.second = static_cast<uint8_t>(second);
  local.weekday = weekday(local.date.year, local.date.month, local.date.day);
  return local;
}

// UTC offset in minutes from CrossPointSettings::clockUtcOffsetQ (quarter
// hours, biased by 48 so 48 = UTC+0).
constexpr int offsetMinutesFromBiasedQuarters(const uint8_t biasedQuarters) {
  return (static_cast<int>(biasedQuarters) - 48) * 15;
}

}  // namespace civil_cal
