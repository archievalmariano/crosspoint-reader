#include "UtilityDates.h"

#include <HalClock.h>
#include <I18n.h>

#include "CrossPointSettings.h"

namespace utility_dates {
namespace {
constexpr int16_t EARLIEST_TRUSTED_YEAR = 2025;

constexpr StrId MONTHS[12] = {
    StrId::STR_MONTH_JANUARY,   StrId::STR_MONTH_FEBRUARY, StrId::STR_MONTH_MARCH,    StrId::STR_MONTH_APRIL,
    StrId::STR_MONTH_MAY,       StrId::STR_MONTH_JUNE,     StrId::STR_MONTH_JULY,     StrId::STR_MONTH_AUGUST,
    StrId::STR_MONTH_SEPTEMBER, StrId::STR_MONTH_OCTOBER,  StrId::STR_MONTH_NOVEMBER, StrId::STR_MONTH_DECEMBER,
};
constexpr StrId WEEKDAYS[7] = {
    StrId::STR_WEEKDAY_SUNDAY,   StrId::STR_WEEKDAY_MONDAY, StrId::STR_WEEKDAY_TUESDAY,  StrId::STR_WEEKDAY_WEDNESDAY,
    StrId::STR_WEEKDAY_THURSDAY, StrId::STR_WEEKDAY_FRIDAY, StrId::STR_WEEKDAY_SATURDAY,
};
constexpr StrId WEEKDAYS_SHORT[7] = {
    StrId::STR_WEEKDAY_SHORT_SUNDAY,    StrId::STR_WEEKDAY_SHORT_MONDAY,   StrId::STR_WEEKDAY_SHORT_TUESDAY,
    StrId::STR_WEEKDAY_SHORT_WEDNESDAY, StrId::STR_WEEKDAY_SHORT_THURSDAY, StrId::STR_WEEKDAY_SHORT_FRIDAY,
    StrId::STR_WEEKDAY_SHORT_SATURDAY,
};
}  // namespace

int localOffsetMinutes() {
  uint8_t biased = SETTINGS.clockUtcOffsetQ;
  if (biased > 104) biased = 104;  // same clamp as HalClock::formatTime
  return civil_cal::offsetMinutesFromBiasedQuarters(biased);
}

bool trustedLocalNow(civil_cal::DateTime& out) {
  if (!halClock.isAvailable() || !SETTINGS.clockHasBeenSynced) return false;
  HalClock::DateTime utc;
  if (!halClock.getDateTime(utc, true)) return false;
  if (utc.year < EARLIEST_TRUSTED_YEAR) return false;
  out = civil_cal::shift({static_cast<int16_t>(utc.year), utc.month, utc.day}, utc.hour, utc.minute, utc.second,
                         localOffsetMinutes());
  return true;
}

const char* monthName(const uint8_t month) {
  return month >= 1 && month <= 12 ? I18n::getInstance().get(MONTHS[month - 1]) : "";
}
const char* weekdayName(const uint8_t weekday) { return weekday < 7 ? I18n::getInstance().get(WEEKDAYS[weekday]) : ""; }
const char* weekdayShortName(const uint8_t weekday) {
  return weekday < 7 ? I18n::getInstance().get(WEEKDAYS_SHORT[weekday]) : "";
}

}  // namespace utility_dates
