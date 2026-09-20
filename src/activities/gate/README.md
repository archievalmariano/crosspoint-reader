# The Gate Is Open! — CrossPoint integration (Phase 2A)

First playable integration of the portable **The Gate Is Open!** narrative engine
as a CrossPoint Activity, targeting the ESP32-C3 XTEINK X4 (`env:default`).

## What lives where

- **Canonical engine/app/content/tests:** `~/Downloads/the-gate-is-open`
  (the Gate repo, **unchanged** by this integration). The engine is portable
  C++17 with no CrossPoint dependencies.
- **This directory (`src/activities/gate/`)** — CrossPoint-side glue only:
  - `GateActivity.{h,cpp}` — a thin Activity that hosts a `gate::Game`, maps
    buttons to `Game::handle()`, and renders `Game::buildScreen()` with
    `GfxRenderer`. It does **not** use `Game::run()` or a blocking input loop.
  - `gate_tu_*.cpp` — one-line forwarding translation units that `#include` each
    portable Gate `.cpp`, so each compiles as its own TU (a single unity build
    would collide on the engine's per-file anonymous-namespace helpers).
  - `GateContent.generated.h` — the Chacha story + cast/relationships embedded
    verbatim as strings (validated at runtime by the engine; no narrative logic
    is hardcoded).
  - `tools/embed_content.py` — regenerates the content header from the Gate repo.

## How the Gate sources enter the build

`platformio.ini` `[env:default]` adds the Gate repo's `engine/`, `platform/`,
`app/` include dirs (via `${sysenv.HOME}/Downloads/the-gate-is-open/...`) plus the
repo root. The forwarding TUs then resolve `#include "engine/src/Json.cpp"` etc.
No source is duplicated; the Gate repo is the single source of truth.

If the two repos are not both under `~/Downloads`, adjust those `-I` paths.

## Regenerating content

After editing the Gate story/cast:

```
python3 src/activities/gate/tools/embed_content.py [GATE_REPO_DIR]
```

Default `GATE_REPO_DIR` is `~/Downloads/the-gate-is-open`.

## Launch

Home menu → **"The Gate Is Open!"** (added as a `HomeMenuItem::GATE` entry in
`ActivityManager.h` + `HomeActivity`). Back at the Gate title screen returns to
CrossPoint home.

## Device profile

`GateDeviceProfile.h` is a tiny capability layer (selected from
`FREEINK_CAP_TOUCH`, not product names): `inputMode` (NavButtons /
TouchAndButtons), `hasTouch`, `name`, and a one-line `controlHint`. It drives
input routing and the Title-screen control hint. One profile, one renderer, many
targets — see [`docs/gate-device-compatibility.md`](../../../docs/gate-device-compatibility.md).

## Input mapping

The X4 physically has Back / Confirm / Left / Right / Up / Down (ADC ladder).
Gate uses the SDK's device-correct semantic buttons:

| Physical (semantic) button | Gate action |
|----------------------------|-------------|
| `NavPrevious` (= Up / Left; also `PageBack`) | Up |
| `NavNext` (= Down / Right; also `PageForward`) | Down |
| `Confirm` | Select / Continue / Begin |
| `Back` | Back (at Title: exit to CrossPoint home) |
| screen tap (touch devices only) | Select — seam only, see below |

The on-screen footer is the native theme hint bar (`mapLabels()` +
`GUI.drawButtonHints()`), so labels appear at the device's real button positions
instead of generic desktop text. The portable `Screen::footer` (desktop key
hints) is intentionally ignored on device.

## Touch seam (touch-capable devices only)

On `TouchAndButtons` devices a screen tap routes to `Game::handle(Select)`
(confirms the highlighted choice), proving taps reach the portable engine. It is
inert on the button-only X4 (`wasScreenTapped()` returns false). Per-choice
hit-testing (move the cursor to the tapped row) is the documented next step and
needs no engine change. **Untested** — no touch hardware here.

## Refresh behavior

`render()` uses a full refresh (`HalDisplay::FULL_REFRESH`) on a scene change
(phase / node / kind change, and every Beat) and a fast refresh
(`FAST_REFRESH`) for cursor movement within the same scene. Every input that
changes state calls `requestUpdate()`; the CrossPoint render task then calls
`render()`.

## Deferred for this phase

- **Save/load** is not wired to CrossPoint storage yet (the Gate Platform is
  all-null on device). The engine's save architecture is unchanged; this is a
  documented Phase 2A limitation, not fake persistence.
- Production artwork (art ids render as labelled placeholder boxes).
- Shadow/Chico slices, new content, X4 Pro specifics.
