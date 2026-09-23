#pragma once

#include <cstddef>

#include "OnPointSchedule.h"

namespace on_point {

constexpr size_t SCHEDULE_COUNT = 8;
constexpr size_t ROUTE_COUNT = 4;

struct RoutePair {
  const char* routeId = nullptr;
  size_t outboundScheduleIndex = 0;
  size_t returnScheduleIndex = 0;
  const char* originArea = nullptr;
  const char* destinationArea = nullptr;
};

const Schedule& scheduleAt(size_t index);
size_t routeCount();
const RoutePair& routeAt(size_t index);
const Schedule& scheduleForRoute(size_t routeIndex, bool reversed);

}  // namespace on_point
