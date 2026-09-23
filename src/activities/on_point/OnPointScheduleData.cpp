#include "OnPointScheduleData.h"

#include <iterator>

namespace on_point {
namespace {

constexpr uint8_t WEEKDAYS = 0b0111110;  // Monday through Friday.

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
};

constexpr RoutePair ROUTES[] = {
    {"p2p-balagtas-trinoma", 0, 1, "BULACAN", "QUEZON CITY"},
    {"p2p-caypombo-sm-north-edsa", 2, 3, "SANTA MARIA, BULACAN", "QUEZON CITY"},
    {"p2p-calamba-bgc", 4, 5, "LAGUNA", "TAGUIG"},
    {"p2p-up-town-center-one-ayala", 6, 7, "QUEZON CITY", "MAKATI"},
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

}  // namespace on_point
