#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

// The device's short date, "Wed 30 Sep": abbreviated weekday, day of month,
// abbreviated month, no year. The month is always a word, so the date reads
// the same whichever day/month order the reader is used to. Pure and
// host-testable; callers pass the (translated) abbreviations.
namespace short_date {

// Longest output: two 3-letter words, a 2-digit day, two spaces, terminator.
// Translations may use longer abbreviations, so callers should size generously.
constexpr size_t MIN_BUFFER = 12;

// Writes the short date into buf (always terminated when size > 0). Returns
// false, leaving buf empty, when an input is missing or the day is out of range.
inline bool format(char* buf, const size_t size, const char* weekdayAbbr, const uint8_t day, const char* monthAbbr) {
  if (buf == nullptr || size == 0) return false;
  buf[0] = '\0';
  if (weekdayAbbr == nullptr || monthAbbr == nullptr || weekdayAbbr[0] == '\0' || monthAbbr[0] == '\0' || day < 1 ||
      day > 31) {
    return false;
  }
  const int written = snprintf(buf, size, "%s %u %s", weekdayAbbr, static_cast<unsigned>(day), monthAbbr);
  if (written < 0 || static_cast<size_t>(written) >= size) {
    buf[0] = '\0';  // never show a cut-off date
    return false;
  }
  return true;
}

}  // namespace short_date
