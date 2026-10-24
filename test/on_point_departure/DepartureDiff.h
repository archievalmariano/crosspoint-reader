#pragma once

// Differential checks of on_point::getDepartureState against the frozen reference
// (test/fixtures/on_point/LegacyDepartureState.h): every DepartureState field must match.

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "../fixtures/on_point/LegacyDepartureState.h"
#include "activities/on_point/OnPointSchedule.h"

namespace departure_diff {

using on_point::CivilDate;
using on_point::CivilDateTime;
using on_point::DepartureOccurrence;
using on_point::DepartureState;
using on_point::Schedule;

inline int64_t epochFromLocal(int year, unsigned month, unsigned day, int hour = 0, int minute = 0, int second = 0) {
  using namespace std::chrono;
  const sys_days days{std::chrono::year{year} / std::chrono::month{month} / std::chrono::day{day}};
  return days.time_since_epoch().count() * 86400LL + hour * 3600LL + minute * 60LL + second;
}

inline CivilDateTime civilFromEpoch(const int64_t epoch) {
  using namespace std::chrono;
  const sys_seconds t{seconds{epoch}};
  const sys_days days = floor<std::chrono::days>(t);
  const year_month_day ymd{days};
  const int64_t rest = (t - days).count();
  return {static_cast<int16_t>(static_cast<int>(ymd.year())),
          static_cast<uint8_t>(static_cast<unsigned>(ymd.month())),
          static_cast<uint8_t>(static_cast<unsigned>(ymd.day())),
          static_cast<uint8_t>(rest / 3600),
          static_cast<uint8_t>(rest % 3600 / 60),
          static_cast<uint8_t>(rest % 60)};
}

// UTC instant for a local wall-clock time under a schedule's fixed offset.
inline CivilDateTime utcForLocal(const Schedule& schedule, int year, unsigned month, unsigned day, int hour = 0,
                                 int minute = 0, int second = 0) {
  return civilFromEpoch(epochFromLocal(year, month, day, hour, minute, second) -
                        static_cast<int64_t>(schedule.utcOffsetMinutes) * 60);
}

inline std::string describe(const CivilDateTime& t) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%04d-%02u-%02u %02u:%02u:%02u", t.year, t.month, t.day, t.hour, t.minute, t.second);
  return buf;
}

inline bool sameDateTime(const CivilDateTime& a, const CivilDateTime& b) {
  return a.year == b.year && a.month == b.month && a.day == b.day && a.hour == b.hour && a.minute == b.minute &&
         a.second == b.second;
}

inline bool sameOccurrence(const DepartureOccurrence& a, const DepartureOccurrence& b) {
  return sameDateTime(a.local, b.local) && a.serviceMinute == b.serviceMinute && a.isLast == b.isLast &&
         a.valid == b.valid;
}

// Every field, including the unused tail of `following`.
inline bool sameState(const DepartureState& a, const DepartureState& b) {
  if (a.serviceStatus != b.serviceStatus || !sameDateTime(a.localNow, b.localNow) || !sameOccurrence(a.next, b.next) ||
      a.followingCount != b.followingCount || !sameOccurrence(a.first, b.first) || !sameOccurrence(a.last, b.last) ||
      a.minutesToNext != b.minutesToNext || a.minutesToLast != b.minutesToLast ||
      a.intervalMinutes != b.intervalMinutes || a.isLastDeparture != b.isLastDeparture)
    return false;
  for (size_t i = 0; i < a.following.size(); ++i)
    if (!sameOccurrence(a.following[i], b.following[i])) return false;
  return true;
}

// Compares at one UTC instant; returns the reference state for scenario-specific expectations.
inline DepartureState check(const Schedule& schedule, const CivilDateTime& nowUtc, const char* context = "") {
  const DepartureState expected = on_point::legacy::getDepartureState(schedule, nowUtc);
  const DepartureState actual = on_point::getDepartureState(schedule, nowUtc);
  EXPECT_TRUE(sameState(expected, actual))
      << context << " " << (schedule.routeId ? schedule.routeId : "(null)") << " at UTC " << describe(nowUtc);
  return expected;
}

// Compares at every `step` seconds over [fromEpoch, toEpoch) (UTC epochs); stops after the
// first mismatch so a regression doesn't flood the log. Returns the number of instants.
inline size_t sweep(const Schedule& schedule, const int64_t fromEpoch, const int64_t toEpoch, const int64_t step,
                    const char* context = "") {
  size_t count = 0;
  for (int64_t t = fromEpoch; t < toEpoch; t += step, ++count) {
    const CivilDateTime now = civilFromEpoch(t);
    if (!sameState(on_point::legacy::getDepartureState(schedule, now), on_point::getDepartureState(schedule, now))) {
      ADD_FAILURE() << context << " " << schedule.routeId << " differs at UTC " << describe(now);
      break;
    }
  }
  return count;
}

// A hand-built schedule that owns its arrays (pointers stay valid: never copied or moved).
class Built {
 public:
  struct Rule {
    uint8_t mask;
    std::vector<uint16_t> minutes;
  };
  struct Override {
    CivilDate date;
    std::vector<uint16_t> minutes;
  };

  Built(const char* id, CivilDate effective, std::vector<Rule> rules, std::vector<Override> overrides = {},
        int16_t offsetMinutes = 480)
      : rulesData(std::move(rules)), overridesData(std::move(overrides)) {
    for (auto& rule : rulesData)
      serviceRules.push_back({rule.mask, {rule.minutes.empty() ? nullptr : rule.minutes.data(), rule.minutes.size()}});
    for (auto& entry : overridesData)
      dateOverrides.push_back(
          {entry.date, {entry.minutes.empty() ? nullptr : entry.minutes.data(), entry.minutes.size()}});
    schedule = {id,
                "TEST",
                "ORIGIN",
                "DESTINATION",
                "Asia/Manila",
                offsetMinutes,
                effective,
                serviceRules.empty() ? nullptr : serviceRules.data(),
                serviceRules.size(),
                dateOverrides.empty() ? nullptr : dateOverrides.data(),
                dateOverrides.size(),
                "",
                on_point::SourceStatus::Verified};
  }
  Built(const Built&) = delete;
  Built& operator=(const Built&) = delete;

  Schedule schedule;

 private:
  std::vector<Rule> rulesData;
  std::vector<Override> overridesData;
  std::vector<on_point::ServiceRule> serviceRules;
  std::vector<on_point::DateOverride> dateOverrides;
};

constexpr uint8_t ALL_DAYS = 0x7F;

}  // namespace departure_diff
