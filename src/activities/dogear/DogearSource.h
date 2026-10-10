#pragma once

#include "DogearIssue.h"

// Where the loaded issue came from (same semantics as GOTO's edition origin):
//   Network      - downloaded and hash-verified this session.
//   CacheCurrent - the SD copy, confirmed current by this session's manifest.
//   CacheStale   - the SD copy, unverified (no network or server): "CACHED".
//   None         - no readable issue anywhere. There is no compiled-in issue.
enum class DogearOrigin { Network, CacheCurrent, CacheStale, None };

// Load the current issue once per reading session, offline-first:
//   1. Network, only if Wi-Fi is already up: GET <server>/current.json, then the
//      issue it names unless the SD copy has the same SHA-256. A download is
//      verified against that hash and parsed before it replaces the cache.
//   2. SD cache (/dogear/active.txt -> /dogear/issues/<id>.<sha16>.json).
DogearOrigin loadCurrentDogearIssue(DogearIssue& out);

bool dogearWifiConnected();
