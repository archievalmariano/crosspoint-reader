#pragma once

#include <cstddef>
#include <cstdint>

#include "util/CivilCalendar.h"

// Shared by the Calendar and Clock utilities, and the system date in headers
// and the reader status bar.
namespace utility_dates {

// The local date and time from the RTC, only when it can be trusted: the board
// has an RTC, it has been synced at least once, and the year is plausible.
// Boards without an RTC (X4) always return false, so they never show a
// "today" or a time.
bool trustedLocalNow(civil_cal::DateTime& out);

// The UTC offset used for local time, from Settings (minutes).
int localOffsetMinutes();

const char* monthName(uint8_t month);           // 1..12
const char* weekdayName(uint8_t weekday);       // 0 = Sunday
const char* weekdayShortName(uint8_t weekday);  // 0 = Sunday, uppercase grid label ("SUN")

// Today's short date ("Wed 30 Sep", see util/ShortDate.h) when the local date
// can be trusted (trustedLocalNow); false, with buf empty, otherwise.
bool trustedShortDate(char* buf, size_t size);

}  // namespace utility_dates
