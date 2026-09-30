#include "UtilityDates.h"

#include <HalClock.h>
#include <I18n.h>

#include "CrossPointSettings.h"
#include "util/ShortDate.h"

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
constexpr StrId WEEKDAYS_ABBR[7] = {
    StrId::STR_WEEKDAY_ABBR_SUNDAY,    StrId::STR_WEEKDAY_ABBR_MONDAY,   StrId::STR_WEEKDAY_ABBR_TUESDAY,
    StrId::STR_WEEKDAY_ABBR_WEDNESDAY, StrId::STR_WEEKDAY_ABBR_THURSDAY, StrId::STR_WEEKDAY_ABBR_FRIDAY,
    StrId::STR_WEEKDAY_ABBR_SATURDAY,
};
constexpr StrId MONTHS_ABBR[12] = {
    StrId::STR_MONTH_ABBR_JANUARY, StrId::STR_MONTH_ABBR_FEBRUARY, StrId::STR_MONTH_ABBR_MARCH,
    StrId::STR_MONTH_ABBR_APRIL,   StrId::STR_MONTH_ABBR_MAY,      StrId::STR_MONTH_ABBR_JUNE,
    StrId::STR_MONTH_ABBR_JULY,    StrId::STR_MONTH_ABBR_AUGUST,   StrId::STR_MONTH_ABBR_SEPTEMBER,
    StrId::STR_MONTH_ABBR_OCTOBER, StrId::STR_MONTH_ABBR_NOVEMBER, StrId::STR_MONTH_ABBR_DECEMBER,
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

bool trustedShortDate(char* buf, const size_t size) {
  if (buf != nullptr && size > 0) buf[0] = '\0';
  civil_cal::DateTime now;
  if (!trustedLocalNow(now) || now.weekday > 6 || now.date.month < 1 || now.date.month > 12) return false;
  const auto& i18n = I18n::getInstance();
  return short_date::format(buf, size, i18n.get(WEEKDAYS_ABBR[now.weekday]), now.date.day,
                            i18n.get(MONTHS_ABBR[now.date.month - 1]));
}

}  // namespace utility_dates
