#pragma once

#include <string>

#include "GotoEdition.h"

// Where the loaded edition came from (for logging / future status UI).
enum class GotoEditionOrigin { Network, Cache, Builtin, None };

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
