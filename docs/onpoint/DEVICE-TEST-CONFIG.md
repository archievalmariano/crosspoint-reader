# ON POINT device-test configuration (X4 Pro)

**Status:** implemented on `feature/onpoint-stage1` and built locally. **Not installed on any device.** Installing
one needs separate owner approval, after a full-flash backup (the local backup-and-restore checklist).

## Envs (dev only, never a release)

| Env | Catalogue | Extends | Identity |
|---|---|---|---|
| `x4pro-all-devtest` | the shipped 7 routes | `x4pro-all-gh_release` (GPD) | version `…-GPD-DEVTEST`, USB product `CrossPoint_X4_Pro_DEVTEST` |
| `x4pro-all-capacity-devtest` | the synthetic 60-route fixture | `x4pro-all-capacity-bench` | version `…-GPD-CAPACITY-DEVTEST`, USB product `CrossPoint_X4_Pro_CAPACITY_DEVTEST` |

Both define `ON_POINT_BENCH_TIMING` (the `ONPTB` serial timing lines, and the model and `OnPointActivity` sizes) and
`CROSSPOINT_FIRMWARE_UPDATES_DISABLED`.

## Firmware updates compiled out

`CROSSPOINT_FIRMWARE_UPDATES_DISABLED` removes every path by which the firmware can write a new image:

| Path | Where | In a devtest image |
|---|---|---|
| Settings → System → Check for updates (OTA) | `SettingsActivity.cpp` (list and dispatch) | not listed; the action is a no-op |
| Settings → System → SD Firmware Update | `SettingsActivity.cpp` | not listed; the action is a no-op |
| DOWN + POWER at boot (SD recovery picker) | `main.cpp` `recoveryFirmwareMode` | always `false`; boots normally |

The `[devtest]` source filter also drops the update modules from the build, so any remaining reference fails at link
time rather than linking them back in:
- `network/OtaUpdater.cpp`, `network/FirmwareFlasher.cpp`, `network/OtaBootSwitch.cpp`,
  `network/FirmwareBoardTag.cpp`;
- `activities/settings/OtaUpdateActivity.cpp`, `activities/settings/SdFirmwareUpdateActivity.cpp`.

Release envs never define the macro, so their code is unchanged.

## Never installed from PlatformIO

- `scripts/on_point_devtest.py` refuses any other env, any upload target, and a devtest env without the macro.
- The envs' `upload_command` always fails.
- Installation is by hand with esptool, app-only at `0x10000` with `boot_app0` at `0xe000`, after the backups and
  the owner's approval.

## Guards (`scripts/check_no_capacity_fixture.py`)

- **`--config`:**
  - only the dev envs may name the updates-disabled macro, the bench and fixture macros, or the dev scripts;
  - no release env may use the `[devtest]` filter or extend a dev env;
  - every devtest env must define the macro, filter out all six sources and run `on_point_devtest.py`;
  - every dev env must refuse uploads;
  - no dev env may be in the candidate matrix.
- **`--scan`** (every release image): rejects `DEVTEST`, as well as the fixture markers.
- **`--scan-devtest`** (devtest images): requires the `-DEVTEST` identity. It rejects the update URL
  (`api.github.com/repos/`, `/releases/latest`), and, in the `.elf` symbol table, any symbol of the update modules.
- The guard has its own tests (`scripts/test_check_no_capacity_fixture.py`), which run in ctest.
