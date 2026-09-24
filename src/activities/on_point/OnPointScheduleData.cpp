#include "OnPointScheduleData.h"

#include <cstring>
#include <iterator>

namespace on_point {
namespace {

constexpr uint8_t WEEKDAYS = 0b0111110;  // Monday through Friday.
constexpr uint8_t DAILY = 0b1111111;

// Provisional timetable data. Replace only these arrays and schedule metadata
// after checking the operators' current published timetables.
constexpr uint16_t BALAGTAS_TO_TRINOMA[] = {
    270, 300, 330, 360, 390, 420, 450, 480, 510, 540,  570,  600,  630,  660,  690,  720,
    750, 780, 810, 840, 870, 900, 930, 960, 990, 1020, 1050, 1080, 1110, 1140, 1170, 1200,
};

constexpr uint16_t TRINOMA_TO_BALAGTAS[] = {
    360, 390, 420, 450, 480, 510,  540,  570,  600,  630,  660,  690,  720,  750,  780,  810,  840,
    870, 900, 930, 960, 990, 1020, 1050, 1080, 1110, 1140, 1170, 1200, 1230, 1260, 1290, 1320, 1350,
};

constexpr uint16_t CAYPOMBO_TO_SM_NORTH_EDSA[] = {
    300, 330, 360, 390, 420, 450, 480, 510, 540, 570, 600, 660, 720,
    780, 840, 900, 930, 960, 990, 1020, 1050, 1080, 1110, 1140, 1185,
};

constexpr uint16_t SM_NORTH_EDSA_TO_CAYPOMBO[] = {
    360, 390, 420, 450, 480, 510, 540, 570, 600, 660, 720, 780, 840, 900, 930,
    960, 990, 1020, 1050, 1080, 1110, 1140, 1170, 1200, 1230, 1260, 1290, 1320, 1350,
};

constexpr uint16_t CALAMBA_TO_BGC[] = {
    240, 300, 360, 420, 480, 540, 600, 660, 720, 780, 840, 900, 960, 1020, 1080, 1140, 1200,
};

constexpr uint16_t BGC_TO_CALAMBA[] = {
    360, 420, 480, 540, 600, 660, 720, 780, 840, 900, 960, 1020, 1080, 1140, 1200, 1260,
};

constexpr uint16_t UP_TOWN_CENTER_TO_ONE_AYALA[] = {
    320, 350, 405, 450, 540, 660, 780, 900, 960, 1020, 1080,
};

constexpr uint16_t ONE_AYALA_TO_UP_TOWN_CENTER[] = {
    390, 450, 510, 570, 660, 780, 900, 975, 1050, 1125, 1170,
};

constexpr uint16_t BAGUIO_TO_PITX[] = {
    60,  120, 180, 240, 300, 360, 420, 480, 540, 600, 660, 690,
    750, 840, 900, 960, 1020, 1080, 1140, 1200, 1260, 1320, 1380, 1440,
};

constexpr uint16_t PITX_TO_BAGUIO[] = {
    60,  120, 180, 240, 300, 360, 420, 480, 540, 600, 660, 690,
    750, 840, 900, 960, 1020, 1080, 1140, 1200, 1260, 1320, 1380, 1440,
};

constexpr uint16_t ARANETA_CITY_TO_NAIA[] = {
    360, 420, 540, 660, 750, 900, 990, 1110, 1260,
};

constexpr uint16_t NAIA_TO_ARANETA_CITY[] = {
    330, 450, 540, 660, 810, 870, 990, 1110, 1230,
};

constexpr uint16_t CLARK_AIRPORT_TO_NAIA_T3[] = {
    120, 180, 240, 300, 360, 420, 480, 540, 600, 660,
    720, 780, 840, 900, 960, 1020, 1080, 1140, 1230,
};

constexpr uint16_t NAIA_T3_TO_CLARK_AIRPORT[] = {
    0,   120, 240, 330, 420, 480, 540, 600, 660, 720,
    780, 840, 900, 960, 1020, 1080, 1170, 1230, 1290, 1350,
};

constexpr ServiceRule BALAGTAS_RULES[] = {
    {WEEKDAYS, {BALAGTAS_TO_TRINOMA, std::size(BALAGTAS_TO_TRINOMA)}},
};

constexpr ServiceRule TRINOMA_RULES[] = {
    {WEEKDAYS, {TRINOMA_TO_BALAGTAS, std::size(TRINOMA_TO_BALAGTAS)}},
};

constexpr ServiceRule CAYPOMBO_RULES[] = {
    {WEEKDAYS, {CAYPOMBO_TO_SM_NORTH_EDSA, std::size(CAYPOMBO_TO_SM_NORTH_EDSA)}},
};

constexpr ServiceRule SM_NORTH_EDSA_RULES[] = {
    {WEEKDAYS, {SM_NORTH_EDSA_TO_CAYPOMBO, std::size(SM_NORTH_EDSA_TO_CAYPOMBO)}},
};

constexpr ServiceRule CALAMBA_RULES[] = {
    {WEEKDAYS, {CALAMBA_TO_BGC, std::size(CALAMBA_TO_BGC)}},
};

constexpr ServiceRule BGC_RULES[] = {
    {WEEKDAYS, {BGC_TO_CALAMBA, std::size(BGC_TO_CALAMBA)}},
};

constexpr ServiceRule UP_TOWN_CENTER_RULES[] = {
    {WEEKDAYS, {UP_TOWN_CENTER_TO_ONE_AYALA, std::size(UP_TOWN_CENTER_TO_ONE_AYALA)}},
};

constexpr ServiceRule ONE_AYALA_RULES[] = {
    {WEEKDAYS, {ONE_AYALA_TO_UP_TOWN_CENTER, std::size(ONE_AYALA_TO_UP_TOWN_CENTER)}},
};

constexpr ServiceRule BAGUIO_RULES[] = {
    {DAILY, {BAGUIO_TO_PITX, std::size(BAGUIO_TO_PITX)}},
};

constexpr ServiceRule PITX_RULES[] = {
    {DAILY, {PITX_TO_BAGUIO, std::size(PITX_TO_BAGUIO)}},
};

constexpr ServiceRule ARANETA_CITY_RULES[] = {
    {DAILY, {ARANETA_CITY_TO_NAIA, std::size(ARANETA_CITY_TO_NAIA)}},
};

constexpr ServiceRule NAIA_RULES[] = {
    {DAILY, {NAIA_TO_ARANETA_CITY, std::size(NAIA_TO_ARANETA_CITY)}},
};

constexpr ServiceRule CLARK_AIRPORT_RULES[] = {
    {DAILY, {CLARK_AIRPORT_TO_NAIA_T3, std::size(CLARK_AIRPORT_TO_NAIA_T3)}},
};

constexpr ServiceRule NAIA_T3_RULES[] = {
    {DAILY, {NAIA_T3_TO_CLARK_AIRPORT, std::size(NAIA_T3_TO_CLARK_AIRPORT)}},
};

constexpr Schedule SCHEDULES[] = {
    {"p2p-balagtas-trinoma",
     "Bulacan P2P",
     "BALAGTAS",
     "TRINOMA",
     "Asia/Manila",
     8 * 60,
     {2026, 9, 15},
     BALAGTAS_RULES,
     std::size(BALAGTAS_RULES),
     nullptr,
     0,
     "Development fixture; verify against the operator timetable before release.",
     SourceStatus::Provisional},
    {"p2p-trinoma-balagtas",
     "Bulacan P2P",
     "TRINOMA",
     "BALAGTAS",
     "Asia/Manila",
     8 * 60,
     {2026, 9, 15},
     TRINOMA_RULES,
     std::size(TRINOMA_RULES),
     nullptr,
     0,
     "Development fixture; verify against the operator timetable before release.",
     SourceStatus::Provisional},
    {"p2p-caypombo-sm-north-edsa",
     "Precious Grace",
     "CAYPOMBO",
     "SM NORTH EDSA",
     "Asia/Manila",
     8 * 60,
     {2026, 9, 15},
     CAYPOMBO_RULES,
     std::size(CAYPOMBO_RULES),
     nullptr,
     0,
     "P2PBus.ph rider directory; verify with Precious Grace before release.",
     SourceStatus::Provisional},
    {"p2p-sm-north-edsa-caypombo",
     "Precious Grace",
     "SM NORTH EDSA",
     "CAYPOMBO",
     "Asia/Manila",
     8 * 60,
     {2026, 9, 15},
     SM_NORTH_EDSA_RULES,
     std::size(SM_NORTH_EDSA_RULES),
     nullptr,
     0,
     "P2PBus.ph rider directory; verify with Precious Grace before release.",
     SourceStatus::Provisional},
    {"p2p-calamba-bgc",
     "Calamba P2P",
     "CALAMBA",
     "BGC",
     "Asia/Manila",
     8 * 60,
     {2026, 9, 15},
     CALAMBA_RULES,
     std::size(CALAMBA_RULES),
     nullptr,
     0,
     "P2PBus.ph rider directory; verify with Calamba P2P before release.",
     SourceStatus::Provisional},
    {"p2p-bgc-calamba",
     "Calamba P2P",
     "BGC",
     "CALAMBA",
     "Asia/Manila",
     8 * 60,
     {2026, 9, 15},
     BGC_RULES,
     std::size(BGC_RULES),
     nullptr,
     0,
     "P2PBus.ph rider directory; verify with Calamba P2P before release.",
     SourceStatus::Provisional},
    {"p2p-up-town-center-one-ayala",
     "DNS",
     "UP TOWN CENTER",
     "ONE AYALA",
     "Asia/Manila",
     8 * 60,
     {2026, 9, 15},
     UP_TOWN_CENTER_RULES,
     std::size(UP_TOWN_CENTER_RULES),
     nullptr,
     0,
     "P2PBus.ph rider directory; verify with DNS before release.",
     SourceStatus::Provisional},
    {"p2p-one-ayala-up-town-center",
     "DNS",
     "ONE AYALA",
     "UP TOWN CENTER",
     "Asia/Manila",
     8 * 60,
     {2026, 9, 15},
     ONE_AYALA_RULES,
     std::size(ONE_AYALA_RULES),
     nullptr,
     0,
     "P2PBus.ph rider directory; verify with DNS before release.",
     SourceStatus::Provisional},
    {"p2p-baguio-pitx",
     "Solid North",
     "BAGUIO",
     "PITX",
     "Asia/Manila",
     8 * 60,
     {2026, 9, 23},
     BAGUIO_RULES,
     std::size(BAGUIO_RULES),
     nullptr,
     0,
     "P2PBus.ph rider directory; verify with Solid North before release.",
     SourceStatus::Provisional},
    {"p2p-pitx-baguio",
     "Solid North",
     "PITX",
     "BAGUIO",
     "Asia/Manila",
     8 * 60,
     {2026, 9, 23},
     PITX_RULES,
     std::size(PITX_RULES),
     nullptr,
     0,
     "P2PBus.ph rider directory; verify with Solid North before release.",
     SourceStatus::Provisional},
    {"p2p-araneta-city-naia",
     "UBE Express",
     "ARANETA CITY",
     "NAIA",
     "Asia/Manila",
     8 * 60,
     {2026, 9, 23},
     ARANETA_CITY_RULES,
     std::size(ARANETA_CITY_RULES),
     nullptr,
     0,
     "P2PBus.ph rider directory; verify with UBE Express before release.",
     SourceStatus::Provisional},
    {"p2p-naia-araneta-city",
     "UBE Express",
     "NAIA",
     "ARANETA CITY",
     "Asia/Manila",
     8 * 60,
     {2026, 9, 23},
     NAIA_RULES,
     std::size(NAIA_RULES),
     nullptr,
     0,
     "P2PBus.ph rider directory; verify with UBE Express before release.",
     SourceStatus::Provisional},
    {"p2p-clark-airport-naia-t3",
     "Genesis",
     "CLARK AIRPORT",
     "NAIA T3",
     "Asia/Manila",
     8 * 60,
     {2026, 9, 23},
     CLARK_AIRPORT_RULES,
     std::size(CLARK_AIRPORT_RULES),
     nullptr,
     0,
     "P2PBus.ph rider directory; verify with Genesis before release.",
     SourceStatus::Provisional},
    {"p2p-naia-t3-clark-airport",
     "Genesis",
     "NAIA T3",
     "CLARK AIRPORT",
     "Asia/Manila",
     8 * 60,
     {2026, 9, 23},
     NAIA_T3_RULES,
     std::size(NAIA_T3_RULES),
     nullptr,
     0,
     "P2PBus.ph rider directory; verify with Genesis before release.",
     SourceStatus::Provisional},
};

constexpr RoutePair ROUTES[] = {
    {"p2p-balagtas-trinoma", 0, 1, "BULACAN", "QUEZON CITY"},
    {"p2p-caypombo-sm-north-edsa", 2, 3, "SANTA MARIA, BULACAN", "QUEZON CITY"},
    {"p2p-calamba-bgc", 4, 5, "LAGUNA", "TAGUIG"},
    {"p2p-up-town-center-one-ayala", 6, 7, "QUEZON CITY", "MAKATI"},
    {"p2p-baguio-pitx", 8, 9, "BAGUIO", "PARAÑAQUE"},
    {"p2p-araneta-city-naia", 10, 11, "QUEZON CITY", "PASAY / PARAÑAQUE"},
    {"p2p-clark-airport-naia-t3", 12, 13, "PAMPANGA", "PASAY"},
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
