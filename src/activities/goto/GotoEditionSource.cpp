#include "GotoEditionSource.h"

#include <ArduinoJson.h>
#include <HalStorage.h>
#include <Logging.h>
#include <WiFi.h>

#include <optional>
#include <string>

#include "WifiCredentialStore.h"
#include "network/HttpDownloader.h"

// Base URL of the local publication server (see backend/goto/server.py). It is a
// build-time constant because the device has no UI to enter it; set the dev
// host's LAN address in platformio.local.ini, e.g.:
//   build_flags = -DGOTO_SERVER_BASE=\"http://192.168.1.50:8080\"
#ifndef GOTO_SERVER_BASE
#define GOTO_SERVER_BASE "http://goto.local:8080"
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
  if (!cred->password.empty()) {
    WiFi.begin(cred->ssid.c_str(), cred->password.c_str());
  } else {
    WiFi.begin(cred->ssid.c_str());
  }

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

// Parse the tiny current.json manifest into the two fields the device needs.
bool parseManifest(const char* json, std::string& editionId, std::string& editionPath) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return false;
  editionId = doc["editionId"] | "";
  editionPath = doc["editionPath"] | "";
  return !editionId.empty() && !editionPath.empty();
}

// Load the edition currently recorded in the SD cache manifest. Returns true and
// fills out/editionId on success.
bool loadFromCache(GotoEdition& out, std::string& editionId) {
  if (!Storage.exists(kCacheManifestPath)) return false;
  const String manifest = Storage.readFile(kCacheManifestPath);
  std::string cachedPath;
  if (manifest.length() == 0 || !parseManifest(manifest.c_str(), editionId, cachedPath)) return false;
  const String edition = Storage.readFile(cacheEditionPath(editionId).c_str());
  return edition.length() > 0 && parseGotoEdition(edition.c_str(), out);
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
      if (parseManifest(manifestJson.c_str(), editionId, editionPath)) {
        const std::string localPath = cacheEditionPath(editionId);

        // Download only a newer edition; an already-cached editionId is reused.
        if (!Storage.exists(localPath.c_str())) {
          std::string editionJson;
          if (HttpDownloader::fetchUrl(std::string(kServerBase) + "/" + editionPath, editionJson)) {
            GotoEdition fresh;
            if (parseGotoEdition(editionJson.c_str(), fresh)) {
              // Cache the edition first, then the manifest, so the manifest never
              // points at an edition file that is not on disk.
              Storage.ensureDirectoryExists(kCacheDir);
              Storage.ensureDirectoryExists(kCacheEditionsDir);
              if (Storage.writeFile(localPath.c_str(), String(editionJson.c_str()))) {
                Storage.writeFile(kCacheManifestPath, String(manifestJson.c_str()));
              }
              out = std::move(fresh);
              result.origin = GotoEditionOrigin::Network;
              result.editionId = editionId;
              LOG_INF("GOTO", "loaded edition %s from network", editionId.c_str());
              return result;
            }
            LOG_ERR("GOTO", "downloaded edition failed to parse; falling back");
          } else {
            LOG_ERR("GOTO", "edition download failed; falling back");
          }
        } else {
          // Server's current edition is already cached: refresh the manifest
          // pointer and serve it from cache (no re-download).
          Storage.writeFile(kCacheManifestPath, String(manifestJson.c_str()));
          const String cached = Storage.readFile(localPath.c_str());
          if (cached.length() > 0 && parseGotoEdition(cached.c_str(), out)) {
            // Live-verified: manifest fetched this session confirms this is the
            // current edition. Served from cache bytes, but not stale -> no marker.
            result.origin = GotoEditionOrigin::CacheCurrent;
            result.editionId = editionId;
            LOG_INF("GOTO", "edition %s confirmed current; serving cache (live)", editionId.c_str());
            return result;
          }
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
