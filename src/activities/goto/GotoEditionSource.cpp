#include "GotoEditionSource.h"

#include <ArduinoJson.h>
#include <HalStorage.h>
#include <Logging.h>
#include <WiFi.h>
#include <mbedtls/sha256.h>

#include <cstdio>
#include <string>

#include "GotoLimits.h"
#include "network/HttpDownloader.h"

// Base URL of the GOTO publication runtime. Production is the hosted Cloudflare
// Pages endpoint; the device resolves the active edition through
// <base>/current.json -> <base>/e/<opaque-token>/edition.json (never a
// date-derived or archive URL). It is a build-time constant because the device
// has no UI to enter it; a dev host may override it in platformio.local.ini,
// e.g. build_flags = -DGOTO_SERVER_BASE=\"http://192.168.1.50:8080\".
#ifndef GOTO_SERVER_BASE
#define GOTO_SERVER_BASE "https://goto.archievalmariano.com"
#endif

namespace {
constexpr char kServerBase[] = GOTO_SERVER_BASE;
constexpr char kCacheDir[] = "/goto";
constexpr char kCacheEditionsDir[] = "/goto/editions";
constexpr char kCacheManifestPath[] = "/goto/current.json";

bool wifiUp() { return WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0); }

std::string cacheEditionPath(const std::string& editionId) {
  return std::string(kCacheEditionsDir) + "/" + editionId + ".json";
}

// Lowercase hex SHA-256 of a byte string (mbedtls; hashing a ~7 KB edition takes
// a few ms on the C3). Verifies a downloaded edition matches the manifest digest
// and compares cached vs server content identity.
std::string computeSha256Hex(const std::string& data) {
  uint8_t digest[32];
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, /*is224=*/0);
  mbedtls_sha256_update(&ctx, reinterpret_cast<const uint8_t*>(data.data()), data.size());
  mbedtls_sha256_finish(&ctx, digest);
  mbedtls_sha256_free(&ctx);
  char hex[65];
  for (int i = 0; i < 32; ++i) snprintf(hex + i * 2, 3, "%02x", digest[i]);
  return std::string(hex, 64);
}

// Parse the current.json manifest. editionSha256 is absent in pre-E1B2.5
// manifests -> returned as "" (which forces a re-fetch, safely).
// Fetch `url` into `out`, aborting (and rejecting) once ACTUAL received bytes
// exceed `maxBytes` — independent of any declared Content-Length or chunking, so
// a lying/oversized/streaming response cannot exhaust the heap. On overflow or
// transport failure, `out` is cleared and false is returned.
bool fetchBounded(const std::string& url, std::string& out, size_t maxBytes) {
  out.clear();
  bool overflow = false;
  const bool ok = HttpDownloader::fetchUrl(url, [&](const uint8_t* data, size_t len) -> bool {
    if (out.size() + len > maxBytes) {
      overflow = true;
      return false;  // abort the transfer
    }
    out.append(reinterpret_cast<const char*>(data), len);
    return true;
  });
  if (overflow) {
    LOG_ERR("GOTO", "response for %s exceeded %u-byte cap; rejecting", url.c_str(), static_cast<unsigned>(maxBytes));
    out.clear();
    return false;
  }
  return ok;
}

bool parseManifest(const char* json, std::string& editionId, std::string& editionPath, std::string& editionSha,
                   std::string& companionUrl) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return false;
  editionId = doc["editionId"] | "";
  editionPath = doc["editionPath"] | "";
  editionSha = doc["editionSha256"] | "";
  // Opaque hosted companion-page URL for the whole-edition QR (may be absent on a
  // pre-companion manifest); the device never derives it from the date/editionId.
  companionUrl = doc["companion"]["url"] | "";
  // Reject a manifest whose untrusted fields are unsafe or oversized. editionId is
  // used to build an SD path, so it must pass the safe-filename grammar.
  if (!goto_limits::isSafeEditionId(editionId)) return false;
  if (editionPath.empty() || editionPath.size() > goto_limits::kMaxEditionPathLen) return false;
  if (editionSha.size() > goto_limits::kMaxShaLen) return false;
  if (companionUrl.size() > goto_limits::kMaxUrlLen) {
    companionUrl.clear();  // drop an oversized companion URL; the edition still loads
  }
  return true;
}

// Content hash recorded in the SD cache manifest for `editionId` (empty if no
// manifest, a different editionId, or a pre-hash manifest).
std::string cachedEditionSha256(const std::string& editionId) {
  if (!Storage.exists(kCacheManifestPath)) return "";
  const String manifest = Storage.readFile(kCacheManifestPath);
  if (manifest.length() == 0) return "";
  std::string id;
  std::string path;
  std::string sha;
  std::string companionUrl;
  if (!parseManifest(manifest.c_str(), id, path, sha, companionUrl)) return "";
  return (id == editionId) ? sha : std::string();
}

// Load the edition currently recorded in the SD cache manifest. Returns true and
// fills out/editionId on success.
bool loadFromCache(GotoEdition& out, std::string& editionId) {
  if (!Storage.exists(kCacheManifestPath)) return false;
  const String manifest = Storage.readFile(kCacheManifestPath);
  std::string cachedPath;
  std::string cachedSha;
  std::string companionUrl;
  if (manifest.length() == 0 || !parseManifest(manifest.c_str(), editionId, cachedPath, cachedSha, companionUrl))
    return false;
  const String edition = Storage.readFile(cacheEditionPath(editionId).c_str());
  if (edition.length() == 0 || !parseGotoEdition(edition.c_str(), out)) return false;
  out.companionUrl = companionUrl;  // whole-edition QR works offline from cached metadata
  return true;
}
}  // namespace

bool gotoWifiConnected() { return wifiUp(); }

GotoLoadResult loadCurrentGotoEdition(GotoEdition& out) {
  GotoLoadResult result;

  // 1) Network path, only if Wi-Fi is already connected. GotoActivity opens
  //    CrossPoint's Wi-Fi picker first when it is not.
  if (wifiUp()) {
    std::string manifestJson;
    if (fetchBounded(std::string(kServerBase) + "/current.json", manifestJson, goto_limits::kMaxManifestBytes)) {
      std::string editionId;
      std::string editionPath;
      std::string editionSha;
      std::string companionUrl;
      if (parseManifest(manifestJson.c_str(), editionId, editionPath, editionSha, companionUrl)) {
        const std::string localPath = cacheEditionPath(editionId);
        const std::string cachedSha = cachedEditionSha256(editionId);

        // Cache identity is (editionId + content hash), not editionId alone: a
        // same-id --force correction changes editionSha256. Reuse the cache only
        // when the file exists AND its recorded hash matches the server's (a
        // pre-hash/empty cachedSha never matches -> re-fetch).
        const bool cacheIsCurrent = Storage.exists(localPath.c_str()) && !editionSha.empty() && cachedSha == editionSha;

        if (cacheIsCurrent) {
          Storage.writeFile(kCacheManifestPath, String(manifestJson.c_str()));  // refresh pointer
          const String cached = Storage.readFile(localPath.c_str());
          if (cached.length() > 0 && parseGotoEdition(cached.c_str(), out)) {
            out.companionUrl = companionUrl;
            result.origin = GotoEditionOrigin::CacheCurrent;  // verified current -> no marker
            result.editionId = editionId;
            LOG_INF("GOTO", "edition %s hash matches cache; serving cache (live)", editionId.c_str());
            return result;
          }
          // Cache unreadable despite a hash match: fall through and re-download.
        }

        // Download the new or revised edition. Verify its bytes against the
        // manifest hash BEFORE promoting to cache; a mismatch or parse failure
        // must never replace a good cached edition.
        std::string editionJson;
        if (fetchBounded(std::string(kServerBase) + "/" + editionPath, editionJson, goto_limits::kMaxEditionBytes)) {
          bool hashOk = true;
          if (!editionSha.empty()) {
            const std::string dlSha = computeSha256Hex(editionJson);
            hashOk = (dlSha == editionSha);
            if (!hashOk)
              LOG_ERR("GOTO", "edition hash mismatch (got %s want %s); rejecting download", dlSha.c_str(),
                      editionSha.c_str());
          }
          GotoEdition fresh;
          if (hashOk && parseGotoEdition(editionJson.c_str(), fresh)) {
            // Edition file written before the manifest, so the manifest never
            // points at an edition not on disk.
            Storage.ensureDirectoryExists(kCacheDir);
            Storage.ensureDirectoryExists(kCacheEditionsDir);
            // Edition file first, manifest pointer only if it succeeded — so the
            // cache is only advanced when BOTH writes land (never a manifest
            // pointing past a missing/partial edition). The freshly fetched
            // edition still renders this session regardless (origin = Network).
            const bool cached = Storage.writeFile(localPath.c_str(), String(editionJson.c_str())) &&
                                Storage.writeFile(kCacheManifestPath, String(manifestJson.c_str()));
            if (!cached) LOG_ERR("GOTO", "edition cache write incomplete; offline copy not updated");
            out = std::move(fresh);
            out.companionUrl = companionUrl;
            result.origin = GotoEditionOrigin::Network;
            result.editionId = editionId;
            LOG_INF("GOTO", "downloaded edition %s (hash verified)%s", editionId.c_str(),
                    cached ? "; cached" : " [cache write failed]");
            return result;
          }
          LOG_ERR("GOTO", "downloaded edition invalid; keeping prior cache");
        } else {
          LOG_ERR("GOTO", "edition download failed; keeping prior cache");
        }
      } else {
        LOG_ERR("GOTO", "current.json manifest parse failed");
      }
    } else {
      LOG_ERR("GOTO", "current.json fetch failed");
    }
    // Any network failure falls through to the offline cache below.
  }

  // 2) Offline cache — last-known SD edition, used WITHOUT live verification.
  std::string cachedId;
  if (loadFromCache(out, cachedId)) {
    result.origin = GotoEditionOrigin::CacheStale;
    result.editionId = cachedId;
    LOG_INF("GOTO", "loaded edition %s from SD cache (unverified/offline)", cachedId.c_str());
    return result;
  }

  // 3) Compiled-in fixture (no network, empty cache).
  if (loadBuiltinGotoEdition(out)) {
    result.origin = GotoEditionOrigin::Builtin;
    LOG_INF("GOTO", "loaded compiled-in fixture edition (no network/cache)");
    return result;
  }

  result.origin = GotoEditionOrigin::None;
  return result;
}

bool cachedCurrentIsTogo() {
  // Home launcher label only — read the persisted manifest, never the network.
  if (!Storage.exists(kCacheManifestPath)) return false;
  const String manifest = Storage.readFile(kCacheManifestPath);
  if (manifest.length() == 0) return false;
  JsonDocument doc;
  if (deserializeJson(doc, manifest.c_str())) return false;
  const char* edition = doc["edition"] | "";
  return std::string(edition) == "TOGO";
}
