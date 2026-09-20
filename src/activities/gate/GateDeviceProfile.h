#pragma once

// The Gate Is Open! -- device profile (CrossPoint side only).
//
// One game, one engine, one renderer. This tiny layer describes ONLY the
// capabilities Gate's interaction actually depends on, so future CrossPoint
// devices are new *profiles/targets*, not forks. The portable engine never
// sees any of this (and never sees a product name).
//
// The profile is selected from FreeInk CAPABILITY macros (FREEINK_CAP_TOUCH),
// not product-name conditionals -- so any device the SDK marks touch-capable
// gets the touch profile automatically. Display width/height still come from
// the renderer at runtime; refresh policy stays generic. Fields are added here
// only when a real device difference needs one.

#include <BoardConfig.h>  // FREEINK_CAP_TOUCH and friends

namespace gate_device {

enum class GateInputMode {
  NavButtons,       // dedicated Prev/Next/Confirm/Back (X3/X4, X4 Classic)
  TouchAndButtons,  // touch taps in addition to the nav buttons (X4 Pro, Sticky, ...)
};

struct GateDeviceProfile {
  GateInputMode inputMode;
  bool hasTouch;        // raw capability (drives whether taps are routed)
  const char* name;     // short label for logs / docs, not a product name
  const char* controlHint;  // one-line plain-text summary of controls
};

// The active profile for THIS firmware build, chosen by capability.
inline GateDeviceProfile activeProfile() {
#if FREEINK_CAP_TOUCH
  return GateDeviceProfile{
      GateInputMode::TouchAndButtons, /*hasTouch=*/true, "touch+buttons",
      "Tap a choice, or Up/Down then OK. Back leaves."};
#else
  return GateDeviceProfile{
      GateInputMode::NavButtons, /*hasTouch=*/false, "buttons",
      "Up/Down to move, OK to select, Back to leave."};
#endif
}

}  // namespace gate_device
