// M2 (Genesis) catalogue data integrity. Every board below is an independent "HH:MM" transcription of the
// operator's posted times, kept per board, so the combinations the timetables rely on (classes, Thursday-to-Monday
// trips, the Baguio to Pasay lists, PITX-bound trips added to Clark to NAIA) are computed here and a slip in either
// copy fails.

#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <iterator>
#include <set>
#include <string>
#include <vector>

#include "activities/on_point/OnPointRegions.h"
#include "activities/on_point/OnPointSchedule.h"
#include "activities/on_point/OnPointScheduleData.h"

namespace {

using on_point::CivilDateTime;
using on_point::Schedule;
using on_point::SourceStatus;

constexpr uint8_t SUNDAY = 1u << 0;
constexpr uint8_t MONDAY = 1u << 1;
constexpr uint8_t TUESDAY = 1u << 2;
constexpr uint8_t WEDNESDAY = 1u << 3;
constexpr uint8_t THURSDAY = 1u << 4;
constexpr uint8_t FRIDAY = 1u << 5;
constexpr uint8_t SATURDAY = 1u << 6;
constexpr uint8_t ALL_DAYS[] = {SUNDAY, MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY};

// PITX > Clark Airport, via NAIA 3 and SM Clark, as of 2026-11-04
const char* const PITX_TO_CLARK[] = {"04:30", "05:30", "07:30", "09:30", "12:00", "14:30", "16:30", "18:30"};
// Clark Airport > PITX, via SM Clark and NAIA 3, as of 2026-11-04
const char* const CLARK_TO_PITX[] = {"03:00", "04:30", "07:30", "09:00", "10:45", "12:00", "15:30", "19:00"};
// Clark Airport > NAIA 1, 2, 3, as of 2026-11-03
const char* const CLARK_TO_NAIA[] = {"00:30", "02:00", "03:45", "05:15", "06:00", "06:45", "08:00", "08:30",
                                     "09:30", "10:00", "11:30", "12:30", "13:00", "13:45", "14:30", "15:00",
                                     "16:00", "16:45", "17:30", "18:15", "20:00", "21:00", "22:30"};
// NAIA 3 > Clark Airport, as of 2026-11-03
const char* const NAIA_TO_CLARK[] = {"00:00", "01:00", "01:30", "02:30", "03:30", "04:30", "05:00", "05:30",
                                     "06:00", "06:30", "07:30", "08:00", "08:30", "09:30", "10:00", "11:00",
                                     "11:30", "12:00", "12:30", "13:00", "14:00", "15:00", "15:30", "16:00",
                                     "17:00", "18:00", "19:00", "19:30", "20:30", "21:30", "22:30"};
// Cubao > Baguio (JoyBus Premiere/Executive), as of 2026-09-18
const char* const CUBAO_BAGUIO_JOYBUS[] = {"01:00", "03:00", "05:00", "06:00", "07:00", "09:00", "11:00",
                                           "13:00", "15:00", "17:30", "19:30", "21:30", "23:30"};
// Cubao > Baguio (Deluxe), as of 2026-09-18
const char* const CUBAO_BAGUIO_DELUXE[] = {"02:00", "04:00", "08:00", "10:00", "12:00", "14:00", "16:00"};
// Baguio > Cubao, as of 2026-09-18
const char* const BAGUIO_TO_CUBAO[] = {"02:00", "04:00", "06:30", "08:30", "10:30", "12:30", "14:30",
                                       "15:30", "16:30", "17:30", "18:00", "20:00", "22:00"};
// Pasay > Baguio (JoyBus Premiere/Executive), as of 2026-09-18
const char* const PASAY_BAGUIO_JOYBUS[] = {"01:00", "03:00", "05:00", "07:00", "09:00", "11:00",
                                           "13:00", "15:00", "17:00", "19:00", "21:00"};
// Pasay > Baguio (Deluxe), as of 2026-09-18
const char* const PASAY_BAGUIO_DELUXE[] = {"02:00", "04:00", "06:00", "08:00", "10:00",
                                           "12:00", "14:00", "16:00", "23:00"};
// Baguio > NAIA 3 / Pasay, as of 2026-09-18
const char* const BAGUIO_PASAY_NAIA3[] = {"00:00", "02:30", "05:00", "06:00", "08:00", "09:00",
                                          "10:00", "12:00", "14:00", "16:00", "18:00", "23:00"};
// Baguio > NAIA 3, 2, 1 / Pasay, as of 2026-09-18
const char* const BAGUIO_PASAY_NAIA321[] = {"01:00", "04:00", "07:00", "09:00", "11:00", "13:00",
                                            "15:00", "17:00", "19:00", "20:00", "21:00", "22:00"};
// Baguio > Pasay via Avenida (panel of the Avenida poster), as of 2026-09-18
const char* const BAGUIO_PASAY_AVENIDA[] = {"23:00"};
// Clark Airport > Balanga, Bataan, as of 2026-09-23
const char* const CLARK_TO_BALANGA[] = {"05:00", "08:30", "09:30", "11:30", "12:30",
                                        "14:30", "15:30", "17:00", "18:00"};
// Balanga, Bataan > Clark Airport, as of 2026-09-23
const char* const BALANGA_TO_CLARK[] = {"05:00", "06:00", "07:00", "09:00", "10:30",
                                        "12:30", "14:30", "15:30", "17:30"};

// Trips the Deluxe boards mark "Only available from Thurs to Mon".
const char* const CUBAO_DELUXE_THURSDAY_TO_MONDAY[] = {"04:00", "08:00", "12:00", "16:00"};
const char* const PASAY_DELUXE_THURSDAY_TO_MONDAY[] = {"02:00", "06:00", "08:00", "12:00"};
// NAIA 3 > Clark rows highlighted as coming from PITX.
const char* const NAIA_TO_CLARK_FROM_PITX[] = {"05:00", "06:00", "08:00", "10:00", "12:30", "15:00", "17:00", "19:00"};

using Minutes = std::set<int>;

template <size_t N>
Minutes board(const char* const (&times)[N]) {
  Minutes out;
  for (const char* time : times) {
    const std::string t = time;
    out.insert(std::stoi(t.substr(0, 2)) * 60 + std::stoi(t.substr(3, 2)));
  }
  EXPECT_EQ(out.size(), N) << "a board repeats a time";
  return out;
}

Minutes operator+(Minutes a, const Minutes& b) {
  a.insert(b.begin(), b.end());
  return a;
}

Minutes operator-(Minutes a, const Minutes& b) {
  for (const int minute : b) a.erase(minute);
  return a;
}

const Schedule* scheduleById(const char* id) {
  for (size_t i = 0; i < on_point::SCHEDULE_COUNT; ++i) {
    if (strcmp(on_point::scheduleAt(i).routeId, id) == 0) return &on_point::scheduleAt(i);
  }
  return nullptr;
}

// The departures of the one rule covering this weekday bit.
std::vector<int> ruleFor(const Schedule& schedule, const uint8_t day) {
  std::vector<int> out;
  size_t matches = 0;
  for (size_t i = 0; i < schedule.serviceRuleCount; ++i) {
    const on_point::ServiceRule& rule = schedule.serviceRules[i];
    if ((rule.weekdayMask & day) == 0) continue;
    ++matches;
    out.assign(rule.departures.minutes, rule.departures.minutes + rule.departures.count);
  }
  EXPECT_EQ(matches, 1u) << "exactly one rule must cover each day";
  return out;
}

std::vector<int> sorted(const Minutes& minutes) { return {minutes.begin(), minutes.end()}; }

void expectEveryDay(const char* id, const Minutes& expected) {
  SCOPED_TRACE(id);
  const Schedule* schedule = scheduleById(id);
  ASSERT_NE(schedule, nullptr);
  ASSERT_EQ(schedule->serviceRuleCount, 1u);
  for (const uint8_t day : ALL_DAYS) EXPECT_EQ(ruleFor(*schedule, day), sorted(expected));
}

void expectThursdayToMondayPattern(const char* id, const Minutes& thursdayToMonday, const Minutes& tuesdayWednesday) {
  SCOPED_TRACE(id);
  const Schedule* schedule = scheduleById(id);
  ASSERT_NE(schedule, nullptr);
  ASSERT_EQ(schedule->serviceRuleCount, 2u);
  for (const uint8_t day : {THURSDAY, FRIDAY, SATURDAY, SUNDAY, MONDAY}) {
    EXPECT_EQ(ruleFor(*schedule, day), sorted(thursdayToMonday));
  }
  for (const uint8_t day : {TUESDAY, WEDNESDAY}) EXPECT_EQ(ruleFor(*schedule, day), sorted(tuesdayWednesday));
}

// Manila wall-clock on a November 2026 day (always past the 1st), as the UTC instant the clock supplies.
CivilDateTime manila(const uint8_t day, const uint8_t hour, const uint8_t minute) {
  if (hour >= 8) return {2026, 11, day, static_cast<uint8_t>(hour - 8), minute, 0};
  return {2026, 11, static_cast<uint8_t>(day - 1), static_cast<uint8_t>(hour + 16), minute, 0};
}

int nextMinute(const char* id, const CivilDateTime& now) {
  const Schedule* schedule = scheduleById(id);
  EXPECT_NE(schedule, nullptr);
  if (schedule == nullptr) return -1;
  const on_point::DepartureState state = on_point::getDepartureState(*schedule, now);
  return state.next.local.hour * 60 + state.next.local.minute;
}

constexpr int clock(const int hour, const int minute) { return hour * 60 + minute; }

TEST(OnPointM2Data, DailyDirectionsMatchTheirBoards) {
  expectEveryDay("p2p-pitx-clark-airport", board(PITX_TO_CLARK));
  expectEveryDay("p2p-clark-airport-pitx", board(CLARK_TO_PITX));
  expectEveryDay("genesis-baguio-cubao", board(BAGUIO_TO_CUBAO));
  expectEveryDay("p2p-clark-airport-balanga", board(CLARK_TO_BALANGA));
  expectEveryDay("p2p-balanga-clark-airport", board(BALANGA_TO_CLARK));
}

TEST(OnPointM2Data, BaguioOutboundCombinesClassesAndOmitsThursdayToMondayTripsOnTuesdayWednesday) {
  const Minutes cubao = board(CUBAO_BAGUIO_JOYBUS) + board(CUBAO_BAGUIO_DELUXE);
  const Minutes cubaoOnly = board(CUBAO_DELUXE_THURSDAY_TO_MONDAY);
  EXPECT_EQ(cubao.size(), 20u);
  EXPECT_EQ(cubaoOnly.size(), 4u);
  expectThursdayToMondayPattern("genesis-cubao-baguio", cubao, cubao - cubaoOnly);

  const Minutes pasay = board(PASAY_BAGUIO_JOYBUS) + board(PASAY_BAGUIO_DELUXE);
  const Minutes pasayOnly = board(PASAY_DELUXE_THURSDAY_TO_MONDAY);
  EXPECT_EQ(pasay.size(), 20u);
  EXPECT_EQ(pasayOnly.size(), 4u);
  expectThursdayToMondayPattern("genesis-pasay-baguio", pasay, pasay - pasayOnly);
}

TEST(OnPointM2Data, BaguioToPasayIsTheUnionOfThePasayBoards) {
  const Minutes naia3 = board(BAGUIO_PASAY_NAIA3);
  const Minutes naia321 = board(BAGUIO_PASAY_NAIA321);
  const Minutes avenida = board(BAGUIO_PASAY_AVENIDA);
  EXPECT_TRUE(std::includes(naia3.begin(), naia3.end(), avenida.begin(), avenida.end()))
      << "the Avenida trip is the 23:00 already on the NAIA 3 / Pasay board";
  const Minutes all = naia3 + naia321 + avenida;
  EXPECT_EQ(all.size(), 23u);
  expectEveryDay("genesis-baguio-pasay", all);
}

TEST(OnPointM2Data, ThursdayToMondayTripsFollowTheManilaDay) {
  // 04:00 (Cubao) and 02:00 (Pasay) run Thursday to Monday only. Nov 2026: Mon 9, Tue 10, Wed 11, Thu 12.
  for (const uint8_t day : {uint8_t{9}, uint8_t{12}}) {
    EXPECT_EQ(nextMinute("genesis-cubao-baguio", manila(day, 3, 30)), clock(4, 0)) << int(day);
    EXPECT_EQ(nextMinute("genesis-pasay-baguio", manila(day, 1, 30)), clock(2, 0)) << int(day);
  }
  for (const uint8_t day : {uint8_t{10}, uint8_t{11}}) {
    EXPECT_EQ(nextMinute("genesis-cubao-baguio", manila(day, 3, 30)), clock(5, 0)) << int(day);
    EXPECT_EQ(nextMinute("genesis-pasay-baguio", manila(day, 1, 30)), clock(3, 0)) << int(day);
  }
}

TEST(OnPointM2Data, NewDirectionsCarryTheirDatesLabelsAndNoHolidayRules) {
  struct Row {
    const char* id;
    const char* origin;
    const char* destination;
    uint8_t month;
    uint8_t day;
  };
  const Row rows[] = {
      {"p2p-pitx-clark-airport", "PITX", "CLARK AIRPORT", 11, 4},
      {"p2p-clark-airport-pitx", "CLARK AIRPORT", "PITX", 11, 4},
      {"genesis-cubao-baguio", "CUBAO", "BAGUIO", 9, 18},
      {"genesis-baguio-cubao", "BAGUIO", "CUBAO", 9, 18},
      {"genesis-pasay-baguio", "PASAY", "BAGUIO", 9, 18},
      {"genesis-baguio-pasay", "BAGUIO", "PASAY", 9, 18},
      {"p2p-clark-airport-balanga", "CLARK AIRPORT", "BALANGA", 9, 23},
      {"p2p-balanga-clark-airport", "BALANGA", "CLARK AIRPORT", 9, 23},
  };
  for (const Row& row : rows) {
    SCOPED_TRACE(row.id);
    const Schedule* s = scheduleById(row.id);
    ASSERT_NE(s, nullptr);
    EXPECT_TRUE(on_point::isValidSchedule(*s));
    EXPECT_STREQ(s->routeName, "Genesis");
    EXPECT_STREQ(s->origin, row.origin);
    EXPECT_STREQ(s->destination, row.destination);
    EXPECT_STREQ(s->timezone, "Asia/Manila");
    EXPECT_EQ(s->utcOffsetMinutes, 480);
    EXPECT_EQ(s->effectiveDate.year, 2026);
    EXPECT_EQ(s->effectiveDate.month, row.month);
    EXPECT_EQ(s->effectiveDate.day, row.day);
    EXPECT_EQ(s->sourceStatus, SourceStatus::Verified);
    EXPECT_EQ(s->dateOverrideCount, 0u);
  }
}

TEST(OnPointM2Data, ClarkToNaiaAddsThePitxBoundTripsThatStopAtTerminal3) {
  const Minutes naia = board(CLARK_TO_NAIA);
  const Minutes pitx = board(CLARK_TO_PITX);
  EXPECT_EQ(naia.size(), 23u);
  EXPECT_EQ(pitx.size(), 8u);
  EXPECT_EQ((naia - pitx).size(), 23u) << "the two boards must not overlap";
  const Minutes all = naia + pitx;
  EXPECT_EQ(all.size(), 31u);
  expectEveryDay("p2p-clark-airport-naia-t3", all);
  const Schedule* s = scheduleById("p2p-clark-airport-naia-t3");
  ASSERT_NE(s, nullptr);
  EXPECT_EQ(s->sourceStatus, SourceStatus::Verified);
  EXPECT_EQ(s->effectiveDate.month, 8);
  EXPECT_EQ(s->effectiveDate.day, 18);
}

TEST(OnPointM2Data, NaiaToClarkStaysAtItsThirtyOneTrips) {
  const Minutes naia = board(NAIA_TO_CLARK);
  const Minutes fromPitx = board(NAIA_TO_CLARK_FROM_PITX);
  EXPECT_EQ(naia.size(), 31u);
  EXPECT_EQ(fromPitx.size(), 8u);
  EXPECT_TRUE(std::includes(naia.begin(), naia.end(), fromPitx.begin(), fromPitx.end()))
      << "the PITX-origin trips are already part of the 31";
  expectEveryDay("p2p-naia-t3-clark-airport", naia);
}

TEST(OnPointM2Data, NaiaPitxIsProvisionalWithItsDeparturesUnchanged) {
  const Schedule* out = scheduleById("p2p-naia-pitx");
  const Schedule* back = scheduleById("p2p-pitx-naia");
  ASSERT_NE(out, nullptr);
  ASSERT_NE(back, nullptr);
  EXPECT_EQ(out->sourceStatus, SourceStatus::Provisional);
  EXPECT_EQ(back->sourceStatus, SourceStatus::Provisional);
  EXPECT_EQ(ruleFor(*out, MONDAY), (std::vector<int>{270, 375, 495, 630, 780, 885, 1005, 1125}));
  EXPECT_EQ(ruleFor(*back, MONDAY), (std::vector<int>{285, 405, 525, 660, 855, 990, 1110}));
}

TEST(OnPointM2Data, CubaoToClarkKeepsTheOneTenPmReading) {
  const Schedule* s = scheduleById("p2p-cubao-clark-airport");
  ASSERT_NE(s, nullptr);
  const std::vector<int> departures = ruleFor(*s, MONDAY);
  EXPECT_NE(std::find(departures.begin(), departures.end(), clock(13, 10)), departures.end());
  EXPECT_EQ(std::find(departures.begin(), departures.end(), clock(1, 10)), departures.end());
  EXPECT_NE(strstr(s->notes, "1:10 PM"), nullptr);
}

TEST(OnPointM2Data, TheCatalogueIsAppendOnly) {
  const char* const shippedSchedules[] = {
      "p2p-balagtas-trinoma",
      "p2p-trinoma-balagtas",
      "p2p-caypombo-sm-north-edsa",
      "p2p-sm-north-edsa-caypombo",
      "p2p-cubao-clark-airport",
      "p2p-clark-airport-cubao",
      "p2p-clark-airport-naia-t3",
      "p2p-naia-t3-clark-airport",
      "p2p-trinoma-clark-airport",
      "p2p-clark-airport-trinoma",
      "p2p-clark-airport-baguio",
      "p2p-baguio-clark-airport",
      "p2p-naia-pitx",
      "p2p-pitx-naia",
      "p2p-pandi-trinoma",
      "p2p-trinoma-pandi",
      "p2p-plaridel-trinoma",
      "p2p-trinoma-plaridel",
      "p2p-malolos-trinoma",
      "p2p-trinoma-malolos",
      "p2p-guiguinto-trinoma",
      "p2p-trinoma-guiguinto",
  };
  for (size_t i = 0; i < std::size(shippedSchedules); ++i) {
    EXPECT_STREQ(on_point::scheduleAt(i).routeId, shippedSchedules[i]) << i;
  }
  struct Pair {
    size_t route;
    const char* id;
    size_t out;
    size_t back;
    const char* originArea;
    const char* destinationArea;
  };
  const Pair added[] = {
      {11, "p2p-pitx-clark-airport", 22, 23,
       "PARA\xC3\x91"
       "AQUE",
       "PAMPANGA"},
      {12, "genesis-cubao-baguio", 24, 25, "QUEZON CITY", "BAGUIO"},
      {13, "genesis-pasay-baguio", 26, 27, "PASAY", "BAGUIO"},
      {14, "p2p-clark-airport-balanga", 28, 29, "PAMPANGA", "BATAAN"},
  };
  for (const Pair& pair : added) {
    const on_point::RoutePair& route = on_point::routeAt(pair.route);
    EXPECT_STREQ(route.routeId, pair.id);
    EXPECT_EQ(route.outboundScheduleIndex, pair.out);
    EXPECT_EQ(route.returnScheduleIndex, pair.back);
    EXPECT_STREQ(route.originArea, pair.originArea);
    EXPECT_STREQ(route.destinationArea, pair.destinationArea);
    EXPECT_STREQ(on_point::scheduleAt(pair.out).routeId, pair.id);
  }
}

TEST(OnPointM2Data, EachNewRouteJoinsExactlyOneExistingCollection) {
  EXPECT_EQ(on_point::regionCount(), 3u);
  const auto members = [](const char* id) {
    std::vector<int> out;
    const on_point::Region* region = on_point::regionAt(on_point::regionIndexForId(id));
    EXPECT_NE(region, nullptr);
    if (region != nullptr) out.assign(region->routes, region->routes + region->routeCount);
    return out;
  };
  EXPECT_EQ(members("bulacan"), (std::vector<int>{0, 1, 7, 8, 9, 10}));
  EXPECT_EQ(members("north-luzon"), (std::vector<int>{2, 4, 5, 12, 13, 14}));
  EXPECT_EQ(members("airport-transfers"), (std::vector<int>{3, 6, 11}));
  EXPECT_STREQ(on_point::regionAt(on_point::regionIndexForId("north-luzon"))->defaultRouteId,
               "p2p-cubao-clark-airport");
  EXPECT_STREQ(on_point::regionAt(on_point::regionIndexForId("airport-transfers"))->defaultRouteId,
               "p2p-clark-airport-naia-t3");
}

}  // namespace
