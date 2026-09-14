#include "GotoEditionSource.h"

#include <ArduinoJson.h>
#include <HalStorage.h>
#include <Logging.h>
#include <WiFi.h>
#include <mbedtls/sha256.h>

#include <cstdio>
#include <optional>
#include <string>

#include "WifiCredentialStore.h"
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

// Bounded silent-reconnect budget. Matched to CrossPoint's own proven
// auto-connect path (WifiSelectionActivity::AUTO_CONNECTION_TIMEOUT_MS = 7000),
// not lengthened arbitrarily: after a cold boot/wake the all-channel scan +
// associate can take slightly over the previous 6 s, which the Settings picker
// tolerates and GOTO did not. One-time cost on GOTO entry, only when not already
// connected and a saved network exists. delay() yields to the RTOS / feeds the
// watchdog, matching the blocking network work onEnter already does.
constexpr uint32_t kReconnectTimeoutMs = 7000;
constexpr uint32_t kReconnectPollMs = 200;

bool wifiUp() { return WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0); }

// If Wi-Fi is not connected but CrossPoint has a saved network, attempt a SILENT
// reconnect (no picker, no prompt, no new credential storage) using the last-
// connected saved credential. Returns true only once actually connected. Never
// opens UI and never blocks longer than kReconnectTimeoutMs.
bool ensureWifiConnected() {
  if (wifiUp()) return true;
  if (WIFI_STORE.getCredentialCount() == 0) {
    LOG_DBG("GOTO", "no saved Wi-Fi network; staying offline");
    return false;
  }

  std::optional<WifiCredential> cred;
  const std::string last = WIFI_STORE.getLastConnectedSsid();
  if (!last.empty()) cred = WIFI_STORE.findCredential(last);
  if (!cred) cred = WIFI_STORE.getCredentialAt(0);
  if (!cred) return false;

  LOG_INF("GOTO", "Wi-Fi down (status=%d); silent reconnect to saved network %s", (int)WiFi.status(),
          cred->ssid.c_str());

  // Mirror the COMPLETE CrossPoint connection lifecycle, not just WiFi.begin().
  // After a boot or deep-sleep wake the radio starts with modem power-save on
  // and a stale/half-initialized SDK auto-connect state; begin() alone then does
  // not reliably associate. CrossPoint's own reliable-STA paths (CrossPointWebServer,
  // KOReaderAuth) disable modem sleep and enable driver auto-reconnect, and its
  // Settings picker tears the stale state down before associating. Reproduce all
  // of it: persistent(false) -> mode(STA) -> setSleep(false) [the missing state
  // transition: modem power-save off, "critical for reliable operation" per the
  // web server] -> setAutoReconnect(true) [driver retries transient disconnects]
  // -> disconnect(true,true)+100ms [clear stale association + SDK NVS SSID] ->
  // all-channel scan/sort -> begin(). Reuses WifiCredentialStore only.
  WiFi.persistent(false);  // credentials owned by WifiCredentialStore, not SDK NVS
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.disconnect(true, true);
  delay(100);
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);

  // Explicit scan before begin(). This is the one concrete step the working
  // manual path performs that GOTO omitted: Settings->Wi-Fi opens the picker
  // with auto-connect OFF, which runs WiFi.scanNetworks() to populate the list
  // BEFORE the user's connect. After a cold boot/wake that scan appears to be
  // what warms/initializes the radio for a reliable association; a bare begin()
  // did not. Bounded, synchronous, hidden APs included. (Diagnostic-heavy: the
  // device is locked/no serial, so these logs are the seam for a future capture.)
  LOG_DBG("GOTO", "pre-connect: mode=%d status=%d; scanning...", (int)WiFi.getMode(), (int)WiFi.status());
  const int16_t found = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/true);
  LOG_DBG("GOTO", "pre-connect scan found %d networks", (int)found);

  if (!cred->password.empty()) {
    WiFi.begin(cred->ssid.c_str(), cred->password.c_str());
  } else {
    WiFi.begin(cred->ssid.c_str());
  }

  // Connect budget measured from begin() (unchanged at kReconnectTimeoutMs); the
  // scan above is separate warm-up time, not a lengthened connect timeout.
  const uint32_t start = millis();
  wl_status_t lastStatus = WL_IDLE_STATUS;
  while (millis() - start < kReconnectTimeoutMs) {
    if (wifiUp()) {
      WIFI_STORE.setLastConnectedSsid(cred->ssid);
      LOG_INF("GOTO", "silent reconnect succeeded (%.1fs, ip=%s)", (millis() - start) / 1000.0,
              WiFi.localIP().toString().c_str());
      return true;
    }
    const wl_status_t now = WiFi.status();
    if (now != lastStatus) {
      LOG_DBG("GOTO", "reconnect status %d -> %d @ %lums", (int)lastStatus, (int)now, millis() - start);
      lastStatus = now;
    }
    delay(kReconnectPollMs);
  }
  LOG_INF("GOTO", "silent reconnect timed out (status=%d); staying offline", (int)WiFi.status());
  return false;
}

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
  return !editionId.empty() && !editionPath.empty();
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

GotoLoadResult loadCurrentGotoEdition(GotoEdition& out) {
  GotoLoadResult result;

  // 1) Network path. If Wi-Fi is down but a saved network exists, attempt a
  //    bounded SILENT reconnect first (no interactive picker, no prompt). The
  //    reader open path never provisions Wi-Fi; that stays in CrossPoint's flow.
  if (ensureWifiConnected()) {
    std::string manifestJson;
    if (HttpDownloader::fetchUrl(std::string(kServerBase) + "/current.json", manifestJson)) {
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
        if (HttpDownloader::fetchUrl(std::string(kServerBase) + "/" + editionPath, editionJson)) {
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
            if (Storage.writeFile(localPath.c_str(), String(editionJson.c_str()))) {
              Storage.writeFile(kCacheManifestPath, String(manifestJson.c_str()));
            }
            out = std::move(fresh);
            out.companionUrl = companionUrl;
            result.origin = GotoEditionOrigin::Network;
            result.editionId = editionId;
            LOG_INF("GOTO", "downloaded edition %s (hash verified); cached", editionId.c_str());
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
