# The Gate Is Open! — device compatibility model

**One game / one engine / one renderer where possible / multiple device
profiles and firmware targets.**

Gate is a single portable narrative engine (the `the-gate-is-open` repo, built
under `-fno-exceptions -fno-rtti`). CrossPoint hosts it with a thin
`GateActivity` (`src/activities/gate/`). Devices differ only in a small
capability-driven **device profile**, never in the engine, story schema, save
format, `Screen` model, or narrative rendering. Different hardware is a new
*profile/target*, not a fork.

## Terminology

- **Tested** — physically run and checked on real hardware.
- **Compile-validated** — the firmware builds with Gate enabled, but no physical
  behavior is claimed.
- **Planned** — profile/integration path identified, not yet built here.

## What varies, and what doesn't

| Concern | How it's handled |
|---|---|
| Display size | **Runtime** (`renderer.getScreenWidth/Height`). One renderer; no per-device layout. All current targets are 800×480 mono. |
| Refresh | Generic (full refresh on scene change, fast on cursor move). No profile field. |
| Input | Semantic buttons (`NavPrevious`/`NavNext`/`Confirm`/`Back`) via the SDK's `MappedInputManager`, identical across button devices. |
| Touch | Device **profile** (`GateInputMode::TouchAndButtons`) selected from `FREEINK_CAP_TOUCH`. |
| Control hints | Native `mapLabels()` + theme `drawButtonHints()` — device-correct automatically. Profile also carries a one-line `controlHint`. |

The profile (`src/activities/gate/GateDeviceProfile.h`) is chosen from FreeInk
**capability macros**, not product-name conditionals:

```cpp
#if FREEINK_CAP_TOUCH   // MURPHY, LILYGO, M5PAPER, STICKY, X4PRO, PAPERMONO, ...
  TouchAndButtons
#else                    // X3, X4, X4 Classic, ...
  NavButtons
#endif
```

Flow: `CrossPoint target/capabilities → Gate device profile → GateActivity
input/render hints → portable Game`. The engine never sees a product name.

## Compatibility matrix

Verified from `freeink-sdk` `BoardConfig.h` capability macros and the
device-support docs, plus the builds run in this pass.

| Device / family | Build env | MCU | Display (runtime) | Touch | Gate input profile | Status |
|---|---|---|---|---|---|---|
| XTEINK X4 / X3 | `default` | ESP32-C3 | 800×480 mono | no | NavButtons | **TESTED** on X4 (X3 shares the binary, untested) |
| XTEINK X4 Classic | `x4c` | ESP32-S3 | 800×480 mono | no | NavButtons | Compile-validated |
| XTEINK X4 Pro | `x4pro` | ESP32-S3 | 800×480 mono | yes (GT911) | TouchAndButtons | Compile-validated |
| M5 Paper Mono | `papermono` | ESP32-S3 | 800×480 mono | yes | TouchAndButtons | Compile-validated |
| reTerminal Sticky | `sticky` | ESP32-S3 | 800×480 mono | yes (GT911) | TouchAndButtons | Planned (same profile as x4pro/papermono; not built here — its `firmware_tuned` custom-core rebuild was skipped) |

All current targets are 800×480, so the single runtime-sized renderer covers the
whole family with no layout variants.

### Enabling a target

An env enables Gate by pulling in `${gate.build_flags}` (see `platformio.ini`
`[gate]`). Envs without it compile all `src/activities/gate` code out via
`GATE_ENABLED` and are byte-identical. Currently enabled: `default`, `x4pro`,
`x4c`, `papermono`.

## Touch integration path (seam only)

Touch-capable profiles route a screen tap to `Game::handle(Select)` today
(confirms the highlighted choice — proves taps reach the engine). The next step,
which needs **no engine change**, is per-choice hit-testing: record each menu
row's rect during `drawMenu`, and on a tap move the cursor to the tapped row
before selecting. Not implemented yet, and untested (no touch hardware here).

## Future download model (not built yet)

A future Gate download page lets the user pick their device and serves the
matching firmware binary + microSD instructions. These are firmware/build
targets, not forks — several product names may share one binary/profile when the
underlying CrossPoint target genuinely supports it:

```
Gate source  ->  CrossPoint builds  ->  { X3/X4, X4 Classic, X4 Pro, Sticky, M5 Paper Mono }
```

## Known CrossPoint-wide concern (not a Gate issue)

With many apps installed, the CrossPoint Home launcher/theme can crowd and Gate's
entry may be partially obscured in some themes. This is a launcher/theme scaling
concern to address CrossPoint-wide, not inside Gate.
