// Compiled only into builds that enable DOGEAR (see [dogear] in platformio.ini).
#ifdef DOGEAR_ENABLED

#include "DogearSource.h"

#include <ArduinoJson.h>
#include <HalStorage.h>
#include <Logging.h>
#include <WiFi.h>
#include <mbedtls/sha256.h>

#include <cstdio>
#include <string>
#include <vector>

#include "DogearCache.h"
#include "DogearHttps.h"
#include "DogearLimits.h"

// Base URL of the DOGEAR issue server. Production is the hosted edition (HTTPS,
// verified). A dev build may point at a LAN server over plain HTTP, but only if
// it also defines DOGEAR_ALLOW_LAN_HTTP; neither belongs in a committed file:
//   PLATFORMIO_BUILD_FLAGS='-DDOGEAR_SERVER_BASE=\"http://192.168.1.50:8080\" -DDOGEAR_ALLOW_LAN_HTTP'
#ifndef DOGEAR_SERVER_BASE
#define DOGEAR_SERVER_BASE "https://dogear.archievalmariano.com"
#endif

namespace {
constexpr char kServerBase[] = DOGEAR_SERVER_BASE;

bool wifiUp() { return WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0); }

std::string sha256Hex(const std::string& data) {
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

// The cache's file operations on the SD card, through HalStorage. Reads are
// bounded by the caller's cap (checked against the file size before reading),
// not by the generic 50 KB readFile limit.
class SdFiles final : public dogear_cache::FileOps {
 public:
  bool exists(const std::string& path) override { return Storage.exists(path.c_str()); }

  bool read(const std::string& path, std::string& out, size_t maxBytes) override {
    out.clear();
    HalFile f;
    if (!Storage.exists(path.c_str()) || !Storage.openFileForRead("DOGEAR", path, f)) return false;
    const size_t size = f.fileSize();
    if (size > maxBytes) return false;
    out.resize(size);
    size_t got = 0;
    while (got < size) {
      const int n = f.read(&out[got], size - got);
      if (n <= 0) return false;
      got += static_cast<size_t>(n);
    }
    return true;
  }

  bool write(const std::string& path, const std::string& data) override {
    HalFile f;
    if (!Storage.openFileForWrite("DOGEAR", path, f)) return false;
    const bool ok = f.write(data.data(), data.size()) == data.size();
    return f.close() && ok;  // closed before any rename of this path
  }

  bool rename(const std::string& from, const std::string& to) override {
    return Storage.rename(from.c_str(), to.c_str());
  }
  bool remove(const std::string& path) override {
    return !Storage.exists(path.c_str()) || Storage.remove(path.c_str());
  }
  bool ensureDir(const std::string& path) override { return Storage.ensureDirectoryExists(path.c_str()); }

  bool list(const std::string& dir, std::vector<std::string>& names) override {
    names.clear();
    HalFile d = Storage.open(dir.c_str());
    if (!d || !d.isDirectory()) return false;
    char name[96];
    for (HalFile f = d.openNextFile(); f; f = d.openNextFile()) {
      if (!f.isDirectory() && f.getName(name, sizeof(name)) > 0) names.emplace_back(name);
    }
    return true;
  }
};

struct Manifest {
  std::string issueId;
  std::string issuePath;
  std::string issueSha;
};

// The server's current.json: all three fields required and checked (the id
// names a cache file; the hash is the issue's identity).
bool parseServerManifest(const std::string& json, Manifest& m) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return false;
  m.issueId = doc["issueId"] | "";
  m.issuePath = doc["issuePath"] | "";
  m.issueSha = doc["issueSha256"] | "";
  if (!dogear_limits::isSafeIssueId(m.issueId) || !dogear_cache::isLowerHex64(m.issueSha)) return false;
  if (m.issuePath.empty() || m.issuePath.size() > dogear_limits::kMaxIssuePathLen) return false;
  for (const char c : m.issuePath) {
    const bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '/' || c == '.' || c == '_';
    if (!ok) return false;
  }
  return m.issuePath.find("..") == std::string::npos && m.issuePath[0] != '/';
}

bool parseGeneration(const dogear_cache::Generation& g, DogearIssue& out) {
  return parseVerifiedIssue(g.bytes, g.issueId, out);
}

// Network path. True when `out` holds an issue confirmed current this session.
bool loadFromNetwork(dogear_cache::Cache& cache, DogearIssue& out, DogearOrigin& origin) {
  std::string manifestJson;
  const auto got = dogear_https::get(std::string(kServerBase) + "/current.json", manifestJson,
                                     dogear_limits::kMaxManifestBytes, dogear_https::Fetch::Manifest);
  if (got != dogear_https::Result::Ok) {
    LOG_ERR("DOGEAR", "current.json: %s", dogear_https::describe(got));
    return false;
  }
  Manifest m;
  if (!parseServerManifest(manifestJson, m)) {
    LOG_ERR("DOGEAR", "current.json invalid");
    return false;
  }

  // Unchanged: the active generation is reused only if its bytes still hash to
  // the server's SHA-256 and parse as that issue.
  dogear_cache::Generation active;
  if (cache.loadFrom(dogear_cache::kActive, active) && active.issueId == m.issueId && active.sha256 == m.issueSha &&
      parseGeneration(active, out)) {
    origin = DogearOrigin::CacheCurrent;
    LOG_INF("DOGEAR", "issue %s unchanged; serving the verified SD copy", m.issueId.c_str());
    return true;
  }

  std::string issueJson;
  const auto fetched =
      dogear_https::get(std::string(kServerBase) + "/" + m.issuePath, issueJson, dogear_limits::kMaxIssueBytes);
  if (fetched != dogear_https::Result::Ok) {
    LOG_ERR("DOGEAR", "issue download: %s", dogear_https::describe(fetched));
    return false;
  }
  if (sha256Hex(issueJson) != m.issueSha) {
    LOG_ERR("DOGEAR", "issue hash mismatch; keeping the SD copy");
    return false;
  }
  const dogear_cache::Generation fresh{m.issueId, m.issueSha, issueJson};
  DogearIssue issue;
  const auto admitted = dogear_cache::admitDownload(
      cache, fresh, [&issue](const dogear_cache::Generation& g) { return parseGeneration(g, issue); });
  if (admitted == dogear_cache::Admitted::Unreadable) {
    LOG_ERR("DOGEAR", "downloaded issue invalid; keeping the SD copy");
    return false;
  }
  const bool cached = admitted == dogear_cache::Admitted::Cached;
  if (!cached) LOG_ERR("DOGEAR", "cache commit failed; the previous SD copy stays active");
  out = std::move(issue);
  origin = DogearOrigin::Network;
  LOG_INF("DOGEAR", "downloaded issue %s (hash verified)%s", m.issueId.c_str(), cached ? "; cached" : "");
  return true;
}
}  // namespace

bool dogearWifiConnected() { return wifiUp(); }

DogearOrigin loadCurrentDogearIssue(DogearIssue& out) {
  SdFiles files;
  dogear_cache::Cache cache(files, sha256Hex);
  DogearOrigin origin = DogearOrigin::None;
  if (wifiUp() && loadFromNetwork(cache, out, origin)) return origin;

  // Offline (or the server failed): the newest generation that verifies and
  // parses: active, an interrupted replacement, then previous.
  dogear_cache::Generation g;
  const char* manifest = dogear_cache::loadNewestReadable(
      cache, g, [&out](const dogear_cache::Generation& gen) { return parseGeneration(gen, out); });
  if (manifest != nullptr) {
    LOG_INF("DOGEAR", "issue %s from SD (%s, verified)", g.issueId.c_str(), manifest);
    return DogearOrigin::CacheStale;
  }
  LOG_INF("DOGEAR", "no readable issue (no network issue, no verified cache)");
  return DogearOrigin::None;
}

#endif  // DOGEAR_ENABLED
