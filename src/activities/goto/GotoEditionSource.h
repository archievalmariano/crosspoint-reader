#pragma once

#include <string>

#include "GotoEdition.h"

// Where the loaded edition came from, and how "live" it is:
//   Network      - downloaded fresh this session.
//   CacheCurrent - served from SD, but the server manifest was fetched this
//                  session and confirmed this editionId is current (live).
//   CacheStale   - network/server unavailable; last-known SD edition used
//                  WITHOUT live verification (shows the CACHED marker).
//   Builtin      - no network and no cache; compiled-in fixture (OFFLINE).
enum class GotoEditionOrigin { Network, CacheCurrent, CacheStale, Builtin, None };

struct GotoLoadResult {
  GotoEditionOrigin origin = GotoEditionOrigin::None;
  std::string editionId;  // empty for the builtin fixture
};

// Load the current edition for a fresh reading session, offline-first with a
// strict fallback chain:
//   1. Network  — only if Wi-Fi is already connected: fetch <server>/current.json,
//      and if its editionId is not already cached, download the edition, parse to
//      validate, then cache it to SD for offline reuse.
//   2. Cache    — the last edition persisted on SD (network down / unchanged).
//   3. Builtin  — the compiled-in fixture (no network, empty cache).
//
// This is called ONCE per reading session (GotoActivity::onEnter); it never
// re-fetches mid-session, so an edition that flips server-side while the user is
// reading does not swap underneath them — exit/reopen picks up the newer one.
GotoLoadResult loadCurrentGotoEdition(GotoEdition& out);

// Report whether the persisted cache manifest's current edition is TOGO (vs
// GOTO), for the Home launcher label. Reads only the SD cache manifest — NO
// network request — and falls back to false (GOTO) if no valid manifest exists.
bool cachedCurrentIsTogo();
