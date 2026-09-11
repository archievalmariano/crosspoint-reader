#include "GotoEditionSource.h"

#include <ArduinoJson.h>
#include <HalStorage.h>
#include <Logging.h>
#include <WiFi.h>

#include <string>

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

bool wifiUp() {
  return WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0);
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

  // 1) Network path — only when Wi-Fi is already provisioned/connected. The
  //    reader open path never launches interactive Wi-Fi selection; provisioning
  //    stays in the existing CrossPoint flow so GOTO opens offline-first.
  if (wifiUp()) {
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
            result.origin = GotoEditionOrigin::Cache;
            result.editionId = editionId;
            LOG_INF("GOTO", "edition %s already cached; serving cache", editionId.c_str());
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

  // 2) Offline cache.
  std::string cachedId;
  if (loadFromCache(out, cachedId)) {
    result.origin = GotoEditionOrigin::Cache;
    result.editionId = cachedId;
    LOG_INF("GOTO", "loaded edition %s from SD cache (offline)", cachedId.c_str());
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
