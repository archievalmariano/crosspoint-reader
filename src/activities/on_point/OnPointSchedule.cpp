#include "OnPointSchedule.h"

#include <algorithm>
#include <cstdint>

namespace on_point {
namespace {

constexpr int64_t SECONDS_PER_DAY = 86400;
constexpr int MAX_SERVICE_MINUTE = 2879;
// A full year covers long holiday/seasonal override runs while keeping the
// fixed-array search bounded and allocation-free.
constexpr int SEARCH_DAYS = 370;
constexpr size_t CANDIDATE_COUNT = FOLLOWING_DEPARTURE_COUNT + 1;

constexpr bool sameDate(const CivilDate& a, const CivilDate& b) {
  return a.year == b.year && a.month == b.month && a.day == b.day;
}

constexpr int64_t daysFromCivil(int year, unsigned month, unsigned day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(year - era * 400);
  const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468;
}

constexpr CivilDate civilFromDays(int64_t days) {
  days += 719468;
  const int64_t era = (days >= 0 ? days : days - 146096) / 146097;
  const unsigned doe = static_cast<unsigned>(days - era * 146097);
  const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int year = static_cast<int>(yoe) + static_cast<int>(era) * 400;
  const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const unsigned mp = (5 * doy + 2) / 153;
  const unsigned day = doy - (153 * mp + 2) / 5 + 1;
  const unsigned month = mp + (mp < 10 ? 3 : -9);
  year += month <= 2;
  return {static_cast<int16_t>(year), static_cast<uint8_t>(month), static_cast<uint8_t>(day)};
}

constexpr bool isValidDate(const CivilDate& date) {
  if (date.year < 1970 || date.year > 2099 || date.month < 1 || date.month > 12 || date.day < 1 || date.day > 31)
    return false;
  return sameDate(civilFromDays(daysFromCivil(date.year, date.month, date.day)), date);
}

constexpr bool isValidDateTime(const CivilDateTime& value) {
  return isValidDate({value.year, value.month, value.day}) && value.hour < 24 && value.minute < 60 && value.second < 60;
}

constexpr uint8_t weekdayForDay(const int64_t day) {
  int value = static_cast<int>((day + 4) % 7);  // 1970-01-01 was Thursday.
  if (value < 0) value += 7;
  return static_cast<uint8_t>(value);
}

constexpr int64_t floorDiv(const int64_t numerator, const int64_t denominator) {
  int64_t result = numerator / denominator;
  if (numerator < 0 && numerator % denominator != 0) --result;
  return result;
}

CivilDateTime dateTimeFromEpoch(const int64_t seconds) {
  const int64_t day = floorDiv(seconds, SECONDS_PER_DAY);
  const int64_t daySeconds = seconds - day * SECONDS_PER_DAY;
  const CivilDate date = civilFromDays(day);
  return {date.year,
          date.month,
          date.day,
          static_cast<uint8_t>(daySeconds / 3600),
          static_cast<uint8_t>((daySeconds % 3600) / 60),
          static_cast<uint8_t>(daySeconds % 60)};
}

int64_t epochFromDateTime(const CivilDateTime& value) {
  return daysFromCivil(value.year, value.month, value.day) * SECONDS_PER_DAY + static_cast<int64_t>(value.hour) * 3600 +
         static_cast<int64_t>(value.minute) * 60 + value.second;
}

bool validDepartureList(const DepartureList& list, const bool allowEmpty) {
  if (list.count == 0) return allowEmpty && list.minutes == nullptr;
  if (list.minutes == nullptr) return false;
  uint16_t previous = 0;
  for (size_t i = 0; i < list.count; ++i) {
    const uint16_t minute = list.minutes[i];
    if (minute > MAX_SERVICE_MINUTE || (i > 0 && minute <= previous)) return false;
    previous = minute;
  }
  return true;
}

const DepartureList* departuresForDay(const Schedule& schedule, const int64_t serviceDay) {
  const CivilDate date = civilFromDays(serviceDay);
  if (serviceDay < daysFromCivil(schedule.effectiveDate.year, schedule.effectiveDate.month, schedule.effectiveDate.day))
    return nullptr;

  for (size_t i = 0; i < schedule.dateOverrideCount; ++i) {
    if (sameDate(schedule.dateOverrides[i].date, date)) return &schedule.dateOverrides[i].departures;
  }

  const uint8_t weekdayBit = static_cast<uint8_t>(1u << weekdayForDay(serviceDay));
  for (size_t i = 0; i < schedule.serviceRuleCount; ++i) {
    if ((schedule.serviceRules[i].weekdayMask & weekdayBit) != 0) return &schedule.serviceRules[i].departures;
  }
  return nullptr;
}

DepartureOccurrence makeOccurrence(const int64_t serviceDay, const uint16_t serviceMinute, const bool isLast) {
  DepartureOccurrence result;
  result.local = dateTimeFromEpoch(serviceDay * SECONDS_PER_DAY + static_cast<int64_t>(serviceMinute) * 60);
  result.serviceMinute = serviceMinute;
  result.isLast = isLast;
  result.valid = true;
  return result;
}

int64_t occurrenceEpoch(const DepartureOccurrence& occurrence) { return epochFromDateTime(occurrence.local); }

int32_t ceilMinutes(const int64_t seconds) {
  if (seconds < 0) return -1;
  return static_cast<int32_t>((seconds + 59) / 60);
}

void insertCandidate(std::array<DepartureOccurrence, CANDIDATE_COUNT>& candidates, size_t& count,
                     const DepartureOccurrence& candidate) {
  const int64_t epoch = occurrenceEpoch(candidate);
  size_t insertAt = 0;
  while (insertAt < count && occurrenceEpoch(candidates[insertAt]) < epoch) ++insertAt;
  if (insertAt < count && occurrenceEpoch(candidates[insertAt]) == epoch) return;
  if (insertAt >= CANDIDATE_COUNT) return;
  const size_t newCount = std::min(count + 1, CANDIDATE_COUNT);
  for (size_t i = newCount - 1; i > insertAt; --i) candidates[i] = candidates[i - 1];
  candidates[insertAt] = candidate;
  count = newCount;
}

}  // namespace

bool isValidSchedule(const Schedule& schedule) {
  if (schedule.routeId == nullptr || schedule.routeId[0] == '\0' || schedule.routeName == nullptr ||
      schedule.routeName[0] == '\0' || schedule.origin == nullptr || schedule.origin[0] == '\0' ||
      schedule.destination == nullptr || schedule.destination[0] == '\0' || schedule.timezone == nullptr ||
      schedule.timezone[0] == '\0' || schedule.utcOffsetMinutes < -720 || schedule.utcOffsetMinutes > 840 ||
      !isValidDate(schedule.effectiveDate) || schedule.serviceRules == nullptr || schedule.serviceRuleCount == 0)
    return false;

  uint8_t usedWeekdays = 0;
  for (size_t i = 0; i < schedule.serviceRuleCount; ++i) {
    const ServiceRule& rule = schedule.serviceRules[i];
    if (rule.weekdayMask == 0 || (usedWeekdays & rule.weekdayMask) != 0 || !validDepartureList(rule.departures, false))
      return false;
    usedWeekdays |= rule.weekdayMask;
  }

  if (schedule.dateOverrideCount > 0 && schedule.dateOverrides == nullptr) return false;
  for (size_t i = 0; i < schedule.dateOverrideCount; ++i) {
    if (!isValidDate(schedule.dateOverrides[i].date) || !validDepartureList(schedule.dateOverrides[i].departures, true))
      return false;
    for (size_t j = 0; j < i; ++j)
      if (sameDate(schedule.dateOverrides[i].date, schedule.dateOverrides[j].date)) return false;
  }
  return true;
}

DepartureState getDepartureState(const Schedule& schedule, const CivilDateTime& nowUtc) {
  DepartureState state;
  if (!isValidSchedule(schedule) || !isValidDateTime(nowUtc)) return state;

  const int64_t localEpoch = epochFromDateTime(nowUtc) + static_cast<int64_t>(schedule.utcOffsetMinutes) * 60;
  const int64_t localDay = floorDiv(localEpoch, SECONDS_PER_DAY);
  state.localNow = dateTimeFromEpoch(localEpoch);

  // Classify the service day independently from future-departure selection. The
  // previous day is included so a future cross-midnight timetable remains valid.
  int64_t stateServiceDay = localDay;
  const DepartureList* stateList = nullptr;
  for (const int dayDelta : {-1, 0}) {
    const int64_t serviceDay = localDay + dayDelta;
    const DepartureList* list = departuresForDay(schedule, serviceDay);
    if (list == nullptr || list->count == 0) continue;
    const int64_t firstEpoch = serviceDay * SECONDS_PER_DAY + static_cast<int64_t>(list->minutes[0]) * 60;
    const int64_t lastEpoch = serviceDay * SECONDS_PER_DAY + static_cast<int64_t>(list->minutes[list->count - 1]) * 60;
    if (localEpoch >= firstEpoch && localEpoch <= lastEpoch) {
      stateServiceDay = serviceDay;
      stateList = list;
      state.serviceStatus = ServiceStatus::InService;
    }
  }

  if (stateList == nullptr) {
    stateList = departuresForDay(schedule, localDay);
    stateServiceDay = localDay;
    if (stateList == nullptr || stateList->count == 0) {
      state.serviceStatus = ServiceStatus::NoServiceToday;
      stateList = nullptr;
    } else {
      const int64_t firstEpoch = localDay * SECONDS_PER_DAY + static_cast<int64_t>(stateList->minutes[0]) * 60;
      const int64_t lastEpoch =
          localDay * SECONDS_PER_DAY + static_cast<int64_t>(stateList->minutes[stateList->count - 1]) * 60;
      if (localEpoch < firstEpoch)
        state.serviceStatus = ServiceStatus::BeforeFirst;
      else if (localEpoch <= lastEpoch)
        state.serviceStatus = ServiceStatus::InService;
      else
        state.serviceStatus = ServiceStatus::ServiceEnded;
    }
  }

  if (stateList != nullptr) {
    state.first = makeOccurrence(stateServiceDay, stateList->minutes[0], stateList->count == 1);
    state.last = makeOccurrence(stateServiceDay, stateList->minutes[stateList->count - 1], true);
    state.minutesToLast = ceilMinutes(occurrenceEpoch(state.last) - localEpoch);
  }

  std::array<DepartureOccurrence, CANDIDATE_COUNT> candidates{};
  size_t candidateCount = 0;
  for (int dayDelta = -1; dayDelta < SEARCH_DAYS; ++dayDelta) {
    const int64_t serviceDay = localDay + dayDelta;
    const DepartureList* list = departuresForDay(schedule, serviceDay);
    if (list == nullptr) continue;
    for (size_t i = 0; i < list->count; ++i) {
      const int64_t epoch = serviceDay * SECONDS_PER_DAY + static_cast<int64_t>(list->minutes[i]) * 60;
      if (epoch < localEpoch) continue;
      insertCandidate(candidates, candidateCount, makeOccurrence(serviceDay, list->minutes[i], i + 1 == list->count));
    }
  }

  if (candidateCount == 0) {
    state.serviceStatus = ServiceStatus::NoFutureService;
    return state;
  }

  state.next = candidates[0];
  state.minutesToNext = ceilMinutes(occurrenceEpoch(state.next) - localEpoch);
  state.isLastDeparture = state.next.isLast;
  const int64_t nextServiceDay =
      floorDiv(occurrenceEpoch(state.next) - static_cast<int64_t>(state.next.serviceMinute) * 60, SECONDS_PER_DAY);
  if (const DepartureList* nextList = departuresForDay(schedule, nextServiceDay); nextList != nullptr) {
    for (size_t i = 1; i < nextList->count; ++i) {
      if (nextList->minutes[i] == state.next.serviceMinute) {
        state.intervalMinutes = static_cast<uint16_t>(nextList->minutes[i] - nextList->minutes[i - 1]);
        break;
      }
    }
  }
  state.followingCount = std::min(candidateCount - 1, FOLLOWING_DEPARTURE_COUNT);
  for (size_t i = 0; i < state.followingCount; ++i) state.following[i] = candidates[i + 1];

  if (state.serviceStatus == ServiceStatus::InService && occurrenceEpoch(state.next) < localEpoch)
    state.serviceStatus = ServiceStatus::ServiceEnded;
  return state;
}

}  // namespace on_point
