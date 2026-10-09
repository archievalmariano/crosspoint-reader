#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace on_point {

struct CivilDate {
  int16_t year = 2000;
  uint8_t month = 1;
  uint8_t day = 1;
};

struct CivilDateTime {
  int16_t year = 2000;
  uint8_t month = 1;
  uint8_t day = 1;
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint8_t second = 0;
};

struct DepartureList {
  // Minutes from the service day's midnight. Values above 1439 represent a
  // departure after midnight belonging to the preceding service day.
  const uint16_t* minutes = nullptr;
  size_t count = 0;
};

struct ServiceRule {
  // Bit 0 = Sunday ... bit 6 = Saturday.
  uint8_t weekdayMask = 0;
  DepartureList departures;
};

struct DateOverride {
  CivilDate date;
  // An empty list explicitly marks a non-service day.
  DepartureList departures;
};

enum class SourceStatus : uint8_t { Provisional, Verified };

struct Schedule {
  const char* routeId = nullptr;
  const char* routeName = nullptr;
  const char* origin = nullptr;
  const char* destination = nullptr;
  const char* timezone = nullptr;
  int16_t utcOffsetMinutes = 0;
  CivilDate effectiveDate;
  const ServiceRule* serviceRules = nullptr;
  size_t serviceRuleCount = 0;
  const DateOverride* dateOverrides = nullptr;
  size_t dateOverrideCount = 0;
  const char* notes = nullptr;
  SourceStatus sourceStatus = SourceStatus::Provisional;
};

enum class ServiceStatus : uint8_t {
  InvalidSchedule,
  NoFutureService,
  BeforeFirst,
  InService,
  ServiceEnded,
  NoServiceToday,
};

struct DepartureOccurrence {
  CivilDateTime local;
  uint16_t serviceMinute = 0;
  bool isLast = false;
  bool valid = false;
};

constexpr size_t FOLLOWING_DEPARTURE_COUNT = 3;

struct DepartureState {
  ServiceStatus serviceStatus = ServiceStatus::InvalidSchedule;
  CivilDateTime localNow;
  DepartureOccurrence next;
  std::array<DepartureOccurrence, FOLLOWING_DEPARTURE_COUNT> following{};
  size_t followingCount = 0;
  DepartureOccurrence first;
  DepartureOccurrence last;
  int32_t minutesToNext = -1;
  int32_t minutesToLast = -1;
  uint16_t intervalMinutes = 0;
  bool isLastDeparture = false;
};

// The clock supplies UTC. This function applies the schedule's explicit UTC
// offset before matching service days, so timetable logic never depends on the
// process or firmware timezone.
DepartureState getDepartureState(const Schedule& schedule, const CivilDateTime& nowUtc);

bool isValidSchedule(const Schedule& schedule);

}  // namespace on_point
