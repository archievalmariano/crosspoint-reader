// BULACAN catalogue data integrity. Every directional timetable is compared
// with an independent "HH:MM" transcription of the editorial tables in
// docs/onpoint/editorial/BULACAN-BATCH1.md, so a slip in either copy fails.

#include <gtest/gtest.h>

#include <cstring>
#include <iterator>
#include <string>
#include <vector>

#include "activities/on_point/OnPointRegions.h"
#include "activities/on_point/OnPointSchedule.h"
#include "activities/on_point/OnPointScheduleData.h"

namespace {

using on_point::CivilDateTime;
using on_point::DepartureState;
using on_point::Schedule;
using on_point::SourceStatus;

constexpr uint8_t SUNDAY = 1u << 0;
constexpr uint8_t MONDAY = 1u << 1;
constexpr uint8_t TUESDAY = 1u << 2;
constexpr uint8_t FRIDAY = 1u << 5;
constexpr uint8_t SATURDAY = 1u << 6;

std::vector<int> minutes(const char* const* times, const size_t count) {
  std::vector<int> out;
  for (size_t i = 0; i < count; ++i) {
    const std::string t = times[i];
    out.push_back(std::stoi(t.substr(0, 2)) * 60 + std::stoi(t.substr(3, 2)));
  }
  return out;
}

std::vector<int> every(const char* first, const char* last, const int step) {
  const char* const bounds[] = {first, last};
  const std::vector<int> b = minutes(bounds, 2);
  std::vector<int> out;
  for (int t = b[0]; t <= b[1]; t += step) out.push_back(t);
  return out;
}

const Schedule* scheduleById(const char* id) {
  for (size_t i = 0; i < on_point::SCHEDULE_COUNT; ++i) {
    if (strcmp(on_point::scheduleAt(i).routeId, id) == 0) return &on_point::scheduleAt(i);
  }
  return nullptr;
}

// Departures of the one rule covering this weekday bit.
std::vector<int> ruleFor(const Schedule& schedule, const uint8_t day) {
  std::vector<int> out;
  size_t matches = 0;
  for (size_t i = 0; i < schedule.serviceRuleCount; ++i) {
    const on_point::ServiceRule& rule = schedule.serviceRules[i];
    if ((rule.weekdayMask & day) == 0) continue;
    ++matches;
    for (size_t d = 0; d < rule.departures.count; ++d) out.push_back(rule.departures.minutes[d]);
  }
  EXPECT_EQ(matches, 1u) << schedule.routeId << " day bit " << static_cast<int>(day);
  return out;
}

struct Expected {
  const char* id;
  const char* operatorName;
  const char* origin;
  const char* destination;
  SourceStatus status;
  std::vector<int> monday;
  std::vector<int> tuesdayToFriday;
  std::vector<int> weekend;
  bool holidaysUseWeekend;  // false: no holiday overrides
};

const char* const GUIGUINTO_WEEKDAYS[] = {
    "04:30", "05:00", "05:30", "06:00", "06:30", "07:00", "07:30", "08:00", "08:30",
    "09:00", "09:30", "10:00", "10:45", "11:30", "12:15", "13:00", "13:45", "14:30",
    "15:00", "15:30", "16:00", "16:30", "17:00", "17:30", "18:00", "18:45", "19:30",
};
const char* const GUIGUINTO_WEEKENDS[] = {
    "05:00", "05:30", "06:15", "07:00", "07:45", "08:30", "09:15", "10:00", "10:45", "11:30", "12:15",
    "13:00", "13:45", "14:30", "15:15", "16:00", "16:45", "17:30", "18:15", "19:00", "19:30",
};

std::vector<Expected> expectedBulacan() {
  const std::vector<int> guiguintoWeekdays = minutes(GUIGUINTO_WEEKDAYS, std::size(GUIGUINTO_WEEKDAYS));
  const std::vector<int> guiguintoWeekends = minutes(GUIGUINTO_WEEKENDS, std::size(GUIGUINTO_WEEKENDS));
  return {
      {"p2p-balagtas-trinoma", "Bulacan P2P", "BALAGTAS", "TRINOMA", SourceStatus::Provisional,
       every("04:30", "20:00", 30), every("04:30", "20:00", 30), every("05:00", "20:00", 30), true},
      {"p2p-trinoma-balagtas", "Bulacan P2P", "TRINOMA", "BALAGTAS", SourceStatus::Provisional,
       every("06:00", "22:30", 30), every("06:00", "22:30", 30), every("06:30", "22:30", 30), true},
      {"p2p-caypombo-sm-north-edsa", "Precious Grace", "CAYPOMBO", "SM NORTH EDSA", SourceStatus::Verified,
       every("05:00", "20:30", 30), every("05:00", "20:30", 30), every("05:00", "20:30", 30), false},
      {"p2p-sm-north-edsa-caypombo", "Precious Grace", "SM NORTH EDSA", "CAYPOMBO", SourceStatus::Provisional,
       every("06:00", "22:30", 30), every("06:00", "22:30", 30), every("06:00", "22:30", 30), false},
      {"p2p-pandi-trinoma", "Bulacan P2P", "PANDI", "TRINOMA", SourceStatus::Provisional, every("03:30", "19:30", 30),
       every("04:30", "19:30", 30), every("04:30", "19:30", 30), true},
      {"p2p-trinoma-pandi", "Bulacan P2P", "TRINOMA", "PANDI", SourceStatus::Provisional, every("06:00", "21:30", 30),
       every("06:00", "21:30", 30), every("06:30", "21:30", 30), true},
      {"p2p-plaridel-trinoma", "Bulacan P2P", "PLARIDEL", "TRINOMA", SourceStatus::Provisional,
       every("03:30", "20:30", 30), every("04:00", "20:30", 30), every("04:30", "20:30", 30), true},
      {"p2p-trinoma-plaridel", "Bulacan P2P", "TRINOMA", "PLARIDEL", SourceStatus::Provisional,
       every("06:00", "21:00", 30), every("06:00", "21:00", 30), every("06:30", "21:00", 30), true},
      {"p2p-malolos-trinoma", "Precious Grace", "MALOLOS", "TRINOMA", SourceStatus::Verified,
       every("05:00", "20:00", 30), every("05:00", "20:00", 30), every("05:00", "20:00", 30), false},
      {"p2p-trinoma-malolos", "Precious Grace", "TRINOMA", "MALOLOS", SourceStatus::Provisional,
       every("06:00", "22:30", 30), every("06:00", "22:30", 30), every("06:00", "22:30", 30), false},
      {"p2p-guiguinto-trinoma", "Bulacan P2P", "GUIGUINTO", "TRINOMA", SourceStatus::Provisional, guiguintoWeekdays,
       guiguintoWeekdays, guiguintoWeekends, true},
      {"p2p-trinoma-guiguinto", "Bulacan P2P", "TRINOMA", "GUIGUINTO", SourceStatus::Provisional,
       every("06:00", "21:30", 30), every("06:00", "21:30", 30), every("06:30", "21:30", 45), true},
  };
}

TEST(OnPointBulacanData, EveryDirectionMatchesTheEditorialTable) {
  for (const Expected& e : expectedBulacan()) {
    SCOPED_TRACE(e.id);
    const Schedule* s = scheduleById(e.id);
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(on_point::isValidSchedule(*s));
    EXPECT_STREQ(s->routeName, e.operatorName);
    EXPECT_STREQ(s->origin, e.origin);
    EXPECT_STREQ(s->destination, e.destination);
    EXPECT_EQ(s->sourceStatus, e.status);
    EXPECT_EQ(s->effectiveDate.year, 2026);
    EXPECT_EQ(s->effectiveDate.month, 9);
    EXPECT_EQ(s->effectiveDate.day, 1);
    EXPECT_EQ(ruleFor(*s, MONDAY), e.monday);
    EXPECT_EQ(ruleFor(*s, TUESDAY), e.tuesdayToFriday);
    EXPECT_EQ(ruleFor(*s, FRIDAY), e.tuesdayToFriday);
    EXPECT_EQ(ruleFor(*s, SATURDAY), e.weekend);
    EXPECT_EQ(ruleFor(*s, SUNDAY), e.weekend);
  }
}

// Bulacan P2P directions carry the same non-working holidays as Balagtas on
// their Saturday/Sunday/holiday timetable; Precious Grace's schedule is daily.
// Deliberately fails when holidays reach Balagtas alone (feature/ph-holidays-2027
// adds 2027 to it): the other Bulacan P2P directions need the same dates.
TEST(OnPointBulacanData, HolidayOverridesFollowEachOperatorsPolicy) {
  const Schedule& balagtas = on_point::scheduleForRoute(0, false);
  for (const Expected& e : expectedBulacan()) {
    SCOPED_TRACE(e.id);
    const Schedule* s = scheduleById(e.id);
    ASSERT_NE(s, nullptr);
    if (!e.holidaysUseWeekend) {
      EXPECT_EQ(s->dateOverrideCount, 0u);
      continue;
    }
    ASSERT_EQ(s->dateOverrideCount, balagtas.dateOverrideCount)
        << "every Bulacan P2P direction needs Balagtas's holiday dates (BULACAN-BATCH1.md §4)";
    for (size_t i = 0; i < s->dateOverrideCount; ++i) {
      const on_point::DateOverride& o = s->dateOverrides[i];
      EXPECT_EQ(o.date.year, balagtas.dateOverrides[i].date.year);
      EXPECT_EQ(o.date.month, balagtas.dateOverrides[i].date.month);
      EXPECT_EQ(o.date.day, balagtas.dateOverrides[i].date.day);
      const std::vector<int> departures(o.departures.minutes, o.departures.minutes + o.departures.count);
      EXPECT_EQ(departures, e.weekend) << "override " << i;
    }
  }
}

TEST(OnPointBulacanData, OperatorFirstAndLastTripsAreKeptExactly) {
  for (const Expected& e : expectedBulacan()) {
    SCOPED_TRACE(e.id);
    for (const auto* list : {&e.monday, &e.tuesdayToFriday, &e.weekend}) {
      ASSERT_FALSE(list->empty());
      for (size_t i = 1; i < list->size(); ++i) EXPECT_LT((*list)[i - 1], (*list)[i]);
    }
  }
  // Spot-check the operators' stated windows (BP2P-0830, PG-P-0313).
  const auto first = [](const char* id, const uint8_t day) { return ruleFor(*scheduleById(id), day).front(); };
  const auto last = [](const char* id, const uint8_t day) { return ruleFor(*scheduleById(id), day).back(); };
  EXPECT_EQ(first("p2p-pandi-trinoma", MONDAY), 3 * 60 + 30);
  EXPECT_EQ(first("p2p-plaridel-trinoma", MONDAY), 3 * 60 + 30);
  EXPECT_EQ(first("p2p-plaridel-trinoma", TUESDAY), 4 * 60);
  EXPECT_EQ(first("p2p-guiguinto-trinoma", SATURDAY), 5 * 60);
  EXPECT_EQ(last("p2p-guiguinto-trinoma", SATURDAY), 19 * 60 + 30);
  EXPECT_EQ(last("p2p-caypombo-sm-north-edsa", SUNDAY), 20 * 60 + 30);
  EXPECT_EQ(last("p2p-trinoma-plaridel", FRIDAY), 21 * 60);
}

TEST(OnPointBulacanData, CollectionHoldsTheSixPairsInCatalogueOrder) {
  const on_point::Region* bulacan = on_point::regionAt(on_point::regionIndexForId("bulacan"));
  ASSERT_NE(bulacan, nullptr);
  const char* const expected[] = {"p2p-balagtas-trinoma", "p2p-caypombo-sm-north-edsa", "p2p-pandi-trinoma",
                                  "p2p-plaridel-trinoma", "p2p-malolos-trinoma",        "p2p-guiguinto-trinoma"};
  ASSERT_EQ(bulacan->routeCount, std::size(expected));
  for (size_t i = 0; i < bulacan->routeCount; ++i) {
    const on_point::RoutePair& route = on_point::routeAt(bulacan->routes[i]);
    EXPECT_STREQ(route.routeId, expected[i]);
    EXPECT_STREQ(on_point::scheduleAt(route.outboundScheduleIndex).routeId, expected[i]);
  }
  // Deferred or excluded routes are absent.
  for (const char* absent : {"p2p-plaridel-sm-north-edsa", "p2p-angat-monumento"}) {
    for (size_t i = 0; i < on_point::routeCount(); ++i) EXPECT_STRNE(on_point::routeAt(i).routeId, absent);
  }
}

// Manila local time (UTC+8) as UTC; dates here avoid month boundaries.
CivilDateTime manila(const int month, const int day, const int hour, const int minute) {
  if (hour >= 8)
    return {2026,
            static_cast<uint8_t>(month),
            static_cast<uint8_t>(day),
            static_cast<uint8_t>(hour - 8),
            static_cast<uint8_t>(minute),
            0};
  return {2026,
          static_cast<uint8_t>(month),
          static_cast<uint8_t>(day - 1),
          static_cast<uint8_t>(hour + 16),
          static_cast<uint8_t>(minute),
          0};
}

int nextMinute(const char* id, const CivilDateTime& now) {
  const DepartureState state = on_point::getDepartureState(*scheduleById(id), now);
  return state.next.local.hour * 60 + state.next.local.minute;
}

TEST(OnPointBulacanData, DayTypesResolveOnTheDevice) {
  // Monday 2026-10-26 runs Pandi's 03:30 Monday trip; Tuesday starts at 04:30.
  EXPECT_EQ(nextMinute("p2p-pandi-trinoma", manila(10, 26, 3, 0)), 3 * 60 + 30);
  EXPECT_EQ(nextMinute("p2p-pandi-trinoma", manila(10, 27, 3, 0)), 4 * 60 + 30);
  // Bonifacio Day, Monday 2026-11-30: the holiday timetable replaces Monday's.
  EXPECT_EQ(nextMinute("p2p-plaridel-trinoma", manila(11, 30, 3, 0)), 4 * 60 + 30);
  // Guiguinto Saturday 2026-10-31: 05:30 follows 05:00 (no 04:30/04:45).
  EXPECT_EQ(nextMinute("p2p-guiguinto-trinoma", manila(10, 31, 4, 0)), 5 * 60);
  EXPECT_EQ(nextMinute("p2p-guiguinto-trinoma", manila(10, 31, 5, 1)), 5 * 60 + 30);
  // Guiguinto weekday midday keeps the notice's 45-minute gap after 10:00.
  EXPECT_EQ(nextMinute("p2p-guiguinto-trinoma", manila(10, 27, 10, 1)), 10 * 60 + 45);
  // TriNoma to Guiguinto, Sunday 2026-10-25: the 45-minute weekend reconstruction.
  EXPECT_EQ(nextMinute("p2p-trinoma-guiguinto", manila(10, 25, 6, 31)), 7 * 60 + 15);
  // Caypombo runs its added 20:30 last trip.
  EXPECT_EQ(nextMinute("p2p-caypombo-sm-north-edsa", manila(10, 27, 20, 1)), 20 * 60 + 30);
}

}  // namespace
