# ON POINT Stage 2: regional collections, report

**Branch:** `feature/onpoint-stage2` (local): Stage 1 `8f44fb93` + P0–P3 + the device bench (`7341c3bf`).

| Phase | Commits |
|---|---|
| P0 departure computation | `95baf9f4` |
| P1 collection model | `d674c40a`, `d9185d1a` |
| P2 OnPointStore | `dbc133f8`, `5b648c15`, `321e949d` |
| P3 interface | `2b0998db` |
| P4 bench | `7341c3bf` |

## Hardware validation (X4 Pro, 2026-10-25)

**Image:** `x4pro-all-devtest`, built from v1.2.1-maint `353d48b0` + this branch at `7341c3bf`, in a temporary
integration merge. That merge's change on v1.2.1 is patch-identical to Stage 1 + 2 on v1.2.0.
- Version `1.6.5-A1.2.1-GPD-DEVTEST`, 5,751,360 B.
- SHA-256 `ce48bab8e17cc6b453b6726333bc7bc3448c27a564e94dc7c6ac32c9100864ce`.
- Partition table and bootloader byte-identical to v1.2.1's; no updater code (`--scan-devtest`).

**Install:** the established app-only USB write: `boot_app0` at `0xe000`, the app at `0x10000`.
`Hash of data verified.` ×2, and the `verify-flash` digests matched. NVS, partitions and the bootloader weren't
written.

**Owner checks (all PASS):**
- the identity;
- direct launch to the migrated main route's board;
- Back: board → list → Apps;
- the collection row by buttons and touch;
- the picker: contents, cancel, and switching among BULACAN, NORTH LUZON and AIRPORT TRANSFERS;
- a separate main route per collection;
- relaunch and full power-cycle persistence;
- the board unchanged;
- GOTO, DOGEAR, books, Tools and Wi-Fi.

**Serial capture:**
- 5 ON POINT entries and a power-cycle; no panics, watchdog resets, errors or unexpected resets.
- **Store:**
  - the first launch migrated (no file);
  - every save (2 main routes, 5 collection switches) returned Saved with nothing unsaved;
  - every launch after the first save loaded the live file, including after the power-cycle, which reopened the last
    collection.

**Measurements** (`ONPTB`, 27 samples per figure after 3 discarded):

| Measure | X4 Pro |
|---|---|
| `getDepartureState`, Stage 1 bench route (TRINOMA → CLARK) | **43 µs** median (Stage 1: 40,154 µs) |
| `getDepartureState`, all 14 schedules at 08:30 and 22:30 (28 cases) | min 42, median 46, max **137 µs** (BALAGTAS, which has overrides); max of any sample 142 µs |
| `RouteListModel::build` / per-input list update | 15 µs / 3 µs |
| Stack, lowest free | loop task 11,276 B (including saves); render task 10,600 B |
| `sizeof(OnPointActivity)` / `sizeof(RouteListModel)` | 1,404 B / 268 B |
| Internal heap | lowest free 125,724 B, lowest min-free 106,324 B, lowest largest block 77,812 B |
| PSRAM free | at least 8.23 MB |
| Draw | list 39 ms avg (46 max); board 38 ms; picker 10 ms |
| Refresh | 0.60 s (FAST) – 1.52 s (HALF) |

**Firmware size (release GPD, P3):** 5,776,912 B, with 776,688 B (11.9%) of the app slot free.
