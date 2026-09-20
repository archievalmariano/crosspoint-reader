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

## Input mapping (button-only X4)

| Physical (semantic) button | Gate action |
|----------------------------|-------------|
| `NavPrevious` (also `Up`/`PageBack`) | Up |
| `NavNext` (also `Down`/`PageForward`) | Down |
| `Confirm` | Select / Continue |
| `Back` | Back (at Title: exit to CrossPoint home) |

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
