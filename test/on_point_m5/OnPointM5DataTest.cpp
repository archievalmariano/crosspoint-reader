// M5 (SOUTH LUZON) catalogue data integrity. The lists below are typed directly from RRCG's operator boards (Batangas
// and Lipa DAILY; SM Masinag WEEKDAY; Sierra Valley MONDAY & FRIDAY and TUESDAY TO THURSDAY), not from the
// transcription file that generated OnPointScheduleData.cpp, so a slip in either copy fails. Also covers day rules,
// no-service days, the new collection, exact membership of every earlier collection and the M4 catalogue.

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

constexpr uint8_t ALL_DAYS = 0b1111111;
constexpr uint8_t MONDAY_TO_FRIDAY = 0b0111110;
constexpr uint8_t MONDAY_AND_FRIDAY = 0b0100010;
constexpr uint8_t TUESDAY_TO_THURSDAY = 0b0011100;

// ---- RRCG DAILY SCHEDULE: Batangas Grand Terminal <> Newport World Resorts (NAIA Terminal 3) ----
const char* const BATANGAS_TO_NEWPORT[] = {"02:00", "04:00", "05:00", "07:00", "08:00", "10:00",
                                           "11:30", "13:00", "15:30", "17:30", "19:00", "20:30"};
const char* const NEWPORT_TO_BATANGAS[] = {"05:00", "07:00", "08:30", "10:00", "12:00", "13:30",
                                           "15:00", "17:00", "19:30", "21:00", "22:30", "23:45"};
// ---- RRCG DAILY SCHEDULE: SM Lipa City <> Newport World Resorts (NAIA Terminal 3) ----
const char* const SM_LIPA_TO_NEWPORT[] = {"03:00", "04:30", "05:30", "06:30", "08:00", "09:30",
                                          "12:00", "13:30", "15:15", "16:30", "18:30", "20:00"};
const char* const NEWPORT_TO_SM_LIPA[] = {"05:30", "06:30", "07:30", "09:30", "11:30", "14:00",
                                          "15:30", "17:15", "18:30", "20:30", "22:00", "23:15"};
// ---- RRCG WEEKDAY SCHEDULE: SM Masinag <> One Ayala (SM City Masinag column; One Ayala Makati column) ----
const char* const SM_MASINAG_TO_ONE_AYALA[] = {"05:00", "05:30", "06:00", "06:30", "07:00",
                                               "07:30", "08:00", "08:30", "17:00", "18:00"};
const char* const ONE_AYALA_TO_SM_MASINAG[] = {"15:00", "16:30", "17:00", "17:30", "18:00", "18:30",
                                               "19:00", "19:30", "20:00", "20:30", "21:00"};
// ---- RRCG Sierra Valley (Cainta) <> One Ayala: MONDAY & FRIDAY and TUESDAY TO THURSDAY boards ----
const char* const SIERRA_VALLEY_TO_ONE_AYALA_MON_FRI[] = {"05:30", "06:00", "06:30", "07:00"};
const char* const SIERRA_VALLEY_TO_ONE_AYALA_TUE_THU[] = {"05:30", "06:00", "06:30", "07:00"};
const char* const ONE_AYALA_TO_SIERRA_VALLEY_MON_FRI[] = {"17:30", "18:00", "19:00", "20:00"};
const char* const ONE_AYALA_TO_SIERRA_VALLEY_TUE_THU[] = {"17:30", "18:00", "18:30", "19:30", "20:00"};

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

struct Rule {
  uint8_t mask;
  Minutes departures;
};

struct Direction {
  const char* id;
  size_t index;
  const char* origin;
  const char* destination;
  std::vector<Rule> rules;
};

std::vector<Direction> directions() {
  return {
      {"p2p-batangas-grand-newport", 42, "BATANGAS", "NEWPORT", {{ALL_DAYS, board(BATANGAS_TO_NEWPORT)}}},
      {"p2p-newport-batangas-grand", 43, "NEWPORT", "BATANGAS", {{ALL_DAYS, board(NEWPORT_TO_BATANGAS)}}},
      {"p2p-sm-lipa-newport", 44, "SM LIPA", "NEWPORT", {{ALL_DAYS, board(SM_LIPA_TO_NEWPORT)}}},
      {"p2p-newport-sm-lipa", 45, "NEWPORT", "SM LIPA", {{ALL_DAYS, board(NEWPORT_TO_SM_LIPA)}}},
      {"p2p-sm-masinag-one-ayala", 46, "SM MASINAG", "ONE AYALA", {{MONDAY_TO_FRIDAY, board(SM_MASINAG_TO_ONE_AYALA)}}},
      {"p2p-one-ayala-sm-masinag", 47, "ONE AYALA", "SM MASINAG", {{MONDAY_TO_FRIDAY, board(ONE_AYALA_TO_SM_MASINAG)}}},
      {"p2p-sierra-valley-one-ayala",
       48,
       "SIERRA VALLEY",
       "ONE AYALA",
       {{MONDAY_AND_FRIDAY, board(SIERRA_VALLEY_TO_ONE_AYALA_MON_FRI)},
        {TUESDAY_TO_THURSDAY, board(SIERRA_VALLEY_TO_ONE_AYALA_TUE_THU)}}},
      {"p2p-one-ayala-sierra-valley",
       49,
       "ONE AYALA",
       "SIERRA VALLEY",
       {{MONDAY_AND_FRIDAY, board(ONE_AYALA_TO_SIERRA_VALLEY_MON_FRI)},
        {TUESDAY_TO_THURSDAY, board(ONE_AYALA_TO_SIERRA_VALLEY_TUE_THU)}}},
  };
}

Minutes departuresFor(const Schedule& schedule, const uint8_t mask) {
  for (size_t r = 0; r < schedule.serviceRuleCount; ++r) {
    const on_point::ServiceRule& rule = schedule.serviceRules[r];
    if (rule.weekdayMask == mask)
      return Minutes(rule.departures.minutes, rule.departures.minutes + rule.departures.count);
  }
  return {};
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

std::string nextAt(const Schedule& s, const CivilDateTime& when) {
  const auto state = on_point::getDepartureState(s, when);
  char text[6];
  snprintf(text, sizeof(text), "%02u:%02u", state.next.local.hour, state.next.local.minute);
  return text;
}

// ---- Catalogue shape ----

TEST(OnPointM5Data, CatalogueGrewByExactlyFourAppendedPairs) {
  EXPECT_EQ(on_point::SCHEDULE_COUNT, 50u);
  EXPECT_EQ(on_point::ROUTE_COUNT, 25u);
  EXPECT_EQ(on_point::routeCount(), 25u);
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
      {21, "p2p-batangas-grand-newport", 42, 43, "p2p-newport-batangas-grand", "BATANGAS", "PASAY"},
      {22, "p2p-sm-lipa-newport", 44, 45, "p2p-newport-sm-lipa", "BATANGAS", "PASAY"},
      {23, "p2p-sm-masinag-one-ayala", 46, 47, "p2p-one-ayala-sm-masinag", "RIZAL", "MAKATI"},
      {24, "p2p-sierra-valley-one-ayala", 48, 49, "p2p-one-ayala-sierra-valley", "RIZAL", "MAKATI"},
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
    EXPECT_LE(strlen(pair.returnId), 47u);  // fits the store's 48-byte ID field
  }
}

// ---- Every direction against the operator boards ----

TEST(OnPointM5Data, EveryDirectionMatchesTheOperatorBoards) {
  for (const Direction& d : directions()) {
    SCOPED_TRACE(d.id);
    const Schedule& s = on_point::scheduleAt(d.index);
    ASSERT_STREQ(s.routeId, d.id);
    EXPECT_TRUE(on_point::isValidSchedule(s));
    EXPECT_STREQ(s.origin, d.origin);
    EXPECT_STREQ(s.destination, d.destination);
    ASSERT_EQ(s.serviceRuleCount, d.rules.size());
    uint8_t covered = 0;
    for (size_t r = 0; r < s.serviceRuleCount; ++r) {
      EXPECT_EQ(s.serviceRules[r].weekdayMask, d.rules[r].mask) << "rule " << r;
      EXPECT_EQ(departuresFor(s, d.rules[r].mask), d.rules[r].departures) << "rule " << r;
      EXPECT_EQ(covered & s.serviceRules[r].weekdayMask, 0) << "rules overlap";
      covered |= s.serviceRules[r].weekdayMask;
    }
    // Batangas and Lipa run every day; the Rizal routes run Monday to Friday only.
    EXPECT_EQ(covered, d.index <= 45 ? ALL_DAYS : MONDAY_TO_FRIDAY);
  }
}

TEST(OnPointM5Data, SierraValleyKeepsItsTwoOperatorBoardPatterns) {
  const Schedule& out = on_point::scheduleAt(48);
  const Schedule& back = on_point::scheduleAt(49);
  // Mornings: the same four trips on both boards. Evenings: four on Monday & Friday, five Tuesday to Thursday.
  EXPECT_EQ(departuresFor(out, MONDAY_AND_FRIDAY), departuresFor(out, TUESDAY_TO_THURSDAY));
  EXPECT_EQ(departuresFor(back, MONDAY_AND_FRIDAY).size(), 4u);
  EXPECT_EQ(departuresFor(back, TUESDAY_TO_THURSDAY).size(), 5u);
  EXPECT_NE(departuresFor(back, MONDAY_AND_FRIDAY), departuresFor(back, TUESDAY_TO_THURSDAY));
  // From One Ayala at 18:15: Monday and Friday next 19:00; Tuesday to Thursday next 18:30.
  EXPECT_EQ(nextAt(back, manila(2026, 11, 23, 18, 15)), "19:00");  // Monday
  EXPECT_EQ(nextAt(back, manila(2026, 11, 24, 18, 15)), "18:30");  // Tuesday
  EXPECT_EQ(nextAt(back, manila(2026, 11, 25, 18, 15)), "18:30");  // Wednesday
  EXPECT_EQ(nextAt(back, manila(2026, 11, 26, 18, 15)), "18:30");  // Thursday
  EXPECT_EQ(nextAt(back, manila(2026, 11, 27, 18, 15)), "19:00");  // Friday
  // 19:10: Thursday 19:30 (Tuesday to Thursday only) against Friday 20:00.
  EXPECT_EQ(nextAt(back, manila(2026, 11, 26, 19, 10)), "19:30");
  EXPECT_EQ(nextAt(back, manila(2026, 11, 27, 19, 10)), "20:00");
}

TEST(OnPointM5Data, MetadataIsProvisionalAndDocumentsSourcesAndConflicts) {
  for (const Direction& d : directions()) {
    SCOPED_TRACE(d.id);
    const Schedule& s = on_point::scheduleAt(d.index);
    EXPECT_STREQ(s.routeName, "RRCG");
    EXPECT_STREQ(s.timezone, "Asia/Manila");
    EXPECT_EQ(s.utcOffsetMinutes, 480);
    const int day = d.index <= 43 ? 21 : (d.index <= 45 ? 28 : 8);
    EXPECT_EQ(s.effectiveDate.year, 2026);
    EXPECT_EQ(s.effectiveDate.month, d.index <= 45 ? 10 : 11);
    EXPECT_EQ(s.effectiveDate.day, day);
    EXPECT_EQ(s.sourceStatus, SourceStatus::Provisional);
    EXPECT_EQ(s.dateOverrides, nullptr);  // no holiday or special-date overrides
    EXPECT_EQ(s.dateOverrideCount, 0u);
    ASSERT_NE(s.notes, nullptr);
    const std::string notes = s.notes;
    EXPECT_NE(notes.find("Holiday service may differ."), std::string::npos);
    if (d.index <= 43) {
      for (const char* f :
           {"RRCG DAILY board, posted 2026-10-21 (owner-reported)", "equals the 2026-11-01/02 board",
            "RRCG website feed (undated) lists 10 different trips: unresolved", "NAIA Terminal 3 via Runway Manila"})
        EXPECT_NE(notes.find(f), std::string::npos) << f;
      EXPECT_NE(notes.find(d.index == 42 ? "Batangas Grand Terminal to Newport World Resorts"
                                         : "to Batangas Grand Terminal / Pier"),
                std::string::npos);
    } else if (d.index <= 45) {
      for (const char* f : {"RRCG DAILY board, posted 2026-10-28 (owner-reported)", "equals the 2026-11-01/02 board",
                            "RRCG website feed (undated) lists 9 different trips: unresolved", "SM Lipa City"})
        EXPECT_NE(notes.find(f), std::string::npos) << f;
    } else {
      EXPECT_NE(notes.find("RRCG: no operations 2026-11-16 to 18 (ASEAN Summit; not encoded)."), std::string::npos);
      if (d.index <= 47) {
        for (const char* f : {"RRCG WEEKDAY board (undated, seen 2026-11-08) equals the RRCG website feed",
                              "SM City Masinag", "Monday to Friday."})
          EXPECT_NE(notes.find(f), std::string::npos) << f;
      } else {
        for (const char* f : {"Monday & Friday", "Tuesday to Thursday",
                              "website feed has Tuesday to Thursday only; Monday and Friday rest on the operator board",
                              "Sierra Valley (Cainta)"})
          EXPECT_NE(notes.find(f), std::string::npos) << f;
      }
    }
  }
}

// ---- Weekend and no-service behaviour ----

TEST(OnPointM5Data, WeekendsRunOnlyOnTheDailyRoutes) {
  for (const int day : {21, 22}) {  // Saturday and Sunday, November 2026
    SCOPED_TRACE(day);
    EXPECT_EQ(nextAt(on_point::scheduleAt(42), manila(2026, 11, day, 6, 0)), "07:00");    // Batangas -> Newport
    EXPECT_EQ(nextAt(on_point::scheduleAt(45), manila(2026, 11, day, 22, 30)), "23:15");  // Newport -> SM Lipa
    for (const size_t index : {46u, 47u, 48u, 49u})
      EXPECT_EQ(on_point::getDepartureState(on_point::scheduleAt(index), manila(2026, 11, day, 9, 0)).serviceStatus,
                ServiceStatus::NoServiceToday)
          << index;
  }
}

TEST(OnPointM5Data, HolidaysAndTheAseanDaysUseTheOrdinaryLists) {
  // No overrides: Bonifacio Day (Monday) and an ASEAN Summit day (Tuesday) keep the ordinary day's list.
  EXPECT_EQ(nextAt(on_point::scheduleAt(46), manila(2026, 11, 30, 4, 50)), "05:00");
  EXPECT_EQ(nextAt(on_point::scheduleAt(49), manila(2026, 11, 17, 18, 15)), "18:30");
  EXPECT_EQ(nextAt(on_point::scheduleAt(43), manila(2026, 11, 30, 23, 0)), "23:45");
}

// ---- Collection, and exact membership of every earlier collection ----

TEST(OnPointM5Data, SouthLuzonIsAppendedAndEarlierCollectionsAreExact) {
  ASSERT_EQ(on_point::regionCount(), 6u);
  struct Expected {
    const char* id;
    std::vector<int> members;
    const char* defaultRoute;
  };
  const Expected expected[] = {
      {"bulacan", {0, 1, 7, 8, 9, 10}, "p2p-balagtas-trinoma"},
      {"north-luzon", {2, 4, 5, 12, 13, 14}, "p2p-cubao-clark-airport"},
      {"airport-transfers", {3, 6, 11}, "p2p-clark-airport-naia-t3"},
      {"metro-manila", {15, 16, 17}, "p2p-atc-greenbelt"},
      {"laguna", {18, 19, 20}, "p2p-nuvali-one-ayala"},
      {"south-luzon", {21, 22, 23, 24}, "p2p-batangas-grand-newport"},
  };
  for (size_t r = 0; r < std::size(expected); ++r) {
    SCOPED_TRACE(expected[r].id);
    EXPECT_EQ(on_point::regionIndexForId(expected[r].id), r);
    const on_point::Region& region = *on_point::regionAt(r);
    EXPECT_STREQ(region.regionId, expected[r].id);
    EXPECT_EQ(std::vector<int>(region.routes, region.routes + region.routeCount), expected[r].members);
    EXPECT_STREQ(region.defaultRouteId, expected[r].defaultRoute);
  }
}

// ---- The M4 catalogue (schedules 0-41, routes 0-20) is untouched ----

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

uint64_t catalogueDigest(const size_t schedules, const size_t routes) {
  Fnv f;
  for (size_t i = 0; i < schedules; ++i) {
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
  for (size_t i = 0; i < routes; ++i) {
    const on_point::RoutePair& r = on_point::routeAt(i);
    f.str(r.routeId);
    f.num(r.outboundScheduleIndex);
    f.num(r.returnScheduleIndex);
    f.str(r.originArea);
    f.str(r.destinationArea);
  }
  return f.h;
}

TEST(OnPointM5Data, M4CatalogueIsUnchanged) {
  // Computed with this function over the reviewed M4 checkpoint (be9a6fee).
  constexpr uint64_t M4_DIGEST = 0x8abd69e16b1e22d2ull;
  EXPECT_EQ(catalogueDigest(42, 21), M4_DIGEST) << std::hex << "0x" << catalogueDigest(42, 21);
}

// ---- Labels fit, with the firmware's fonts and layout ----

int width(const EpdFontData& data, const char* text) {
  const EpdFont font(&data);
  int w = 0, h = 0;
  font.getTextDimensions(text, &w, &h);
  return w;
}

TEST(OnPointM5Data, SouthLuzonLabelsFitTheBoardTheListAndTheMainRoute) {
  constexpr int SCREEN_WIDTH = 480, MARGIN = 24, ICON_GAP = 8, ICON_WIDTH = 26, CLEARANCE = 16;
  const int boardOrigin =
      SCREEN_WIDTH - MARGIN - width(ubuntu_10_bold, "REVERSE") - ICON_GAP - ICON_WIDTH - CLEARANCE - MARGIN;
  const int list = SCREEN_WIDTH - MARGIN - (MARGIN + 24);
  const int main = SCREEN_WIDTH - MARGIN - (MARGIN + 28);
  ASSERT_EQ(boardOrigin, 289);
  for (const char* label : {"BATANGAS", "NEWPORT", "SM LIPA", "SM MASINAG", "ONE AYALA", "SIERRA VALLEY"}) {
    EXPECT_LE(width(notosans_16_bold, label), boardOrigin) << label;
    EXPECT_LE(width(notosans_16_bold, label), list) << label;
    EXPECT_LE(width(notosans_18_bold, label), main) << label;
  }
  EXPECT_GT(width(notosans_16_bold, "BATANGAS GRAND"), boardOrigin);  // why the label is BATANGAS
}

}  // namespace
