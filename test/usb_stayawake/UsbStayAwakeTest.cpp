// Policy tests for the "keep awake when USB is detected" idle-sleep suppression.
// Exercises the pure decision (src/UsbStayAwake.h) across every build/board/setting
// boundary; the hardware-coupled detection and actual deep sleep stay on the
// physical acceptance checklist.
#include <gtest/gtest.h>

#include "UsbStayAwake.h"

namespace {

// Convenience wrappers naming the three compiled modes.
bool devOverride(bool isX4Pro, bool settingEnabled, bool usbPresent) {
  return usbStayAwakeSuppressIdleSleep(true, false, isX4Pro, settingEnabled, usbPresent);
}
bool production(bool isX4Pro, bool settingEnabled, bool usbPresent) {
  return usbStayAwakeSuppressIdleSleep(false, true, isX4Pro, settingEnabled, usbPresent);
}
bool disabledBuild(bool isX4Pro, bool settingEnabled, bool usbPresent) {
  return usbStayAwakeSuppressIdleSleep(false, false, isX4Pro, settingEnabled, usbPresent);
}

}  // namespace

// --- Production X4 Pro path ---------------------------------------------------

TEST(UsbStayAwake, DefaultSettingIsOff) {
  // The value CrossPointSettings uses to initialize the field (shared constant).
  EXPECT_EQ(0, kUsbStayAwakeDefaultEnabled);
}

TEST(UsbStayAwake, ProductionWithDefaultSettingDoesNotSuppressEvenOnUsb) {
  // Drive the decision with the ACTUAL field default, not a literal: a device with
  // no stored preference, on USB, must still allow normal idle sleep.
  EXPECT_FALSE(production(/*isX4Pro=*/true, /*settingEnabled=*/kUsbStayAwakeDefaultEnabled != 0,
                          /*usbPresent=*/true));
}

TEST(UsbStayAwake, ProductionOnAndUsbSuppresses) { EXPECT_TRUE(production(true, true, true)); }

TEST(UsbStayAwake, ProductionOnButNoUsbDoesNotSuppress) {
  // On battery (no USB detected), idle sleep is unchanged.
  EXPECT_FALSE(production(true, true, false));
}

TEST(UsbStayAwake, ProductionNonX4ProNeverSuppresses) {
  // Runtime board guard: even with the setting ON and USB present, a non-X4 Pro
  // board must not suppress sleep.
  EXPECT_FALSE(production(/*isX4Pro=*/false, /*settingEnabled=*/true, /*usbPresent=*/true));
}

// --- Dev override path --------------------------------------------------------

TEST(UsbStayAwake, DevOverrideUsbSuppressesRegardlessOfStoredSetting) {
  // Dev flashing aid: USB alone controls suppression, ignoring the stored
  // production setting (and independent of the board flag).
  EXPECT_TRUE(devOverride(/*isX4Pro=*/false, /*settingEnabled=*/false, /*usbPresent=*/true));
  EXPECT_TRUE(devOverride(true, false, true));
}

TEST(UsbStayAwake, DevOverrideNoUsbDoesNotSuppress) { EXPECT_FALSE(devOverride(true, true, false)); }

// --- All other builds ---------------------------------------------------------

TEST(UsbStayAwake, DisabledBuildNeverSuppresses) {
  EXPECT_FALSE(disabledBuild(true, true, true));
  EXPECT_FALSE(disabledBuild(false, false, false));
}
