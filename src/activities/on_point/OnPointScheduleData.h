#pragma once

#include <cstddef>

#include "OnPointSchedule.h"

namespace on_point {

constexpr size_t SCHEDULE_COUNT = 2;

const Schedule& scheduleAt(size_t index);

}  // namespace on_point
