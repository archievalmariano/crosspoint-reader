#pragma once

#include <cstdint>

// Default for the "keep awake when USB is detected" setting: OFF (0), so a build
// with no stored value keeps the normal idle-sleep policy. Defined here (a pure,
// host-includable header) and used as the CrossPointSettings field initializer, so
// the persisted default and the tests share one source and cannot drift.
inline constexpr uint8_t kUsbStayAwakeDefaultEnabled = 0;

// Pure policy for the "keep awake when USB is detected" behavior, factored out of
// main.cpp so it can be unit-tested without hardware. It decides ONLY whether the
// automatic idle-timeout sleep should be suppressed; manual power-button sleep is a
// separate path and is never affected.
//
// Precedence (see main.cpp stayAwakeOnUsbPower()):
//   1. X4 Pro dev override (STAY_AWAKE_WHILE_USB_POWERED): USB detection alone.
//   2. X4 Pro production feature (FEATURE_USB_STAYAWAKE): board guard + setting + USB.
//   3. Every other build: never suppresses (returns false).

// devOverride       -- built with the dev-only always-on flag.
// productionFeature -- built with the production opt-in feature flag.
// isX4Pro           -- runtime board guard (defense-in-depth for the production path).
// settingEnabled    -- the user's persisted toggle.
// usbPresent        -- USB host connection OR charger/usbDetect signal.
inline bool usbStayAwakeSuppressIdleSleep(bool devOverride, bool productionFeature, bool isX4Pro, bool settingEnabled,
                                          bool usbPresent) {
  if (devOverride) return usbPresent;  // dev: USB alone, ignores the setting
  if (productionFeature) return isX4Pro && settingEnabled && usbPresent;
  return false;
}

// Tracks when USB suppression ends (undock, or the setting turned off) so the
// caller can restart the idle timer. Without this, idle time accrued while docked
// counts against the timeout and the device sleeps the moment it is undocked.
class UsbStayAwakeReleaseTracker {
 public:
  // Feed the current suppression state once per loop; returns true on the
  // suppressing -> not-suppressing transition only.
  bool released(bool suppressing) {
    const bool wasSuppressing = suppressing_;
    suppressing_ = suppressing;
    return wasSuppressing && !suppressing;
  }

 private:
  bool suppressing_ = false;
};
