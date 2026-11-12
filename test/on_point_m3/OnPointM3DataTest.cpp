// M3-A (RRCG, METRO MANILA) catalogue data integrity. Every list below is an independent "HH:MM" transcription of
// RRCG's website schedule feed (rrcgbus.ph/wp-json/data/trips, retrieved 2026-11-13, sha256 be4ad13c...), derived
// with jq and BSD date rather than the generator that wrote OnPointScheduleData.cpp, so a slip in either copy fails.
// It also pins the 15 pre-existing pairs to their published v1.5.0 content, the new collection, and the labels'
// rendered widths with the firmware's own fonts.

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "EpdFont.h"
#include "activities/on_point/OnPointRegions.h"
#include "activities/on_point/OnPointSchedule.h"
#include "activities/on_point/OnPointScheduleData.h"
#include "builtinFonts/notosans_16_bold.h"
#include "builtinFonts/notosans_18_bold.h"
#include "builtinFonts/ubuntu_10_bold.h"

namespace {

using on_point::CivilDateTime;
using on_point::Schedule;
using on_point::ServiceStatus;
using on_point::SourceStatus;

constexpr uint8_t SUNDAY = 1u << 0;
constexpr uint8_t SATURDAY = 1u << 6;
constexpr uint8_t MONDAY_TO_FRIDAY = 0b0111110;

// ---- RRCG feed, 2026-11-13 ----

// ATC (Alabang) > Greenbelt 1 (Makati), WEEKDAY
const char* const ATC_TO_GREENBELT_WEEKDAYS[] = {
    "05:30", "06:00", "06:30", "07:00", "07:30", "08:00", "08:30", "09:00", "09:30", "10:00", "10:30",
    "11:00", "11:30", "12:00", "12:30", "13:00", "13:30", "14:00", "14:30", "15:00", "15:30", "16:00",
    "16:30", "17:00", "17:30", "18:00", "18:30", "19:00", "19:30", "20:00", "20:30", "21:00", "21:30"};
// ATC (Alabang) > Greenbelt 5 (Makati), SAT
const char* const ATC_TO_GREENBELT_SATURDAYS[] = {"05:30", "06:00", "07:00", "08:00", "08:30", "09:00", "09:30",
                                                  "10:00", "10:30", "11:00", "11:30", "12:00", "13:00", "13:30",
                                                  "14:00", "14:30", "15:00", "15:30", "16:00", "16:30", "17:00",
                                                  "17:30", "18:00", "18:30", "19:00", "19:30", "20:00", "21:00"};
// ATC (Alabang) > Greenbelt 5 (Makati), SUN
const char* const ATC_TO_GREENBELT_SUNDAYS[] = {"06:00", "07:00", "08:00", "09:00", "10:00", "11:00", "12:00", "13:00",
                                                "14:00", "15:00", "16:00", "17:00", "18:00", "19:00", "20:00", "21:00"};
// Greenbelt 1 (Makati) > ATC (Alabang), WEEKDAY
const char* const GREENBELT_TO_ATC_WEEKDAYS[] = {
    "06:30", "07:00", "07:30", "08:00", "08:30", "09:00", "09:30", "10:00", "10:30", "11:00", "11:30",
    "12:00", "12:30", "13:00", "13:30", "14:00", "14:30", "15:00", "15:30", "16:00", "16:30", "17:00",
    "17:30", "18:00", "18:30", "19:00", "19:30", "20:00", "20:30", "21:00", "21:30", "22:00"};
// Greenbelt 5 (Makati) > ATC (Alabang), SAT
const char* const GREENBELT_TO_ATC_SATURDAYS[] = {"06:30", "07:00", "08:00", "09:00", "09:30", "10:00", "10:30",
                                                  "11:00", "11:30", "12:00", "12:30", "13:00", "14:00", "14:30",
                                                  "15:00", "15:30", "16:00", "16:30", "17:00", "17:30", "18:00",
                                                  "18:30", "19:00", "19:30", "20:00", "20:30", "21:00", "22:00"};
// Greenbelt 5 (Makati) > ATC (Alabang), SUN
const char* const GREENBELT_TO_ATC_SUNDAYS[] = {"07:00", "08:00", "09:00", "10:00", "11:00", "12:00", "13:00", "14:00",
                                                "15:00", "16:00", "17:00", "18:00", "19:00", "20:00", "21:00", "22:00"};
// Starmall Alabang (VTX) > Starmall EDSA-Shaw (Mandaluyong), WEEKDAY
const char* const VTX_TO_SHAW_WEEKDAYS[] = {
    "05:30", "06:00", "06:30", "07:00", "07:30", "08:00", "08:30", "09:00", "09:30", "10:00", "10:30",
    "11:00", "11:30", "12:00", "12:30", "13:00", "13:30", "14:00", "14:30", "15:00", "15:30", "16:00",
    "16:30", "17:00", "17:30", "18:00", "18:30", "19:00", "19:30", "20:00", "20:30", "21:00", "21:30"};
// Starmall Alabang (VTX) > Starmall EDSA-Shaw, SAT
const char* const VTX_TO_SHAW_SATURDAYS[] = {"05:30", "06:00", "07:00", "08:00", "08:30", "09:00", "10:00",
                                             "11:00", "12:00", "13:00", "14:00", "15:00", "16:00", "17:00",
                                             "17:30", "18:00", "18:30", "19:00", "19:30", "20:00", "21:00"};
// Starmall Alabang (VTX) > Starmall EDSA-Shaw, SUN
const char* const VTX_TO_SHAW_SUNDAYS[] = {"06:00", "07:00", "08:00", "08:30", "09:00", "10:00", "11:00",
                                           "12:00", "13:00", "14:00", "15:00", "16:00", "17:00", "17:30",
                                           "18:00", "18:30", "19:00", "19:30", "20:00", "20:30"};
// Starmall EDSA-Shaw > Starmall Alabang (VTX), WEEKDAY
const char* const SHAW_TO_VTX_WEEKDAYS[] = {"06:30", "07:00", "07:30", "08:00", "08:30", "09:00", "09:30", "10:00",
                                            "10:30", "11:00", "11:30", "12:00", "12:30", "13:00", "13:30", "14:00",
                                            "14:30", "15:00", "15:30", "16:00", "16:30", "17:00", "17:30", "18:00",
                                            "18:30", "19:00", "19:30", "20:00", "20:30", "21:00", "21:30", "22:00"};
// Starmall EDSA-Shaw > Starmall Alabang (VTX), SAT
const char* const SHAW_TO_VTX_SATURDAYS[] = {"06:30", "07:00", "08:00", "09:00", "10:00", "11:00", "12:00",
                                             "13:00", "14:00", "15:00", "16:00", "17:00", "18:00", "18:30",
                                             "19:00", "19:30", "20:00", "20:30", "21:00", "22:00"};
// Starmall EDSA-Shaw > Starmall Alabang (VTX), SUN
const char* const SHAW_TO_VTX_SUNDAYS[] = {"07:00", "08:00", "09:00", "09:30", "10:00", "11:00", "12:00",
                                           "13:00", "14:00", "15:00", "16:00", "17:00", "18:00", "18:30",
                                           "19:00", "19:30", "20:00", "21:00", "21:30"};
// ATC (Alabang) > SM Megamall (Ortigas), WEEKDAY
const char* const ATC_TO_MEGAMALL_WEEKDAYS[] = {"06:00", "06:45", "07:15", "08:00", "09:00", "10:00",
                                                "11:00", "12:00", "13:00", "14:00", "15:00", "16:00",
                                                "17:00", "17:45", "18:30", "19:30", "20:30"};
// ATC (Alabang) > SM Megamall (Ortigas), SAT
const char* const ATC_TO_MEGAMALL_SATURDAYS[] = {"06:00", "07:00", "08:00", "09:00", "10:00", "11:00", "12:00", "13:00",
                                                 "14:00", "15:00", "16:00", "17:00", "18:00", "19:00", "20:00"};
// SM Megamall (Ortigas) > ATC (Alabang), WEEKDAY
const char* const MEGAMALL_TO_ATC_WEEKDAYS[] = {"07:00", "08:00", "09:00", "10:00", "11:00", "12:00",
                                                "13:00", "14:00", "15:00", "16:00", "17:00", "17:45",
                                                "18:30", "19:30", "20:30", "21:30", "22:00"};
// SM Megamall (Ortigas) > ATC (Alabang), SAT
const char* const MEGAMALL_TO_ATC_SATURDAYS[] = {"07:00", "08:00", "09:00", "10:00", "11:00", "12:00", "13:00", "14:00",
                                                 "15:00", "16:00", "17:00", "18:00", "19:00", "20:00", "21:00"};
// The feed lists no Sunday trips for ATC <> SM Megamall in either direction.

using Minutes = std::vector<int>;

template <size_t N>
Minutes board(const char* const (&times)[N]) {
  Minutes out;
  for (const char* time : times) {
    const std::string t = time;
    EXPECT_EQ(t.size(), 5u) << t;
    out.push_back(std::stoi(t.substr(0, 2)) * 60 + std::stoi(t.substr(3, 2)));
  }
  for (size_t i = 1; i < out.size(); ++i) EXPECT_LT(out[i - 1], out[i]) << "a list is unsorted or repeats a time";
  return out;
}

const Schedule* scheduleById(const char* id) {
  for (size_t i = 0; i < on_point::SCHEDULE_COUNT; ++i) {
    if (strcmp(on_point::scheduleAt(i).routeId, id) == 0) return &on_point::scheduleAt(i);
  }
  return nullptr;
}

Minutes departuresFor(const Schedule& schedule, const uint8_t mask) {
  for (size_t r = 0; r < schedule.serviceRuleCount; ++r) {
    const on_point::ServiceRule& rule = schedule.serviceRules[r];
    if (rule.weekdayMask == mask)
      return Minutes(rule.departures.minutes, rule.departures.minutes + rule.departures.count);
  }
  return {};
}

struct Direction {
  const char* id;
  size_t index;
  const char* origin;
  const char* destination;
  Minutes weekdays;
  Minutes saturdays;
  Minutes sundays;  // empty: no Sunday service
};

std::vector<Direction> directions() {
  return {
      {"p2p-atc-greenbelt", 30, "ATC ALABANG", "GREENBELT", board(ATC_TO_GREENBELT_WEEKDAYS),
       board(ATC_TO_GREENBELT_SATURDAYS), board(ATC_TO_GREENBELT_SUNDAYS)},
      {"p2p-greenbelt-atc", 31, "GREENBELT", "ATC ALABANG", board(GREENBELT_TO_ATC_WEEKDAYS),
       board(GREENBELT_TO_ATC_SATURDAYS), board(GREENBELT_TO_ATC_SUNDAYS)},
      {"p2p-starmall-alabang-starmall-shaw", 32, "VTX ALABANG", "STARMALL SHAW", board(VTX_TO_SHAW_WEEKDAYS),
       board(VTX_TO_SHAW_SATURDAYS), board(VTX_TO_SHAW_SUNDAYS)},
      {"p2p-starmall-shaw-starmall-alabang", 33, "STARMALL SHAW", "VTX ALABANG", board(SHAW_TO_VTX_WEEKDAYS),
       board(SHAW_TO_VTX_SATURDAYS), board(SHAW_TO_VTX_SUNDAYS)},
      {"p2p-atc-sm-megamall",
       34,
       "ATC ALABANG",
       "SM MEGAMALL",
       board(ATC_TO_MEGAMALL_WEEKDAYS),
       board(ATC_TO_MEGAMALL_SATURDAYS),
       {}},
      {"p2p-sm-megamall-atc",
       35,
       "SM MEGAMALL",
       "ATC ALABANG",
       board(MEGAMALL_TO_ATC_WEEKDAYS),
       board(MEGAMALL_TO_ATC_SATURDAYS),
       {}},
  };
}

// Manila local time as the UTC instant the board receives.
CivilDateTime manila(const int year, const int month, const int day, const int hour, const int minute) {
  CivilDateTime utc{static_cast<int16_t>(year), static_cast<uint8_t>(month),  static_cast<uint8_t>(day),
                    static_cast<uint8_t>(hour), static_cast<uint8_t>(minute), 0};
  if (hour >= 8) {
    utc.hour = static_cast<uint8_t>(hour - 8);
  } else {
    utc.hour = static_cast<uint8_t>(hour + 16);
    utc.day = static_cast<uint8_t>(day - 1);  // test instants never fall on the first of a month
  }
  return utc;
}

// ---- Catalogue shape ----

TEST(OnPointM3Data, CatalogueGrewByExactlyThreeAppendedPairs) {
  EXPECT_EQ(on_point::SCHEDULE_COUNT, 36u);
  EXPECT_EQ(on_point::ROUTE_COUNT, 18u);
  EXPECT_EQ(on_point::routeCount(), 18u);
  struct Pair {
    size_t route;
    const char* id;
    size_t out;
    size_t back;
    const char* returnId;
    const char* originArea;
    const char* destinationArea;
  };
  const Pair added[] = {
      {15, "p2p-atc-greenbelt", 30, 31, "p2p-greenbelt-atc", "MUNTINLUPA", "MAKATI"},
      {16, "p2p-starmall-alabang-starmall-shaw", 32, 33, "p2p-starmall-shaw-starmall-alabang", "MUNTINLUPA",
       "MANDALUYONG"},
      {17, "p2p-atc-sm-megamall", 34, 35, "p2p-sm-megamall-atc", "MUNTINLUPA", "MANDALUYONG"},
  };
  for (const Pair& pair : added) {
    const on_point::RoutePair& route = on_point::routeAt(pair.route);
    EXPECT_STREQ(route.routeId, pair.id);
    EXPECT_EQ(route.outboundScheduleIndex, pair.out);
    EXPECT_EQ(route.returnScheduleIndex, pair.back);
    EXPECT_STREQ(route.originArea, pair.originArea);
    EXPECT_STREQ(route.destinationArea, pair.destinationArea);
    EXPECT_STREQ(on_point::scheduleAt(pair.out).routeId, pair.id);
    EXPECT_STREQ(on_point::scheduleAt(pair.back).routeId, pair.returnId);
    EXPECT_EQ(on_point::routeIndexForId(pair.id), pair.route);
    EXPECT_LE(strlen(pair.id), 47u);  // fits the store's 48-byte ID field
    EXPECT_LE(strlen(pair.returnId), 47u);
  }
}

// ---- Every direction against the feed ----

TEST(OnPointM3Data, EveryDirectionMatchesTheFeed) {
  for (const Direction& d : directions()) {
    SCOPED_TRACE(d.id);
    const Schedule& s = on_point::scheduleAt(d.index);
    ASSERT_STREQ(s.routeId, d.id);
    EXPECT_EQ(scheduleById(d.id), &s);
    EXPECT_TRUE(on_point::isValidSchedule(s));
    EXPECT_STREQ(s.origin, d.origin);
    EXPECT_STREQ(s.destination, d.destination);
    EXPECT_EQ(departuresFor(s, MONDAY_TO_FRIDAY), d.weekdays);
    EXPECT_EQ(departuresFor(s, SATURDAY), d.saturdays);
    EXPECT_EQ(departuresFor(s, SUNDAY), d.sundays);
    // Exactly one rule per served day type, and nothing else.
    EXPECT_EQ(s.serviceRuleCount, d.sundays.empty() ? 2u : 3u);
    uint8_t covered = 0;
    for (size_t r = 0; r < s.serviceRuleCount; ++r) {
      EXPECT_EQ(covered & s.serviceRules[r].weekdayMask, 0) << "rules overlap";
      covered |= s.serviceRules[r].weekdayMask;
    }
    EXPECT_EQ(covered, d.sundays.empty() ? (MONDAY_TO_FRIDAY | SATURDAY) : 0b1111111);
  }
}

TEST(OnPointM3Data, TripCountsMatchTheFeed) {
  const auto v = directions();
  const size_t expected[][3] = {{33, 28, 16}, {32, 28, 16}, {33, 21, 20}, {32, 20, 19}, {17, 15, 0}, {17, 15, 0}};
  for (size_t i = 0; i < v.size(); ++i) {
    EXPECT_EQ(v[i].weekdays.size(), expected[i][0]) << v[i].id;
    EXPECT_EQ(v[i].saturdays.size(), expected[i][1]) << v[i].id;
    EXPECT_EQ(v[i].sundays.size(), expected[i][2]) << v[i].id;
  }
}

TEST(OnPointM3Data, MetadataIsProvisionalAndDocumentsTerminalsAndHolidays) {
  for (const Direction& d : directions()) {
    SCOPED_TRACE(d.id);
    const Schedule& s = on_point::scheduleAt(d.index);
    EXPECT_STREQ(s.routeName, "RRCG");
    EXPECT_STREQ(s.timezone, "Asia/Manila");
    EXPECT_EQ(s.utcOffsetMinutes, 480);
    EXPECT_EQ(s.effectiveDate.year, 2026);
    EXPECT_EQ(s.effectiveDate.month, 11);
    EXPECT_EQ(s.effectiveDate.day, 13);
    EXPECT_EQ(s.sourceStatus, SourceStatus::Provisional);
    EXPECT_EQ(s.dateOverrides, nullptr);  // no invented holiday overrides
    EXPECT_EQ(s.dateOverrideCount, 0u);
    ASSERT_NE(s.notes, nullptr);
    const std::string notes = s.notes;
    EXPECT_NE(notes.find("RRCG website schedule, retrieved 2026-11-13; undated."), std::string::npos);
    EXPECT_NE(notes.find("Holiday service may differ."), std::string::npos);
  }
  const auto notesOf = [](const size_t index) { return std::string(on_point::scheduleAt(index).notes); };
  for (const size_t index : {30u, 31u})
    EXPECT_NE(notesOf(index).find("Weekdays: Greenbelt 1; weekends: Greenbelt 5."), std::string::npos);
  for (const size_t index : {32u, 33u}) EXPECT_NE(notesOf(index).find("Starmall Alabang (VTX)"), std::string::npos);
  for (const size_t index : {34u, 35u}) EXPECT_NE(notesOf(index).find("No Sunday service."), std::string::npos);
}

// ---- Behaviour on the board ----

TEST(OnPointM3Data, DayTypesSelectTheirOwnLists) {
  const Schedule& atc = on_point::scheduleAt(30);
  // Wed 2026-11-25, 05:00: the weekday 05:30.
  auto state = on_point::getDepartureState(atc, manila(2026, 11, 25, 5, 0));
  EXPECT_EQ(state.serviceStatus, ServiceStatus::BeforeFirst);
  EXPECT_EQ(state.next.local.hour, 5);
  EXPECT_EQ(state.next.local.minute, 30);
  EXPECT_EQ(state.last.local.hour, 21);
  EXPECT_EQ(state.last.local.minute, 30);
  // Sat 2026-11-28, 12:10: the Saturday list skips 12:30, so 13:00.
  state = on_point::getDepartureState(atc, manila(2026, 11, 28, 12, 10));
  EXPECT_EQ(state.next.local.hour, 13);
  EXPECT_EQ(state.next.local.minute, 0);
  EXPECT_EQ(state.last.local.hour, 21);
  EXPECT_EQ(state.last.local.minute, 0);
  // Sun 2026-11-29, 05:50: hourly from 06:00.
  state = on_point::getDepartureState(atc, manila(2026, 11, 29, 5, 50));
  EXPECT_EQ(state.next.local.hour, 6);
  EXPECT_EQ(state.next.local.minute, 0);
  ASSERT_GE(state.followingCount, 1u);
  EXPECT_EQ(state.following[0].local.hour, 7);
  EXPECT_EQ(state.following[0].local.minute, 0);
  EXPECT_EQ(state.last.local.hour, 21);
  EXPECT_EQ(state.last.local.minute, 0);
}

TEST(OnPointM3Data, MegamallHasNoSundayService) {
  for (const size_t index : {34u, 35u}) {
    const auto state = on_point::getDepartureState(on_point::scheduleAt(index), manila(2026, 11, 29, 10, 0));
    EXPECT_EQ(state.serviceStatus, ServiceStatus::NoServiceToday) << index;
  }
}

TEST(OnPointM3Data, MarkedNonWorkingDaysUseTheOrdinaryList) {
  // Tue 2026-11-17 (Metro Manila special non-working day) and Fri 2026-12-25: no override, so the weekday list.
  for (const auto& date : {std::array<int, 3>{2026, 11, 17}, std::array<int, 3>{2026, 12, 25}}) {
    const auto state = on_point::getDepartureState(on_point::scheduleAt(34), manila(date[0], date[1], date[2], 6, 30));
    EXPECT_EQ(state.next.local.hour, 6) << date[1] << "-" << date[2];
    EXPECT_EQ(state.next.local.minute, 45) << date[1] << "-" << date[2];  // weekday-only 06:45
  }
}

// ---- Collection ----

TEST(OnPointM3Data, MetroManilaIsAppendedWithTheThreePairs) {
  ASSERT_EQ(on_point::regionCount(), 4u);
  EXPECT_EQ(on_point::regionIndexForId("metro-manila"), 3u);
  const on_point::Region* metro = on_point::regionAt(3);
  ASSERT_NE(metro, nullptr);
  EXPECT_STREQ(metro->regionId, "metro-manila");
  EXPECT_EQ(std::vector<int>(metro->routes, metro->routes + metro->routeCount), (std::vector<int>{15, 16, 17}));
  EXPECT_STREQ(metro->defaultRouteId, "p2p-atc-greenbelt");
  // The earlier collections keep their positions, members and defaults.
  EXPECT_EQ(on_point::regionIndexForId("bulacan"), 0u);
  EXPECT_EQ(on_point::regionIndexForId("north-luzon"), 1u);
  EXPECT_EQ(on_point::regionIndexForId("airport-transfers"), 2u);
  for (size_t r = 0; r < 3; ++r) {
    const on_point::Region& region = *on_point::regionAt(r);
    for (size_t i = 0; i < region.routeCount; ++i) EXPECT_LT(region.routes[i], 15) << region.regionId;
  }
}

// ---- The 15 published pairs are untouched ----

// FNV-1a over every field of schedules 0-29 and routes 0-14, in index order.
struct Fnv {
  uint64_t h = 1469598103934665603ull;
  void bytes(const void* p, const size_t n) {
    for (size_t i = 0; i < n; ++i) h = (h ^ static_cast<const uint8_t*>(p)[i]) * 1099511628211ull;
  }
  void str(const char* s) {
    if (s == nullptr) {
      bytes("\xff", 1);
      return;
    }
    bytes(s, strlen(s) + 1);
  }
  template <class T>
  void num(const T v) {
    const int64_t x = static_cast<int64_t>(v);
    bytes(&x, sizeof(x));
  }
};

uint64_t publishedDigest() {
  Fnv f;
  for (size_t i = 0; i < 30; ++i) {
    const Schedule& s = on_point::scheduleAt(i);
    f.str(s.routeId);
    f.str(s.routeName);
    f.str(s.origin);
    f.str(s.destination);
    f.str(s.timezone);
    f.num(s.utcOffsetMinutes);
    f.num(s.effectiveDate.year);
    f.num(s.effectiveDate.month);
    f.num(s.effectiveDate.day);
    f.num(s.serviceRuleCount);
    for (size_t r = 0; r < s.serviceRuleCount; ++r) {
      f.num(s.serviceRules[r].weekdayMask);
      f.num(s.serviceRules[r].departures.count);
      for (size_t d = 0; d < s.serviceRules[r].departures.count; ++d) f.num(s.serviceRules[r].departures.minutes[d]);
    }
    f.num(s.dateOverrideCount);
    for (size_t o = 0; o < s.dateOverrideCount; ++o) {
      const auto& override = s.dateOverrides[o];
      f.num(override.date.year);
      f.num(override.date.month);
      f.num(override.date.day);
      f.num(override.departures.count);
      for (size_t d = 0; d < override.departures.count; ++d) f.num(override.departures.minutes[d]);
    }
    f.str(s.notes);
    f.num(static_cast<int>(s.sourceStatus));
  }
  for (size_t i = 0; i < 15; ++i) {
    const on_point::RoutePair& r = on_point::routeAt(i);
    f.str(r.routeId);
    f.num(r.outboundScheduleIndex);
    f.num(r.returnScheduleIndex);
    f.str(r.originArea);
    f.str(r.destinationArea);
  }
  return f.h;
}

TEST(OnPointM3Data, PublishedPairsAreUnchangedFromV150) {
  // Computed with this function over the published v1.5.0 source (9c0156a3).
  constexpr uint64_t V150_DIGEST = 0xa4f41dca38c50583ull;
  EXPECT_EQ(publishedDigest(), V150_DIGEST) << std::hex << "0x" << publishedDigest();
}

// ---- Labels fit, with the firmware's fonts and layout (OnPointActivity.cpp) ----

int width(const EpdFontData& data, const char* text) {
  const EpdFont font(&data);
  int w = 0, h = 0;
  font.getTextDimensions(text, &w, &h);
  return w;
}

TEST(OnPointM3Data, EveryOriginLabelClearsTheReverseControl) {
  // drawHeaderAndRoute / drawReverseAffordance: origin at MARGIN in Noto Sans 16 bold; REVERSE (Ubuntu 10 bold) plus
  // an 8 px gap and a 26 px icon, right-aligned at the margin. Kept at least 16 px apart.
  constexpr int SCREEN_WIDTH = 480, MARGIN = 24, ICON_GAP = 8, ICON_WIDTH = 26, CLEARANCE = 16;
  const int reverseX = SCREEN_WIDTH - MARGIN - width(ubuntu_10_bold, "REVERSE") - ICON_GAP - ICON_WIDTH;
  const int budget = reverseX - CLEARANCE - MARGIN;
  EXPECT_EQ(budget, 289);  // the measured budget the labels were chosen against
  for (size_t i = 0; i < on_point::SCHEDULE_COUNT; ++i) {
    const char* origin = on_point::scheduleAt(i).origin;
    EXPECT_LE(width(notosans_16_bold, origin), budget) << origin;
  }
}

TEST(OnPointM3Data, NewLabelsFitTheListAndTheMainRoute) {
  constexpr int SCREEN_WIDTH = 480, MARGIN = 24;
  const int list = SCREEN_WIDTH - MARGIN - (MARGIN + 24);  // rows: Noto Sans 16 bold after the selection bar
  const int main = SCREEN_WIDTH - MARGIN - (MARGIN + 28);  // pinned main route: Noto Sans 18 bold
  for (const char* label : {"ATC ALABANG", "GREENBELT", "VTX ALABANG", "STARMALL SHAW", "SM MEGAMALL"}) {
    EXPECT_LE(width(notosans_16_bold, label), list) << label;
    EXPECT_LE(width(notosans_18_bold, label), main) << label;
  }
  // The two Alabang terminals stay distinct on the device.
  EXPECT_STRNE(on_point::scheduleAt(30).origin, on_point::scheduleAt(32).origin);
}

}  // namespace
