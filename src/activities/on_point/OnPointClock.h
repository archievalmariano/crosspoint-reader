#pragma once

#include "OnPointSchedule.h"

namespace on_point {

class ClockSource {
 public:
  virtual ~ClockSource() = default;
  virtual bool nowUtc(CivilDateTime& value) const = 0;
};

// Production adapter. A future X4-compatible source only needs to implement
// ClockSource; the schedule engine, UI state, and renderer remain unchanged.
class HalClockSource final : public ClockSource {
 public:
  bool nowUtc(CivilDateTime& value) const override;
};

}  // namespace on_point
