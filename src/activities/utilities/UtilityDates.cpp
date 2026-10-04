#include "UtilityDates.h"

#include <HalClock.h>
#include <I18n.h>

#include "CrossPointSettings.h"
#include "util/ShortDate.h"
#include "util/Timezones.h"

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

const char* zoneName() { return timezones::table()[timezones::activeIndex()].name; }

bool trustedLocalNow(civil_cal::DateTime& out) {
  if (!halClock.isAvailable() || !SETTINGS.clockHasBeenSynced) return false;
  struct tm local;
  if (!halClock.localTime(local, true)) return false;
  const int year = local.tm_year + 1900;
  if (year < EARLIEST_TRUSTED_YEAR) return false;
  out.date = {static_cast<int16_t>(year), static_cast<uint8_t>(local.tm_mon + 1), static_cast<uint8_t>(local.tm_mday)};
  out.hour = static_cast<uint8_t>(local.tm_hour);
  out.minute = static_cast<uint8_t>(local.tm_min);
  out.second = static_cast<uint8_t>(local.tm_sec);
  out.weekday = static_cast<uint8_t>(local.tm_wday);
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
