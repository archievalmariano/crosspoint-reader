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

## Results at `adcdf9cb` (local)

**Images**, built by the V0 matrix run (a disposable worktree; clean, forced builds; 0 warnings):

| Image | Size | Headroom | SHA-256 |
|---|---|---|---|
| `x4pro-all-devtest.bin` (`1.6.5-A1.2.0-GPD-DEVTEST`) | 5,738,064 | 815,536 | `ecb139c9bb2694ccb3444422a3423b78b5290db56e6bcf9e6fa8a41c345458e8` |
| `x4pro-all-capacity-devtest.bin` (`1.6.5-A1.2.0-GPD-CAPACITY-DEVTEST`) | 5,921,200 | 632,400 | `a0e140ee536924eb0400933c6198206afc1a3eb245d3aa2822ceb305929a4a28` |

- Both are valid images; `--scan-devtest` passes for each; the release scan rejects both; both refuse `-t upload`.

**Release envs are unchanged:**
- P, PD, GP, GPD and G have the same image sizes as at `da598c56`.
- Every loaded section and every symbol (name, size, type) is identical.
- The only ELF difference is 2–3 bytes of `.debug_line`.
- Every differing image byte is the build date/time text, the ELF hash in the app descriptor (`0xb0–0xcf`), or the
  trailing checksum and SHA.

**Tests and checks:**
- 602/602 host tests; the guard's 25 tests pass.
- `pio check` PASSED (`default` and `x4pro-all-devtest`).
- V1 (+ v1.2.1) 606, V2 (+ holidays) 607, V3 (+ both) 611: no conflicts, all release builds OK, scans clean.

**Note:** the images are v1.2.0 + Stage 1. On a device running v1.2.1:
- v1.2.1 changed no settings or state format;
- its library-scan skip of `/dogear` is absent, so `/.crosspoint/library.*` may change while a devtest image runs.
  That's restored from the SD backup afterwards.
