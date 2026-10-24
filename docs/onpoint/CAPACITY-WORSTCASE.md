# ON POINT worst-case capacity (Stage 1, §7–§8, §10)

Measured on Stage 1 commit `adb583f6` in a clean, disposable, detached worktree (compatibility run V0). Every build
was clean and forced (fresh `.pio/build/<env>`, empty build cache).

The fixture is **synthetic** and lives only in the dev envs `x4pro-all-capacity` / `x4pro-all-capacity-bench`.
It is 60 route pairs (120 schedules) with these departure lists:

| Rule | Trips | First trip |
|---|---|---|
| Weekday | 130 | 04:00 |
| Saturday | 120 | 05:00 |
| Sunday | 108 | 06:00 |

Every schedule has 40 date overrides, and every string is at its worst-case length (see
`test/fixtures/on_point/gen_capacity_fixture.py`).

## Flash and RAM (GPD, the largest package; app slot 6,553,600 B)

| Build | Image bytes | Slot headroom | `.flash.text` | `.flash.rodata` | Static RAM (`.dram0.data`+`.bss`) |
|---|---|---|---|---|---|
| v1.2.0 published | 5,763,440 | 790,160 (12.06%) | — | — | — |
| v1.2.0 local rebuild | 5,763,600 | — | 2,227,604 | 3,416,244 | 101,871 |
| Stage 1, 7 shipped routes (`x4pro-all-gh_release`) | 5,764,480 | **789,120 (12.04%)** | 2,228,296 | 3,416,436 | 101,871 |
| Stage 1 + 60-route fixture (`x4pro-all-capacity`) | 5,947,600 | **606,000 (9.25%)** | 2,228,336 | 3,599,508 | 101,871 |
| Stage 1 + fixture + timing hooks (`x4pro-all-capacity-bench`) | 5,950,144 | 603,456 | 2,230,400 | 3,599,988 | 101,879 |

**Stage 1 code cost vs v1.2.0:** +880 B image (+692 B text, +192 B rodata). Static RAM is unchanged.

**Fixture cost vs Stage 1:** +183,120 B image (+183,072 B rodata, +40 B text). Static RAM is unchanged, because the
fixture is entirely flash-resident.

The post-link check (`scripts/check_capacity_fixture.py`) passed, with no findings:
- **Arrays:** 360 departure arrays, each in `.flash.rodata` at its own address, 85,920 B in total.
- **Tables:** 120 rule tables (4,320 B) and 120 override tables (57,600 B); `FIXTURE_SCHEDULES` 6,240 B and
  `FIXTURE_ROUTES` 1,200 B.
- **Strings:** every distinct fixture string is present in the image (20 origins, 40 destinations, 120 operators,
  120 notes, 120 areas, 120 IDs).
- **Totals:** the raw fixture is 186,120 B, and `.flash.rodata` grew by 183,072 B. That's consistent with the
  roughly 3 KB of shipped timetables the fixture replaces; nothing was merged or dropped.

**Per worst-case route pair:** about 3.1 KB of flash (186,120 B / 60). The shipped routes average far less: ON
POINT's whole rodata for 7 routes is 2,336 B.

**Projection (GPD, worst-case pairs):**
- after the 60-pair fixture, 606,000 B of headroom remains;
- at about 3.1 KB per pair, roughly 89 more worst-case pairs fit before reaching the 5% floor (327,680 B).

The route-list cap `MAX_CATALOGUE_ROUTES` = 128 binds first.

**Heap-resident model:**
- `sizeof(on_point::RouteListModel)` = **268 B** on the device (xtensa toolchain; 280 B on the 64-bit host).
- 🔶 The bench's log call passes `sizeof(OnPointActivity)` as **500 B** (read from the disassembly of
  `on_point_bench::runOnEnter`). It will be confirmed by the device log when flashing is authorised.

The model lives inside the heap-allocated `OnPointActivity` and adds no allocation of its own.

## Host benchmarks (Apple M3 Pro, `-O3` Release; median of 30 reps unless noted)

Comparator invocations are the machine-independent metric; times are indicative only.

| Dataset | Layer | Implementation | Comparisons | Median | p95 |
|---|---|---|---|---|---|
| shipped 7 | one-time model build | new | 11 | 69 ns | 69 ns |
| shipped 7 | one list input + render | **new** | **0** | 9 ns | 9 ns |
| shipped 7 | one list input + render | legacy v1.2.0 | 3,378 | 15.5 µs | 26.2 µs |
| shipped 7 | `getDepartureState` | shared | — | 0.33 ms | 0.46 ms |
| fixture 60 | one-time model build | new | 59 (worst case 1,770) | 1.6 µs | 2.3 µs |
| fixture 60 | one list input + render | **new** | **0** | 8 ns | 19 ns |
| fixture 60 | one list input + render | legacy v1.2.0 | **13,529,526** | **293 ms** (7 reps) | 355 ms |
| fixture 60 | `getDepartureState` | shared | — | **2.0 ms** | 3.8 ms |

- **"One list input + render"** is the exact v1.2.0 call sequence:
  - selected position, page start, visible rows;
  - the pinned slot and every visible row;
  - NavNext;
  - one touch hit-test.
- **Model build** is an insertion sort that runs once per entry into ON POINT, plus after each hold-to-set. The
  fixture's catalogue order is nearly sorted, so 59 comparisons is close to the best case; the worst case is
  n(n−1)/2 = 1,770.
- **Legacy at 60 routes:** 13.5 million comparisons per input is about 0.3 s even on an M3 Pro, and would be seconds
  per press on the X4 Pro. This confirms the audit's O(n⁴) finding.
- **`getDepartureState` is unchanged by Stage 1.** Its cost scales with each schedule's departures × date overrides
  over the 371-day search, not with route count. The worst-case fixture schedule is about 6× the shipped
  Balagtas ↔ TriNoma case. The provisional device threshold (median ≤ 20 ms) is **at risk**: a typical host-to-ESP32
  ratio would put it near or above 20 ms. It stays open until measured, and an early exit in the 371-day scan is a
  candidate for a later stage.

## Device measurements: NOT RUN (flashing not authorised, D4)

`x4pro-all-capacity-bench` is built and ready. When it is authorised, opening ON POINT logs `ONPTB` lines with
microsecond medians, p95, min and max over 27 kept of 30 repetitions for:
- model build;
- the new input/render sequence;
- one legacy ranking call (the full legacy sequence would trip the watchdog at 60 routes, so it is projected from
  the host comparison counts);
- `getDepartureState`.

Every render also logs draw and refresh µs separately, per refresh mode. The procedure is plan §9.

## Thresholds (plan §10)

| Metric | Threshold | Result |
|---|---|---|
| GPD headroom with the 60-route fixture | ≥ 327,680 B (5%) | **606,000 B (9.25%): PASS** |
| Static internal RAM vs baseline | Δ ≤ 1 KB | **Δ 0 B (bench env +8 B): PASS** |
| Heap-resident model | ≤ 0.5 KB | **268 B: PASS** |
| Comparator calls per input, 60 routes, new vs legacy | new ≤ 1% of legacy, O(n) or better | **0 vs 13,529,526: PASS** |
| Route-model layer per input, X4 Pro | PROVISIONAL median ≤ 1 ms | not measured (device) |
| `getDepartureState`, worst fixture schedule, X4 Pro | PROVISIONAL median ≤ 20 ms | not measured; **host 2.0 ms suggests risk** |
| Draw / refresh vs v1.2.0 | ±5% | not measured (device) |
