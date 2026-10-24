# Proposal: an OTA-disabled ON POINT device-test configuration (NOT implemented)

**Status:** proposal for later approval. Nothing here is built, flashed or prepared for automatic installation.

The current capacity envs (`x4pro-all-capacity`, `x4pro-all-capacity-bench`) stay **strictly non-flashable**:
- `scripts/on_point_capacity_fixture.py` refuses any upload target before the build runs;
- the envs' `upload_command` always fails;
- `scripts/check_no_capacity_fixture.py` fails if a capacity env could upload.

A request with `-t upload` and a nonexistent port was refused in the pre-script, and esptool was never started.

## Why a separate configuration

The capacity images only override the update repository (to a nonexistent one). Every firmware-write path is still
compiled in:
- Settings → System → **Check for updates** (`OtaUpdateActivity` → `OtaUpdater`);
- **SD Card Firmware Update** (`SdFirmwareUpdateActivity`).

✅ No update check runs automatically (`SettingsActivity.cpp`: both are user-started Settings actions only). Still,
a device running test firmware must not be able to replace itself from the network or the SD card, and must not be
mistaken for a release. That needs code gated by a dev-only define, which is a change to stock CrossPoint paths.

## Proposed configuration

| Item | Proposal |
|---|---|
| Env | `x4pro-all-capacity-devtest`: `extends = env:x4pro-all-capacity-bench` (GPD + fixture + timing logs) |
| Updates compiled out | New dev-only define `CROSSPOINT_FIRMWARE_UPDATES_DISABLED`: `SettingsActivity` omits **Check for updates** and **SD Card Firmware Update**, and `OtaUpdater`/`SdFirmwareUpdateActivity` aren't linked. The version check API URL is absent from the image. |
| Identity | Version suffix `-DEVTEST` (instead of `-CAPACITY`), USB product `CrossPoint_X4_Pro_DEVTEST`, a fixed "DEV TEST BUILD" line in Settings → About |
| Installation | Manual USB only, by esptool, app-only at `0x10000` after the plan §9 full-flash and SD backups. Never on the SD card, never attached to a release, never in the candidate matrix. |
| Upload from PlatformIO | Still refused (same `upload_command` / pre-script refusal); esptool is run by hand only after approval |
| Guards | `check_no_capacity_fixture.py` extended: the devtest env must define `CROSSPOINT_FIRMWARE_UPDATES_DISABLED`; no release env may. Artifact scan of the devtest image must find **no** `api.github.com/repos/` string and **no** `firmware.bin` SD-update path; release scans unchanged. |
| Restoration | Plan §9.5: write back the full 16 MB dump, then `verify_flash`; the production factory image (SHA-verified) as fallback; §9.6 checks |

## What it would let us measure (plan §5.5, §8.3)

- the X4 Pro checklist on the production route list, including the no-main state;
- `ONPTB` timing logs (model build, input/render, legacy ranking call, `getDepartureState`, draw vs refresh);
- heap and PSRAM readings;
- confirmation of `sizeof(OnPointActivity)`.

## Needs approval

1. Adding `CROSSPOINT_FIRMWARE_UPDATES_DISABLED` gating in `SettingsActivity` (a stock CrossPoint screen, dev builds
   only).
2. Adding the env and the guard extensions.
3. Separately: authorisation to flash, under plan §9.
