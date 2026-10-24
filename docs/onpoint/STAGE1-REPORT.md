# ON POINT Stage 1 report (foundations and capacity)

The plan is Revision 3, accepted by Codex with conditions. Branch `feature/onpoint-stage1`, from tag `v1.2.0`
(`85bad542`), **local only**. Nothing was pushed, tagged, merged into a protected branch, released, deployed or
flashed. No region model, no regional UI, and no real-world route or timetable change.

## Commits

All authored and committed as `Archieval Mariano <307380168+archievalmariano@users.noreply.github.com>`, with no
assistant co-author.

| SHA | Summary |
|---|---|
| `8358d31a` | refactor(on-point): add a cached route-list model and saved-route resolver |
| `3899de50` | fix(on-point): sort the route list once and stop substituting unknown routes |
| `4cdf7f36` | test(on-point): synthetic 60-route capacity fixture, guards and benchmarks |
| `adb583f6` | style(on-point): RouteListModel::pageStart is static (cppcheck functionStatic) |
| (this commit) | docs(on-point): Stage 1 capacity baseline, worst case and report |

## What changed

1. **`OnPointRouteList`** (new) sorts the route pairs once, with the v1.2.0 comparator unchanged: outbound origin,
   then destination, then pair ID, by `strcmp`. It caches:
   - the order, the inverse position map and the pinned slot (`RouteListModel`, 268 B on the device);
   - which provide display order, paging, NavNext/NavPrevious, swipe paging and touch hit-testing.

   It replaces the v1.2.0 per-call re-ranking (O(n⁴) per selected-index lookup).
2. **`OnPointRouteResolution`** (new) resolves the saved main-route ID to **Canonical**, **Aliased**, **Unset** or
   **Unknown**:
   - it reads no further than the settings buffer, never writes, and never substitutes a route;
   - the alias table ships empty.
3. **`OnPointActivity`** builds the model when ON POINT opens, and rebuilds it only after a hold-to-set. For every
   saved main route that resolves, behaviour is identical to v1.2.0 (golden tests).
4. **No-main-route state** (D2, C1, C4, Q2), when the saved value is **explicitly** empty or doesn't resolve:
   - the first route in sort order, **BALAGTAS → TRINOMA**, fills the pinned slot;
   - the heading reads **NO MAIN ROUTE SET** (new key `STR_ON_POINT_NO_MAIN_ROUTE`, shown only in this state);
   - every route shows "HOLD TO SET AS MAIN";
   - holding any route, the pinned one included, saves it;
   - the stored value is never rewritten except by that hold.

   A missing settings file or key keeps the compiled default `p2p-balagtas-trinoma` (CrossPointSettings::fromJson,
   pinned by a test).
5. **Capacity fixture and guards** (test/dev only): the generator, the dev envs `x4pro-all-capacity` and
   `-bench`, release-exclusion checks, the post-link check, host fixture tests and benchmarks.

The holiday-touched files (`OnPointScheduleData.cpp`, `MarkedDates.h`, `test/on_point_schedule/*`,
`test/utilities/*`) are **untouched**. The legacy `routeIndexInAlphabeticalOrder` / `routeIndexForId` stay in place
and keep their existing tests; the Activity no longer calls them.

## Tests

- **Host tests:** **586/586** pass on Stage 1. New:
  - `OnPointRouteListTest` (12): golden v1.2.0 order, paging, navigation and swipe for all 7 mains; the no-main
    state; aliased; out-of-range; legacy equivalence; touch edges, gaps and off-screen probes; departure states
    pinned for all 14 schedules at weekday, weekend, holiday and evening instants; the shipped catalogue is
    fixture-free.
  - `OnPointRouteResolutionTest` (10): every input case, including null, empty, case mismatch, trailing space,
    truncation, a 47-character unknown, an unterminated buffer, aliases, no writes, and the settings initializer
    equal to the default.
  - `OnPointCapacityFixtureTest` (13).
  - The guard script tests (10, via ctest).
- **`pio check`** (low/medium/high) on `default`: **PASSED**, with no findings.
- **Formatting:** `./bin/clang-format-fix -g` applied.

## Baseline provenance

See `CAPACITY-BASELINE-v1.2.0.md`.
- **Published release:** v1.2.0's 17 assets verified against `SHA256SUMS.txt` and GitHub digests.
- **Rebuilds:** clean local rebuilds of P, PD, GP and GPD.
- **Result: normalised-equivalent with attributed differences, not byte-reproduced.** The differences are build
  time, the CI build path (+160 B in each package = 32 embedded paths × 5 B), and the gzip output of 5 embedded web
  pages.
- **Pinned build time (N2) is not applicable:** CI's two compile times can't be expressed by one
  `SOURCE_DATE_EPOCH`.

## Capacity and benchmarks

See `CAPACITY-WORSTCASE.md`.
- **Flash:** GPD headroom is 789,120 B (12.04%) with the shipped routes, and 606,000 B (9.25%) with the 60-route
  worst-case fixture.
- **RAM:** static RAM is unchanged.
- **Route list:** zero comparator calls per input, against 13.5 million for the v1.2.0 logic at 60 routes.
- **Departure computation:** `getDepartureState` for the worst fixture schedule takes about 2.0 ms on the host. The
  provisional 20 ms device threshold is at risk and unmeasured.

## Compatibility matrix (plan §2.3, C3)

Each run used a disposable detached worktree, temporary merge commits (no refs moved), recursive submodule init,
full ctest, clean builds, and a fixture scan of every release image and ELF.

| Run | Base + temporary merges | Conflicts | ctest | Builds P / PD / GP / GPD / X4 G (bytes) | Fixture scan |
|---|---|---|---|---|---|
| V0 | Stage 1 `adb583f6` | — | 586/586 | 5,680,032 / 5,746,976 / 5,697,760 / 5,764,480 / 5,643,536 | clean (capacity envs: markers present, as expected) |
| V1 | v1.2.1-maint `353d48b0` + Stage 1 | none | 590/590 | 5,680,096 / 5,747,024 / 5,697,808 / 5,764,528 / 5,643,600 | clean |
| V2 | holidays `4467604b` + Stage 1 | none | 591/591 | 5,680,912 / 5,747,856 / 5,698,640 / 5,765,376 / 5,643,968 | clean |
| V3 | `353d48b0` + holidays + Stage 1 | none | 595/595 | 5,680,976 / 5,747,904 / 5,698,688 / 5,765,424 / 5,644,016 | clean |

The protected refs (82, recorded before the work began) and the main working copy (`feature/ph-holidays-2027` @
`4467604b`, clean) were re-checked after every run and at the end: **unchanged**. The only ref added is
`feature/onpoint-stage1`.

## Deviations from the plan

1. **One bench env, not two.** `x4pro-all-capacity-bench` runs new and legacy measurements in one image. On the
   device, legacy is timed as one ranking call and projected from the host comparison counts: the full legacy
   sequence at 60 routes would take seconds and trip the task watchdog.
2. **Device bench location.** It lives in `src/activities/on_point/OnPointDeviceBench.cpp`, wrapped entirely in
   `#ifdef ON_POINT_BENCH_TIMING`, because sources added by a build script don't get the library include paths. It
   compiles to nothing in every other env; the config guard and artifact scan cover it.
3. **Include path.** `scripts/on_point_capacity_fixture.py` appends `src/` to `CPPPATH` for the two capacity envs
   only.
4. **Update repository marker.** The capacity image's update repository is set to a nonexistent repo
   (`invalid/on-point-capacity-build`) rather than compiling the update check out.

## Remaining hardware-only validation (needs flashing authorisation, D4/Q6)

1. **X4 Pro checklist (plan §5.5):**
   - list order identical to v1.2.0 for two different mains;
   - hold-to-set;
   - touch on every row, the pinned card and the gaps;
   - swipe paging, Back, and the departure board unchanged;
   - the no-main state via a test edit of `onPointMainRouteId` (§9.4), including the heading and the hold
     behaviour.
2. **Device timing from `x4pro-all-capacity-bench`:**
   - model build;
   - input-to-render;
   - one legacy ranking call;
   - `getDepartureState` (provisional ≤ 20 ms; host suggests risk);
   - draw vs refresh per mode;
   - and confirm `sizeof(OnPointActivity)`.
3. **Device heap and PSRAM readings** before, during and after ON POINT.

All of it follows the §9 backup and restoration procedure.

## Open items for Stage 2+ (not started)

- The region model and in-app selector;
- direct launch to the default board;
- per-region defaults;
- the alias table contents;
- removing the unused legacy functions after the holidays branch lands;
- a possible early exit in `getDepartureState`'s 371-day scan if device timing confirms the risk.

## Review fixes (Codex: GO WITH FIXES), commit `da598c56`

1. **Activity integration coverage.** `test/on_point_activity/` runs the production `OnPointActivity.cpp` against
   small stand-ins (`stubs/`: renderer recording text and rectangles, one-event input, settings with a save
   counter, `tr()` yielding the key name, Activity/RenderLock/ActivityManager). 14 tests cover:
   - entry with the default and another saved route;
   - NavNext order and wrap, NavPrevious;
   - hold-to-set (once), and persistence into a new entry;
   - the no-main state for unknown, case-mismatched and empty IDs (stored value untouched, saves 0 until a hold);
     holding the pinned route;
   - touch-down select, tap open, long-press set, gaps and off-screen taps;
   - swipe paging;
   - the departure view: reverse, then Back twice to Apps;
   - the clock-not-set screen;
   - input deferred during a render, except Back.

   A separate empty-catalogue executable covers the zero-route guards. Mutation checks: forcing the "MAIN ROUTE"
   heading fails 6 tests; removing the empty-list guard crashes the empty-catalogue test.
   - **Not covered on the host:** real fonts, layout and pixels, e-ink refresh, the touch classifier, button
     mapping and timing, and settings written to the SD card. These stay on the hardware list below.
2. **Zero-route catalogue.** `drawRouteList()` shows the header and TIMETABLE UNAVAILABLE. `openSelectedRoute`,
   `setSelectedAsMainRoute` and `updateState` do nothing without a route.
3. **Alias table with null entries** and a nonzero count is treated as empty (test added).
4. **Capacity images are never flashable:**
   - the capacity script refuses upload targets before building (a `-t upload` request to a nonexistent port was
     refused; esptool never started);
   - the envs' `upload_command` always fails;
   - the release guard requires both.

   An OTA-disabled device-test env was proposed here; it is now implemented: `DEVICE-TEST-CONFIG.md`.

### Results at `da598c56`

- **Host tests:** 602/602 on Stage 1.
- **Guard script tests:** 11.
- **`pio check`** (low/medium/high): PASSED.

| Run | Conflicts | ctest | P / PD / GP / GPD / X4 G (bytes) | Fixture scan |
|---|---|---|---|---|
| V0 `da598c56` | — | 602/602 | 5,680,176 / 5,747,136 / 5,697,920 / 5,764,640 / 5,643,536 | clean; capacity 5,947,744 and bench 5,950,288 contain markers (expected); post-link check passed |
| V1 + v1.2.1 | none | 606/606 | 5,680,240 / 5,747,184 / 5,697,952 / 5,764,688 / 5,643,600 | clean |
| V2 + holidays | none | 607/607 | 5,681,072 / 5,748,016 / 5,698,784 / 5,765,520 / 5,643,968 | clean |
| V3 + both | none | 611/611 | 5,681,136 / 5,748,064 / 5,698,832 / 5,765,568 / 5,644,016 | clean |

- **GPD headroom:** 788,960 B with the shipped routes; **605,856 B (9.24%)** with the 60-route fixture.
- **Code cost:** the fixes added 160 B to GPD.
- **Protected refs:** unchanged after every run.
