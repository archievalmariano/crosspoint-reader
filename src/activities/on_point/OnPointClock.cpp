#include "OnPointClock.h"

#include <HalClock.h>

namespace on_point {

bool HalClockSource::nowUtc(CivilDateTime& value) const {
  HalClock::DateTime dateTime;
  if (!halClock.getDateTime(dateTime, true)) return false;
  value = {static_cast<int16_t>(dateTime.year),
           dateTime.month,
           dateTime.day,
           dateTime.hour,
           dateTime.minute,
           dateTime.second};
  return true;
}

}  // namespace on_point
