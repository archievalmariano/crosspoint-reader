// M4 (LAGUNA) catalogue data integrity. The lists below are typed directly from the operator boards (Nuvali official
// post 2025-11-12; Calamba P2P poster posted 2026-08-31), not from the transcription file that generated
// OnPointScheduleData.cpp, so a slip in either copy fails. Also covers Nuvali's no-service holidays, the shared
// calendar's regional exceptions, Bulacan's unchanged holidays, the new collection and the earlier catalogue.

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "EpdFont.h"
#include "activities/on_point/OnPointRegions.h"
#include "activities/on_point/OnPointSchedule.h"
#include "activities/on_point/OnPointScheduleData.h"
#include "builtinFonts/notosans_16_bold.h"
#include "builtinFonts/notosans_18_bold.h"
#include "builtinFonts/ubuntu_10_bold.h"
#include "util/MarkedDates.h"

namespace {

using on_point::CivilDateTime;
using on_point::Schedule;
using on_point::ServiceStatus;
using on_point::SourceStatus;

constexpr uint8_t SUNDAY = 1u << 0;
constexpr uint8_t SATURDAY = 1u << 6;
constexpr uint8_t MONDAY_TO_FRIDAY = 0b0111110;

// ---- Nuvali official boards, "NEW SCHEDULE UNTIL FURTHER NOTICE", posted 2025-11-12 ----

// NUVALI TO MAKATI, Monday to Friday (7:00 PM is printed twice; stored once)
const char* const NUVALI_OUT_WEEKDAYS[] = {"05:00", "05:30", "06:00", "06:20", "06:40", "07:00", "07:20", "07:40",
                                           "08:00", "08:30", "09:00", "10:00", "11:00", "12:00", "13:00", "14:00",
                                           "15:00", "16:00", "17:00", "18:00", "19:00", "20:00"};
// NUVALI TO MAKATI, Saturday
const char* const NUVALI_OUT_SATURDAYS[] = {"06:00", "07:00", "08:00", "09:00", "09:40", "10:10", "11:00", "12:00",
                                            "13:00", "14:00", "15:00", "16:00", "17:00", "18:00", "19:00"};
// NUVALI TO MAKATI, Sunday
const char* const NUVALI_OUT_SUNDAYS[] = {"06:00", "07:00", "08:00", "09:00", "09:40", "10:10", "11:00",
                                          "12:00", "13:00", "14:00", "15:00", "16:00", "17:00", "18:00"};
// ONE AYALA TO NUVALI, Monday to Friday
const char* const ONE_AYALA_BACK_WEEKDAYS[] = {"07:00", "08:00", "08:40", "09:10", "10:00", "11:00", "12:00", "13:00",
                                               "14:00", "15:00", "16:00", "16:30", "17:00", "17:30", "18:00", "18:30",
                                               "19:00", "19:30", "20:00", "20:30", "21:00", "22:00"};
// ONE AYALA TO NUVALI, Saturday (12:00 is printed in both the morning and afternoon panels; stored once)
const char* const ONE_AYALA_BACK_SATURDAYS[] = {"08:00", "09:00", "10:00", "11:00", "12:00", "13:00", "14:00", "15:00",
                                                "16:00", "17:00", "18:00", "19:00", "19:30", "20:00", "21:00"};
// ONE AYALA TO NUVALI, Sunday
const char* const ONE_AYALA_BACK_SUNDAYS[] = {"08:00", "09:00", "10:00", "11:00", "12:00", "13:00", "14:00",
                                              "15:00", "16:00", "17:00", "18:00", "19:00", "19:30", "20:00"};

// ---- WalterMart Cabuyao poster ("Regular trips will resume on Tuesday September 1, 2026"), Monday to Friday ----

const char* const CABUYAO_TO_MAKATI[] = {"05:00", "06:00", "07:00", "08:00", "10:00",
                                         "12:00", "14:00", "15:00", "17:00", "18:30"};
const char* const CABUYAO_TO_BGC[] = {"05:00", "06:00", "07:00", "08:00", "10:00",
                                      "12:00", "14:00", "15:00", "17:00", "19:00"};
const char* const MAKATI_TO_CABUYAO[] = {"06:00", "08:00", "09:00", "10:00", "12:00",
                                         "14:00", "17:00", "18:00", "19:00", "20:00"};
const char* const BGC_TO_CABUYAO[] = {"06:00", "08:00", "09:00", "10:00", "12:00",
                                      "14:00", "17:00", "18:00", "19:00", "21:00"};

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
  Minutes saturdays;  // empty: no service
  Minutes sundays;    // empty: no service
};

std::vector<Direction> directions() {
  return {
      {"p2p-nuvali-one-ayala", 36, "NUVALI", "ONE AYALA", board(NUVALI_OUT_WEEKDAYS), board(NUVALI_OUT_SATURDAYS),
       board(NUVALI_OUT_SUNDAYS)},
      {"p2p-one-ayala-nuvali", 37, "ONE AYALA", "NUVALI", board(ONE_AYALA_BACK_WEEKDAYS),
       board(ONE_AYALA_BACK_SATURDAYS), board(ONE_AYALA_BACK_SUNDAYS)},
      {"p2p-cabuyao-makati", 38, "WM CABUYAO", "MAKATI", board(CABUYAO_TO_MAKATI), {}, {}},
      {"p2p-makati-cabuyao", 39, "MAKATI", "WM CABUYAO", board(MAKATI_TO_CABUYAO), {}, {}},
      {"p2p-cabuyao-bgc", 40, "WM CABUYAO", "MARKET MARKET", board(CABUYAO_TO_BGC), {}, {}},
      {"p2p-bgc-cabuyao", 41, "MARKET MARKET", "WM CABUYAO", board(BGC_TO_CABUYAO), {}, {}},
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

using Date = std::tuple<int, int, int>;

std::set<Date> overrideDates(const Schedule& s) {
  std::set<Date> dates;
  for (size_t i = 0; i < s.dateOverrideCount; ++i)
    dates.insert({s.dateOverrides[i].date.year, s.dateOverrides[i].date.month, s.dateOverrides[i].date.day});
  return dates;
}

// ---- Catalogue shape ----

TEST(OnPointM4Data, CatalogueGrewByExactlyThreeAppendedPairs) {
  EXPECT_EQ(on_point::SCHEDULE_COUNT, 42u);
  EXPECT_EQ(on_point::ROUTE_COUNT, 21u);
  EXPECT_EQ(on_point::routeCount(), 21u);
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
      {18, "p2p-nuvali-one-ayala", 36, 37, "p2p-one-ayala-nuvali", "LAGUNA", "MAKATI"},
      {19, "p2p-cabuyao-makati", 38, 39, "p2p-makati-cabuyao", "LAGUNA", "MAKATI"},
      {20, "p2p-cabuyao-bgc", 40, 41, "p2p-bgc-cabuyao", "LAGUNA", "TAGUIG"},
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

// ---- Every direction against the boards ----

TEST(OnPointM4Data, EveryDirectionMatchesTheOperatorBoards) {
  for (const Direction& d : directions()) {
    SCOPED_TRACE(d.id);
    const Schedule& s = on_point::scheduleAt(d.index);
    ASSERT_STREQ(s.routeId, d.id);
    EXPECT_TRUE(on_point::isValidSchedule(s));
    EXPECT_STREQ(s.origin, d.origin);
    EXPECT_STREQ(s.destination, d.destination);
    EXPECT_EQ(departuresFor(s, MONDAY_TO_FRIDAY), d.weekdays);
    EXPECT_EQ(departuresFor(s, SATURDAY), d.saturdays);
    EXPECT_EQ(departuresFor(s, SUNDAY), d.sundays);
    const bool weekend = !d.saturdays.empty();
    EXPECT_EQ(s.serviceRuleCount, weekend ? 3u : 1u);
    uint8_t covered = 0;
    for (size_t r = 0; r < s.serviceRuleCount; ++r) {
      EXPECT_EQ(covered & s.serviceRules[r].weekdayMask, 0) << "rules overlap";
      covered |= s.serviceRules[r].weekdayMask;
    }
    EXPECT_EQ(covered, weekend ? 0b1111111 : MONDAY_TO_FRIDAY);
  }
}

TEST(OnPointM4Data, TripCountsMatchTheBoards) {
  const auto v = directions();
  const size_t expected[][3] = {{22, 15, 14}, {22, 15, 14}, {10, 0, 0}, {10, 0, 0}, {10, 0, 0}, {10, 0, 0}};
  for (size_t i = 0; i < v.size(); ++i) {
    EXPECT_EQ(v[i].weekdays.size(), expected[i][0]) << v[i].id;
    EXPECT_EQ(v[i].saturdays.size(), expected[i][1]) << v[i].id;
    EXPECT_EQ(v[i].sundays.size(), expected[i][2]) << v[i].id;
  }
}

TEST(OnPointM4Data, MetadataIsProvisionalAndDocumentsSourcesStopsAndHolidays) {
  for (const Direction& d : directions()) {
    SCOPED_TRACE(d.id);
    const Schedule& s = on_point::scheduleAt(d.index);
    const bool nuvali = d.index <= 37;
    EXPECT_STREQ(s.routeName, nuvali ? "TAS Trans" : "Calamba P2P");
    EXPECT_STREQ(s.timezone, "Asia/Manila");
    EXPECT_EQ(s.utcOffsetMinutes, 480);
    EXPECT_EQ(s.effectiveDate.year, nuvali ? 2025 : 2026);
    EXPECT_EQ(s.effectiveDate.month, nuvali ? 11 : 9);
    EXPECT_EQ(s.effectiveDate.day, nuvali ? 12 : 1);
    EXPECT_EQ(s.sourceStatus, SourceStatus::Provisional);
    ASSERT_NE(s.notes, nullptr);
    const std::string notes = s.notes;
    if (nuvali) {
      for (const char* fragment : {"Nuvali official post 2025-11-12 (TAS Trans Corp.)", "'until further notice'",
                                   "One Ayala, then Circuit Makati except the last trip",
                                   "our reading of 'HOLIDAYS - NO OPERATIONS'", "Eid and later years not covered"})
        EXPECT_NE(notes.find(fragment), std::string::npos) << fragment;
    } else {
      for (const char* fragment : {"Calamba P2P post 2026-08-31, effective 2026-09-01", "WalterMart Cabuyao",
                                   "Monday to Friday", "Holiday service may differ."})
        EXPECT_NE(notes.find(fragment), std::string::npos) << fragment;
      EXPECT_NE(notes.find(d.index <= 39 ? "Makati (One Ayala / Landmark)" : "BGC Taguig (Market Market)"),
                std::string::npos);
      const char* exitRule = (d.index == 38 || d.index == 40)
                                 ? "06:00, 07:00 and 08:00 via Mayapa Exit, other trips via Pulo Exit"
                                 : " to WalterMart Cabuyao: every trip via Pulo Exit";
      EXPECT_NE(notes.find(exitRule), std::string::npos);
      EXPECT_EQ(s.dateOverrides, nullptr);  // no holiday overrides without operator evidence
      EXPECT_EQ(s.dateOverrideCount, 0u);
    }
  }
}

// ---- Nuvali: no service on nationwide holidays ----

TEST(OnPointM4Data, NuvaliHolidaysAreExactlyTheNationwideNonWorkingDays) {
  std::set<Date> expected;
  for (const marked_dates::MarkedDate& d : marked_dates::PHILIPPINES_2026)
    if ((d.kind == marked_dates::Kind::RegularHoliday || d.kind == marked_dates::Kind::SpecialNonWorking) &&
        d.scope == marked_dates::Scope::Nationwide)
      expected.insert({d.year, d.month, d.day});
  for (const marked_dates::MarkedDate& d : marked_dates::PHILIPPINES_2027)
    if ((d.kind == marked_dates::Kind::RegularHoliday || d.kind == marked_dates::Kind::SpecialNonWorking) &&
        d.scope == marked_dates::Scope::Nationwide)
      expected.insert({d.year, d.month, d.day});
  ASSERT_EQ(expected.size(), 36u);

  for (const size_t index : {36u, 37u}) {
    const Schedule& s = on_point::scheduleAt(index);
    SCOPED_TRACE(s.routeId);
    ASSERT_EQ(s.dateOverrideCount, 36u);
    EXPECT_EQ(overrideDates(s), expected);
    for (size_t i = 0; i < s.dateOverrideCount; ++i) {
      EXPECT_EQ(s.dateOverrides[i].departures.count, 0u);  // an empty list: no service that day
      EXPECT_EQ(s.dateOverrides[i].departures.minutes, nullptr);
    }
    const auto dates = overrideDates(s);
    // Included: regular holidays, special non-working days (our reading of "HOLIDAYS"), weekend holidays.
    for (const Date& d : {Date{2026, 1, 1}, Date{2026, 11, 30}, Date{2026, 12, 24}, Date{2026, 12, 31},
                          Date{2027, 3, 27}, Date{2027, 12, 30}})
      EXPECT_EQ(dates.count(d), 1u) << std::get<0>(d) << "-" << std::get<1>(d) << "-" << std::get<2>(d);
    // Excluded: Metro Manila-only days and special working days.
    for (const Date& d :
         {Date{2026, 11, 16}, Date{2026, 11, 17}, Date{2026, 11, 18}, Date{2026, 2, 25}, Date{2027, 2, 25}})
      EXPECT_EQ(dates.count(d), 0u) << std::get<0>(d) << "-" << std::get<1>(d) << "-" << std::get<2>(d);
    // Nothing outside the supported calendar.
    for (const Date& d : dates) EXPECT_TRUE(std::get<0>(d) == 2026 || std::get<0>(d) == 2027);
  }
}

TEST(OnPointM4Data, NuvaliBoardOnHolidaysRegionalDaysAndWorkingDays) {
  const Schedule& out = on_point::scheduleAt(36);
  // Bonifacio Day (Monday) and Black Saturday 2027: no service.
  EXPECT_EQ(on_point::getDepartureState(out, manila(2026, 11, 30, 9, 0)).serviceStatus, ServiceStatus::NoServiceToday);
  EXPECT_EQ(on_point::getDepartureState(out, manila(2027, 3, 27, 9, 0)).serviceStatus, ServiceStatus::NoServiceToday);
  EXPECT_EQ(on_point::getDepartureState(on_point::scheduleAt(37), manila(2026, 12, 25, 9, 0)).serviceStatus,
            ServiceStatus::NoServiceToday);
  // A Metro Manila-only day (Tuesday 2026-11-17) and the EDSA special working day: the ordinary weekday list.
  for (const auto& day : {std::array<int, 3>{2026, 11, 17}, std::array<int, 3>{2027, 2, 25}}) {
    const auto state = on_point::getDepartureState(out, manila(day[0], day[1], day[2], 6, 30));
    EXPECT_EQ(state.next.local.hour, 6) << day[1] << "-" << day[2];
    EXPECT_EQ(state.next.local.minute, 40) << day[1] << "-" << day[2];  // weekday-only 06:40
  }
  // An ordinary Saturday and Sunday.
  auto state = on_point::getDepartureState(out, manila(2026, 11, 21, 9, 30));
  EXPECT_EQ(state.next.local.hour, 9);
  EXPECT_EQ(state.next.local.minute, 40);
  state = on_point::getDepartureState(out, manila(2026, 11, 22, 12, 0));
  EXPECT_EQ(state.last.local.hour, 18);
  EXPECT_EQ(state.last.local.minute, 0);
}

// ---- Cabuyao: weekdays only, no holiday overrides ----

TEST(OnPointM4Data, CabuyaoRunsWeekdaysOnlyWithNoHolidayOverrides) {
  for (const size_t index : {38u, 39u, 40u, 41u}) {
    const Schedule& s = on_point::scheduleAt(index);
    SCOPED_TRACE(s.routeId);
    EXPECT_EQ(on_point::getDepartureState(s, manila(2026, 11, 21, 9, 0)).serviceStatus, ServiceStatus::NoServiceToday);
    EXPECT_EQ(on_point::getDepartureState(s, manila(2026, 11, 22, 9, 0)).serviceStatus, ServiceStatus::NoServiceToday);
    // Bonifacio Day: the operator states no rule, so the ordinary weekday list applies.
    EXPECT_NE(on_point::getDepartureState(s, manila(2026, 11, 30, 4, 30)).serviceStatus, ServiceStatus::NoServiceToday);
  }
  const auto state = on_point::getDepartureState(on_point::scheduleAt(41), manila(2026, 11, 25, 20, 30));
  EXPECT_EQ(state.next.local.hour, 21);  // BGC -> Cabuyao's last trip, 21:00
  EXPECT_TRUE(state.isLastDeparture);
}

// ---- Bulacan keeps its holidays, including the NCR days ----

TEST(OnPointM4Data, BulacanHolidaysAreUnchanged) {
  const Schedule& balagtas = on_point::scheduleAt(0);
  ASSERT_STREQ(balagtas.routeId, "p2p-balagtas-trinoma");
  ASSERT_EQ(balagtas.dateOverrideCount, 39u);  // 21 for 2026, NCR ASEAN days included, plus 18 for 2027
  const auto dates = overrideDates(balagtas);
  for (const Date& d : {Date{2026, 11, 16}, Date{2026, 11, 17}, Date{2026, 11, 18}}) EXPECT_EQ(dates.count(d), 1u);
  for (size_t i = 0; i < balagtas.dateOverrideCount; ++i) EXPECT_GT(balagtas.dateOverrides[i].departures.count, 0u);
}

// ---- Collection ----

TEST(OnPointM4Data, LagunaIsAppendedWithTheThreePairs) {
  ASSERT_EQ(on_point::regionCount(), 5u);
  EXPECT_EQ(on_point::regionIndexForId("laguna"), 4u);
  const on_point::Region* laguna = on_point::regionAt(4);
  ASSERT_NE(laguna, nullptr);
  EXPECT_EQ(std::vector<int>(laguna->routes, laguna->routes + laguna->routeCount), (std::vector<int>{18, 19, 20}));
  EXPECT_STREQ(laguna->defaultRouteId, "p2p-nuvali-one-ayala");
  const char* const earlier[] = {"bulacan", "north-luzon", "airport-transfers", "metro-manila"};
  for (size_t r = 0; r < 4; ++r) {
    EXPECT_EQ(on_point::regionIndexForId(earlier[r]), r);
    const on_point::Region& region = *on_point::regionAt(r);
    for (size_t i = 0; i < region.routeCount; ++i) EXPECT_LT(region.routes[i], 18) << region.regionId;
  }
}

// ---- The M3-A catalogue (schedules 0-35, routes 0-17) is untouched ----

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

TEST(OnPointM4Data, M3ACatalogueIsUnchanged) {
  // Computed with this function over the Codex-cleared M3-A checkpoint (a526200a).
  constexpr uint64_t M3A_DIGEST = 0xa9dae901c5209659ull;
  EXPECT_EQ(catalogueDigest(36, 18), M3A_DIGEST) << std::hex << "0x" << catalogueDigest(36, 18);
}

// ---- Labels fit, with the firmware's fonts and layout ----

int width(const EpdFontData& data, const char* text) {
  const EpdFont font(&data);
  int w = 0, h = 0;
  font.getTextDimensions(text, &w, &h);
  return w;
}

TEST(OnPointM4Data, LagunaLabelsFitTheBoardTheListAndTheMainRoute) {
  constexpr int SCREEN_WIDTH = 480, MARGIN = 24, ICON_GAP = 8, ICON_WIDTH = 26, CLEARANCE = 16;
  const int boardOrigin =
      SCREEN_WIDTH - MARGIN - width(ubuntu_10_bold, "REVERSE") - ICON_GAP - ICON_WIDTH - CLEARANCE - MARGIN;
  const int list = SCREEN_WIDTH - MARGIN - (MARGIN + 24);
  const int main = SCREEN_WIDTH - MARGIN - (MARGIN + 28);
  ASSERT_EQ(boardOrigin, 289);
  for (const char* label : {"NUVALI", "ONE AYALA", "WM CABUYAO", "MAKATI", "MARKET MARKET"}) {
    EXPECT_LE(width(notosans_16_bold, label), boardOrigin) << label;
    EXPECT_LE(width(notosans_16_bold, label), list) << label;
    EXPECT_LE(width(notosans_18_bold, label), main) << label;
  }
  EXPECT_EQ(width(notosans_16_bold, "MARKET MARKET"), 281);  // the tightest: 8 px to spare beside REVERSE
}

}  // namespace
