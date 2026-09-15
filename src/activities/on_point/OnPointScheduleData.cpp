#include "OnPointScheduleData.h"

#include <iterator>

namespace on_point {
namespace {

constexpr uint8_t WEEKDAYS = 0b0111110;  // Monday through Friday.

// Provisional development fixture. Replace only these arrays and schedule
// metadata after checking the operator's current published timetable.
constexpr uint16_t BALAGTAS_TO_TRINOMA[] = {
    270, 300, 330, 360, 390, 420, 450, 480, 510, 540,  570,  600,  630,  660,  690,  720,
    750, 780, 810, 840, 870, 900, 930, 960, 990, 1020, 1050, 1080, 1110, 1140, 1170, 1200,
};

constexpr uint16_t TRINOMA_TO_BALAGTAS[] = {
    360, 390, 420, 450, 480, 510,  540,  570,  600,  630,  660,  690,  720,  750,  780,  810,  840,
    870, 900, 930, 960, 990, 1020, 1050, 1080, 1110, 1140, 1170, 1200, 1230, 1260, 1290, 1320, 1350,
};

constexpr ServiceRule BALAGTAS_RULES[] = {
    {WEEKDAYS, {BALAGTAS_TO_TRINOMA, std::size(BALAGTAS_TO_TRINOMA)}},
};

constexpr ServiceRule TRINOMA_RULES[] = {
    {WEEKDAYS, {TRINOMA_TO_BALAGTAS, std::size(TRINOMA_TO_BALAGTAS)}},
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
};

static_assert(std::size(SCHEDULES) == SCHEDULE_COUNT);

}  // namespace

const Schedule& scheduleAt(const size_t index) { return SCHEDULES[index < SCHEDULE_COUNT ? index : 0]; }

}  // namespace on_point
