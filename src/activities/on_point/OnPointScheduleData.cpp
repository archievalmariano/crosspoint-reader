#include "OnPointScheduleData.h"

#include <cstring>
#include <iterator>

namespace on_point {
namespace {

constexpr uint8_t DAILY = 0b1111111;
constexpr uint8_t WEEKDAYS = 0b0111110;  // Monday through Friday.
constexpr uint8_t WEEKENDS = 0b1000001;  // Saturday and Sunday.

// Timetables are Verified (shown as PUBLISHED) when they come from the
// operator's own published schedule; anything else must stay Provisional
// (shown as PROVISIONAL) until checked with the operator.

// Bulacan P2P official posts: first/last trips for Monday to Friday and for
// Saturday, Sunday and holidays; 30-minute interval (as P2PBus.ph lists).
constexpr uint16_t BALAGTAS_TO_TRINOMA_WEEKDAYS[] = {
    270, 300, 330, 360, 390, 420, 450, 480, 510, 540,  570,  600,  630,  660,  690,  720,
    750, 780, 810, 840, 870, 900, 930, 960, 990, 1020, 1050, 1080, 1110, 1140, 1170, 1200,
};

constexpr uint16_t BALAGTAS_TO_TRINOMA_WEEKENDS[] = {
    300, 330, 360, 390, 420, 450, 480, 510, 540,  570,  600,  630,  660,  690,  720,  750,
    780, 810, 840, 870, 900, 930, 960, 990, 1020, 1050, 1080, 1110, 1140, 1170, 1200,
};

constexpr uint16_t TRINOMA_TO_BALAGTAS_WEEKDAYS[] = {
    360, 390, 420, 450, 480, 510,  540,  570,  600,  630,  660,  690,  720,  750,  780,  810,  840,
    870, 900, 930, 960, 990, 1020, 1050, 1080, 1110, 1140, 1170, 1200, 1230, 1260, 1290, 1320, 1350,
};

constexpr uint16_t TRINOMA_TO_BALAGTAS_WEEKENDS[] = {
    390, 420, 450, 480, 510,  540,  570,  600,  630,  660,  690,  720,  750,  780,  810,  840,  870,
    900, 930, 960, 990, 1020, 1050, 1080, 1110, 1140, 1170, 1200, 1230, 1260, 1290, 1320, 1350,
};

// Philippine holidays, which take Bulacan P2P's Saturday/Sunday/holiday
// timetable: 2026 regular and special non-working holidays (Official Gazette,
// Proclamation No. 1006) plus the NCR special non-working days for the ASEAN
// Summit (November 16-18). Add each year's list once it is proclaimed.
constexpr CivilDate HOLIDAYS_2026[] = {
    {2026, 1, 1},   {2026, 2, 17},  {2026, 4, 2},  {2026, 4, 3},   {2026, 4, 4},   {2026, 4, 9},   {2026, 5, 1},
    {2026, 6, 12},  {2026, 8, 21},  {2026, 8, 31}, {2026, 11, 1},  {2026, 11, 2},  {2026, 11, 16}, {2026, 11, 17},
    {2026, 11, 18}, {2026, 11, 30}, {2026, 12, 8}, {2026, 12, 24}, {2026, 12, 25}, {2026, 12, 30}, {2026, 12, 31},
};

constexpr DateOverride BALAGTAS_HOLIDAYS[] = {
    {HOLIDAYS_2026[0], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[1], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[2], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[3], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[4], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[5], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[6], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[7], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[8], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[9], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[10], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[11], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[12], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[13], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[14], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[15], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[16], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[17], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[18], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[19], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
    {HOLIDAYS_2026[20], {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
};

constexpr DateOverride TRINOMA_HOLIDAYS[] = {
    {HOLIDAYS_2026[0], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[1], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[2], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[3], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[4], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[5], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[6], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[7], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[8], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[9], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[10], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[11], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[12], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[13], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[14], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[15], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[16], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[17], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[18], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[19], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
    {HOLIDAYS_2026[20], {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
};

// Precious Grace official post (current): first trip, last trip and a
// 30-minute interval, every day; departures may shift with ridership.
constexpr uint16_t CAYPOMBO_TO_SM_NORTH_EDSA[] = {
    300, 330, 360, 390, 420, 450, 480, 510, 540,  570,  600,  630,  660,  690,  720,  750,
    780, 810, 840, 870, 900, 930, 960, 990, 1020, 1050, 1080, 1110, 1140, 1170, 1200,
};

constexpr uint16_t SM_NORTH_EDSA_TO_CAYPOMBO[] = {
    360, 390, 420, 450, 480, 510,  540,  570,  600,  630,  660,  690,  720,  750,  780,  810,  840,
    870, 900, 930, 960, 990, 1020, 1050, 1080, 1110, 1140, 1170, 1200, 1230, 1260, 1290, 1320, 1350,
};

// Genesis Transport official trip schedules as of 2026-08-18 (via TriNoma and
// SM City Clark). The posted 1:10 AM Cubao trip is read as 1:10 PM: it sits
// between 11:10 AM and 3:10 PM and reaches TriNoma at 2:00 PM.
constexpr uint16_t CUBAO_TO_CLARK_AIRPORT[] = {
    210, 310, 430, 550, 670, 790, 910, 1030, 1150, 1270,
};

constexpr uint16_t CLARK_AIRPORT_TO_CUBAO[] = {
    270, 360, 480, 600, 720, 840, 960, 1020, 1080, 1140, 1230,
};

// Genesis Transport official trip schedules as of 2026-08-18. Clark to NAIA
// continues to NAIA Terminals 2 and 1; NAIA T3 departures include the PITX
// trips that pass through Terminal 3.
constexpr uint16_t CLARK_AIRPORT_TO_NAIA_T3[] = {
    30,  120, 225, 315, 360, 405,  480,  510,  570,  600,  690,  750,
    780, 825, 870, 900, 960, 1005, 1050, 1095, 1200, 1260, 1350,
};

constexpr uint16_t NAIA_T3_TO_CLARK_AIRPORT[] = {
    0,   60,  90,  150, 210, 270, 300, 330, 360,  390,  450,  480,  510,  570,  600,  660,
    690, 720, 750, 780, 840, 900, 930, 960, 1020, 1080, 1140, 1170, 1230, 1290, 1350,
};

// Genesis Transport official trip schedules as of 2026-08-18 (TriNoma trips
// include the Cubao departures passing through TriNoma).
constexpr uint16_t TRINOMA_TO_CLARK_AIRPORT[] = {
    270, 360, 420, 480, 540, 600, 660, 720, 780, 840, 900, 960, 1020, 1080, 1140, 1200, 1290, 1380,
};

constexpr uint16_t CLARK_AIRPORT_TO_TRINOMA[] = {
    210, 270, 315, 360, 420, 480, 540, 600, 660, 720, 780, 840, 900, 960, 1020, 1080, 1140, 1230,
};

// Genesis Transport official trip schedules as of 2026-07-07.
constexpr uint16_t CLARK_AIRPORT_TO_BAGUIO[] = {
    120, 240, 480, 570, 660, 780, 930, 1050, 1170,
};

constexpr uint16_t BAGUIO_TO_CLARK_AIRPORT[] = {
    180, 360, 450, 480, 615, 720, 840, 960, 1020, 1080,
};

// UBE Express (Airport Transport Express) daily trip schedule as of
// 2026-05-01: NAIA Terminal 3 to the NAIA-PITX loop, and back to NAIA
// Terminals 1, 2 and 3.
constexpr uint16_t NAIA_T3_TO_PITX[] = {
    270, 375, 495, 630, 780, 885, 1005, 1125,
};

constexpr uint16_t PITX_TO_NAIA[] = {
    285, 405, 525, 660, 855, 990, 1110,
};

constexpr ServiceRule BALAGTAS_RULES[] = {
    {WEEKDAYS, {BALAGTAS_TO_TRINOMA_WEEKDAYS, std::size(BALAGTAS_TO_TRINOMA_WEEKDAYS)}},
    {WEEKENDS, {BALAGTAS_TO_TRINOMA_WEEKENDS, std::size(BALAGTAS_TO_TRINOMA_WEEKENDS)}},
};

constexpr ServiceRule TRINOMA_RULES[] = {
    {WEEKDAYS, {TRINOMA_TO_BALAGTAS_WEEKDAYS, std::size(TRINOMA_TO_BALAGTAS_WEEKDAYS)}},
    {WEEKENDS, {TRINOMA_TO_BALAGTAS_WEEKENDS, std::size(TRINOMA_TO_BALAGTAS_WEEKENDS)}},
};

constexpr ServiceRule CAYPOMBO_RULES[] = {
    {DAILY, {CAYPOMBO_TO_SM_NORTH_EDSA, std::size(CAYPOMBO_TO_SM_NORTH_EDSA)}},
};

constexpr ServiceRule SM_NORTH_EDSA_RULES[] = {
    {DAILY, {SM_NORTH_EDSA_TO_CAYPOMBO, std::size(SM_NORTH_EDSA_TO_CAYPOMBO)}},
};

constexpr ServiceRule CUBAO_CLARK_RULES[] = {
    {DAILY, {CUBAO_TO_CLARK_AIRPORT, std::size(CUBAO_TO_CLARK_AIRPORT)}},
};

constexpr ServiceRule CLARK_AIRPORT_CUBAO_RULES[] = {
    {DAILY, {CLARK_AIRPORT_TO_CUBAO, std::size(CLARK_AIRPORT_TO_CUBAO)}},
};

constexpr ServiceRule CLARK_AIRPORT_NAIA_RULES[] = {
    {DAILY, {CLARK_AIRPORT_TO_NAIA_T3, std::size(CLARK_AIRPORT_TO_NAIA_T3)}},
};

constexpr ServiceRule NAIA_T3_RULES[] = {
    {DAILY, {NAIA_T3_TO_CLARK_AIRPORT, std::size(NAIA_T3_TO_CLARK_AIRPORT)}},
};

constexpr ServiceRule TRINOMA_CLARK_RULES[] = {
    {DAILY, {TRINOMA_TO_CLARK_AIRPORT, std::size(TRINOMA_TO_CLARK_AIRPORT)}},
};

constexpr ServiceRule CLARK_AIRPORT_TRINOMA_RULES[] = {
    {DAILY, {CLARK_AIRPORT_TO_TRINOMA, std::size(CLARK_AIRPORT_TO_TRINOMA)}},
};

constexpr ServiceRule CLARK_AIRPORT_BAGUIO_RULES[] = {
    {DAILY, {CLARK_AIRPORT_TO_BAGUIO, std::size(CLARK_AIRPORT_TO_BAGUIO)}},
};

constexpr ServiceRule BAGUIO_RULES[] = {
    {DAILY, {BAGUIO_TO_CLARK_AIRPORT, std::size(BAGUIO_TO_CLARK_AIRPORT)}},
};

constexpr ServiceRule NAIA_PITX_RULES[] = {
    {DAILY, {NAIA_T3_TO_PITX, std::size(NAIA_T3_TO_PITX)}},
};

constexpr ServiceRule PITX_RULES[] = {
    {DAILY, {PITX_TO_NAIA, std::size(PITX_TO_NAIA)}},
};

constexpr Schedule SCHEDULES[] = {
    {"p2p-balagtas-trinoma",
     "Bulacan P2P",
     "BALAGTAS",
     "TRINOMA",
     "Asia/Manila",
     8 * 60,
     {2026, 9, 1},
     BALAGTAS_RULES,
     std::size(BALAGTAS_RULES),
     BALAGTAS_HOLIDAYS,
     std::size(BALAGTAS_HOLIDAYS),
     "Bulacan P2P official posts: first/last trips, 30-minute interval; holidays use the weekend timetable.",
     SourceStatus::Verified},
    {"p2p-trinoma-balagtas",
     "Bulacan P2P",
     "TRINOMA",
     "BALAGTAS",
     "Asia/Manila",
     8 * 60,
     {2026, 9, 1},
     TRINOMA_RULES,
     std::size(TRINOMA_RULES),
     TRINOMA_HOLIDAYS,
     std::size(TRINOMA_HOLIDAYS),
     "Bulacan P2P official posts: first/last trips, 30-minute interval; holidays use the weekend timetable.",
     SourceStatus::Verified},
    {"p2p-caypombo-sm-north-edsa",
     "Precious Grace",
     "CAYPOMBO",
     "SM NORTH EDSA",
     "Asia/Manila",
     8 * 60,
     {2026, 9, 1},
     CAYPOMBO_RULES,
     std::size(CAYPOMBO_RULES),
     nullptr,
     0,
     "Precious Grace official post: first/last trips, 30-minute interval.",
     SourceStatus::Verified},
    {"p2p-sm-north-edsa-caypombo",
     "Precious Grace",
     "SM NORTH EDSA",
     "CAYPOMBO",
     "Asia/Manila",
     8 * 60,
     {2026, 9, 1},
     SM_NORTH_EDSA_RULES,
     std::size(SM_NORTH_EDSA_RULES),
     nullptr,
     0,
     "Precious Grace official post: first/last trips, 30-minute interval.",
     SourceStatus::Verified},
    {"p2p-cubao-clark-airport",
     "Genesis",
     "CUBAO",
     "CLARK AIRPORT",
     "Asia/Manila",
     8 * 60,
     {2026, 8, 18},
     CUBAO_CLARK_RULES,
     std::size(CUBAO_CLARK_RULES),
     nullptr,
     0,
     "Genesis Transport trip schedule as of 2026-08-18.",
     SourceStatus::Verified},
    {"p2p-clark-airport-cubao",
     "Genesis",
     "CLARK AIRPORT",
     "CUBAO",
     "Asia/Manila",
     8 * 60,
     {2026, 8, 18},
     CLARK_AIRPORT_CUBAO_RULES,
     std::size(CLARK_AIRPORT_CUBAO_RULES),
     nullptr,
     0,
     "Genesis Transport trip schedule as of 2026-08-18.",
     SourceStatus::Verified},
    {"p2p-clark-airport-naia-t3",
     "Genesis",
     "CLARK AIRPORT",
     "NAIA T3",
     "Asia/Manila",
     8 * 60,
     {2026, 8, 18},
     CLARK_AIRPORT_NAIA_RULES,
     std::size(CLARK_AIRPORT_NAIA_RULES),
     nullptr,
     0,
     "Genesis Transport trip schedule as of 2026-08-18.",
     SourceStatus::Verified},
    {"p2p-naia-t3-clark-airport",
     "Genesis",
     "NAIA T3",
     "CLARK AIRPORT",
     "Asia/Manila",
     8 * 60,
     {2026, 8, 18},
     NAIA_T3_RULES,
     std::size(NAIA_T3_RULES),
     nullptr,
     0,
     "Genesis Transport trip schedule as of 2026-08-18.",
     SourceStatus::Verified},
    {"p2p-trinoma-clark-airport",
     "Genesis",
     "TRINOMA",
     "CLARK AIRPORT",
     "Asia/Manila",
     8 * 60,
     {2026, 8, 18},
     TRINOMA_CLARK_RULES,
     std::size(TRINOMA_CLARK_RULES),
     nullptr,
     0,
     "Genesis Transport trip schedule as of 2026-08-18.",
     SourceStatus::Verified},
    {"p2p-clark-airport-trinoma",
     "Genesis",
     "CLARK AIRPORT",
     "TRINOMA",
     "Asia/Manila",
     8 * 60,
     {2026, 8, 18},
     CLARK_AIRPORT_TRINOMA_RULES,
     std::size(CLARK_AIRPORT_TRINOMA_RULES),
     nullptr,
     0,
     "Genesis Transport trip schedule as of 2026-08-18.",
     SourceStatus::Verified},
    {"p2p-clark-airport-baguio",
     "Genesis",
     "CLARK AIRPORT",
     "BAGUIO",
     "Asia/Manila",
     8 * 60,
     {2026, 7, 7},
     CLARK_AIRPORT_BAGUIO_RULES,
     std::size(CLARK_AIRPORT_BAGUIO_RULES),
     nullptr,
     0,
     "Genesis Transport trip schedule as of 2026-07-07.",
     SourceStatus::Verified},
    {"p2p-baguio-clark-airport",
     "Genesis",
     "BAGUIO",
     "CLARK AIRPORT",
     "Asia/Manila",
     8 * 60,
     {2026, 7, 7},
     BAGUIO_RULES,
     std::size(BAGUIO_RULES),
     nullptr,
     0,
     "Genesis Transport trip schedule as of 2026-07-07.",
     SourceStatus::Verified},
    {"p2p-naia-pitx",
     "UBE Express",
     "NAIA T3",
     "PITX",
     "Asia/Manila",
     8 * 60,
     {2026, 5, 1},
     NAIA_PITX_RULES,
     std::size(NAIA_PITX_RULES),
     nullptr,
     0,
     "UBE Express trip schedule as of 2026-05-01.",
     SourceStatus::Verified},
    {"p2p-pitx-naia",
     "UBE Express",
     "PITX",
     "NAIA T3",
     "Asia/Manila",
     8 * 60,
     {2026, 5, 1},
     PITX_RULES,
     std::size(PITX_RULES),
     nullptr,
     0,
     "UBE Express trip schedule as of 2026-05-01.",
     SourceStatus::Verified},
};

constexpr RoutePair ROUTES[] = {
    {"p2p-balagtas-trinoma", 0, 1, "BULACAN", "QUEZON CITY"},
    {"p2p-caypombo-sm-north-edsa", 2, 3, "SANTA MARIA, BULACAN", "QUEZON CITY"},
    {"p2p-cubao-clark-airport", 4, 5, "QUEZON CITY", "PAMPANGA"},
    {"p2p-clark-airport-naia-t3", 6, 7, "PAMPANGA", "PASAY"},
    {"p2p-trinoma-clark-airport", 8, 9, "QUEZON CITY", "PAMPANGA"},
    {"p2p-clark-airport-baguio", 10, 11, "PAMPANGA", "BAGUIO"},
    {"p2p-naia-pitx", 12, 13, "PASAY", "PARAÑAQUE"},
};

static_assert(std::size(SCHEDULES) == SCHEDULE_COUNT);
static_assert(std::size(ROUTES) == ROUTE_COUNT);

}  // namespace

const Schedule& scheduleAt(const size_t index) { return SCHEDULES[index < SCHEDULE_COUNT ? index : 0]; }

size_t routeCount() { return std::size(ROUTES); }

const RoutePair& routeAt(const size_t index) { return ROUTES[index < ROUTE_COUNT ? index : 0]; }

const Schedule& scheduleForRoute(const size_t routeIndex, const bool reversed) {
  const RoutePair& route = routeAt(routeIndex);
  return scheduleAt(reversed ? route.returnScheduleIndex : route.outboundScheduleIndex);
}

size_t routeIndexInAlphabeticalOrder(const size_t position) {
  if (position >= ROUTE_COUNT) return 0;

  for (size_t candidate = 0; candidate < ROUTE_COUNT; ++candidate) {
    const Schedule& candidateSchedule = scheduleAt(ROUTES[candidate].outboundScheduleIndex);
    size_t earlierRoutes = 0;
    for (size_t other = 0; other < ROUTE_COUNT; ++other) {
      if (other == candidate) continue;
      const Schedule& otherSchedule = scheduleAt(ROUTES[other].outboundScheduleIndex);
      int comparison = strcmp(otherSchedule.origin, candidateSchedule.origin);
      if (comparison == 0) comparison = strcmp(otherSchedule.destination, candidateSchedule.destination);
      if (comparison == 0) comparison = strcmp(ROUTES[other].routeId, ROUTES[candidate].routeId);
      if (comparison < 0) ++earlierRoutes;
    }
    if (earlierRoutes == position) return candidate;
  }
  return 0;
}

size_t routeIndexForId(const char* routeId) {
  if (routeId) {
    for (size_t index = 0; index < ROUTE_COUNT; ++index) {
      if (strcmp(ROUTES[index].routeId, routeId) == 0) return index;
    }
  }
  return 0;
}

}  // namespace on_point
