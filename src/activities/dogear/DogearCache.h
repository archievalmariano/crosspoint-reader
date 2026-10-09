#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "DogearLimits.h"

// DOGEAR's SD cache: two verified generations that survive interruption
// (pure logic over a small file interface, so it is host-testable with fault
// injection).
//
//   /dogear/issues/<issueId>.<sha16>.json   issue bytes, named by content
//   /dogear/active.txt                      "v1 <issueId> <sha256>"
//   /dogear/previous.txt                    the generation before it
//
// A generation is accepted only if its manifest parses, its issue file exists
// within the size cap, and the file's bytes hash to the manifest's SHA-256.
// Commit order: a verified temp manifest left by an interrupted commit is made
// active first; then issue to a temp file, read back and verify, rename into
// place; manifest to a temp file, read back and verify; then previous <- active
// and active <- temp. Whatever step an interruption hits, the last good
// generation (active, the temp manifest, or previous during the swap) still
// loads, including across repeated interrupted commits.
namespace dogear_cache {

// SD operations the cache needs. write() creates or truncates; rename() fails
// if the target exists (as SdFat's does).
class FileOps {
 public:
  virtual ~FileOps() = default;
  virtual bool exists(const std::string& path) = 0;
  // Reads the whole file if it is at most maxBytes; false if missing or larger.
  virtual bool read(const std::string& path, std::string& out, size_t maxBytes) = 0;
  virtual bool write(const std::string& path, const std::string& data) = 0;
  virtual bool rename(const std::string& from, const std::string& to) = 0;
  virtual bool remove(const std::string& path) = 0;
  virtual bool list(const std::string& dir, std::vector<std::string>& names) = 0;
  virtual bool ensureDir(const std::string& path) = 0;
};

// Lowercase hex SHA-256 (mbedtls on the device, a test implementation on host).
using Sha256Hex = std::string (*)(const std::string& data);

struct Generation {
  std::string issueId;
  std::string sha256;
  std::string bytes;  // verified against sha256
};

inline constexpr char kDir[] = "/dogear";
inline constexpr char kIssuesDir[] = "/dogear/issues";
inline constexpr char kActive[] = "/dogear/active.txt";
inline constexpr char kPrevious[] = "/dogear/previous.txt";
inline constexpr char kActiveTmp[] = "/dogear/active.txt.tmp";
inline constexpr size_t kMaxManifestFile = 128;

inline bool isLowerHex64(const std::string& s) {
  if (s.size() != 64) return false;
  for (const char c : s)
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  return true;
}

inline std::string manifestText(const std::string& issueId, const std::string& sha) {
  return "v1 " + issueId + " " + sha + "\n";
}

// "v1 <issueId> <sha256>\n", strictly.
inline bool parseManifest(const std::string& text, std::string& issueId, std::string& sha) {
  if (text.size() < 4 || text.rfind("v1 ", 0) != 0 || text.back() != '\n') return false;
  const size_t space = text.find(' ', 3);
  if (space == std::string::npos) return false;
  issueId = text.substr(3, space - 3);
  sha = text.substr(space + 1, text.size() - space - 2);
  return dogear_limits::isSafeIssueId(issueId) && isLowerHex64(sha);
}

inline std::string issuePath(const std::string& issueId, const std::string& sha) {
  return std::string(kIssuesDir) + "/" + issueId + "." + sha.substr(0, 16) + ".json";
}

class Cache {
 public:
  Cache(FileOps& fs, Sha256Hex sha256) : fs(fs), sha256(sha256) {}

  // A verified generation from one manifest. Fails closed: wrong size, wrong
  // hash or a malformed manifest all reject it.
  bool loadFrom(const char* manifestPath, Generation& out) {
    std::string text, id, sha;
    if (!fs.read(manifestPath, text, kMaxManifestFile) || !parseManifest(text, id, sha)) return false;
    std::string bytes;
    if (!fs.read(issuePath(id, sha), bytes, dogear_limits::kMaxIssueBytes) || bytes.empty()) return false;
    if (sha256(bytes) != sha) return false;
    out = Generation{id, sha, std::move(bytes)};
    return true;
  }

  // The newest verified generation: active; else a fully written replacement
  // that was cut off before it became active; else previous.
  bool loadBest(Generation& out, bool* fromPrevious = nullptr) {
    if (fromPrevious) *fromPrevious = false;
    if (loadFrom(kActive, out) || loadFrom(kActiveTmp, out)) return true;
    if (loadFrom(kPrevious, out)) {
      if (fromPrevious) *fromPrevious = true;
      return true;
    }
    return false;
  }

  // Make (issueId, bytes) the active generation. `sha` must already be the
  // verified hash of `bytes`. Returns false, leaving the last good generation
  // loadable, if any step fails.
  bool commit(const std::string& issueId, const std::string& sha, const std::string& bytes) {
    if (!dogear_limits::isSafeIssueId(issueId) || !isLowerHex64(sha) || bytes.empty() ||
        bytes.size() > dogear_limits::kMaxIssueBytes || sha256(bytes) != sha) {
      return false;
    }
    if (!fs.ensureDir(kDir) || !fs.ensureDir(kIssuesDir)) return false;
    if (!promotePending()) return false;

    // 1. The issue file, by content. An existing copy is reused only if it verifies.
    const std::string path = issuePath(issueId, sha);
    if (!fileHolds(path, sha)) {
      const std::string tmp = path + ".tmp";
      fs.remove(tmp);
      if (!fs.write(tmp, bytes) || !fileHolds(tmp, sha)) return false;
      if (fs.exists(path) && !fs.remove(path)) return false;  // a corrupt copy under the final name
      if (!fs.rename(tmp, path) || !fileHolds(path, sha)) return false;
    }

    // 2. The new manifest, verified before it can replace anything.
    const std::string manifest = manifestText(issueId, sha);
    fs.remove(kActiveTmp);
    std::string check;
    if (!fs.write(kActiveTmp, manifest) || !fs.read(kActiveTmp, check, kMaxManifestFile) || check != manifest) {
      return false;
    }

    // 3. Swap. The active generation becomes previous, so a cut between the
    // two renames leaves previous (and the verified temp manifest) loadable.
    Generation current;
    const bool haveActive = loadFrom(kActive, current);
    if (haveActive && current.sha256 == sha) {  // already active
      fs.remove(kActiveTmp);
      cleanup();
      return true;
    }
    if (haveActive) {
      fs.remove(kPrevious);
      if (!fs.rename(kActive, kPrevious)) return false;
    } else if (fs.exists(kActive) && !fs.remove(kActive)) {
      return false;
    }
    if (!fs.rename(kActiveTmp, kActive)) return false;

    // 4. Tidy up. Failure here never affects what loads.
    cleanup();
    return true;
  }

  // Delete issue files neither manifest names, stray temp files, and the
  // first prototype's layout (current.json + issues/<id>.json).
  void cleanup() {
    std::vector<std::string> keep;
    std::string text, id, sha;
    for (const char* m : {kActive, kActiveTmp, kPrevious}) {
      if (fs.read(m, text, kMaxManifestFile) && parseManifest(text, id, sha)) keep.push_back(issuePath(id, sha));
    }
    std::vector<std::string> names;
    if (fs.list(kIssuesDir, names)) {
      for (const std::string& name : names) {
        const std::string path = std::string(kIssuesDir) + "/" + name;
        bool kept = false;
        for (const std::string& k : keep) kept = kept || k == path;
        if (!kept) fs.remove(path);
      }
    }
    fs.remove(std::string(kDir) + "/current.json");
  }

 private:
  FileOps& fs;
  Sha256Hex sha256;

  // A verified manifest left in kActiveTmp with no loadable active (a commit
  // cut off before its final rename) is the generation loadBest() serves.
  // Make it active before kActiveTmp is reused; if that fails, kActiveTmp is
  // left untouched and the commit stops.
  bool promotePending() {
    {
      Generation g;
      if (loadFrom(kActive, g) || !loadFrom(kActiveTmp, g)) return true;
    }
    if (fs.exists(kActive) && !fs.remove(kActive)) return false;
    return fs.rename(kActiveTmp, kActive);
  }

  bool fileHolds(const std::string& path, const std::string& sha) {
    std::string bytes;
    return fs.read(path, bytes, dogear_limits::kMaxIssueBytes) && !bytes.empty() && sha256(bytes) == sha;
  }
};

// What became of a downloaded, hash-verified issue.
enum class Admitted { Unreadable, Cached, NotCached };

// A downloaded issue becomes a cache generation only if this firmware can read it
// (`accept`); an unreadable one is never written, so the readable generations stay
// as they were and the device keeps showing the newest of them.
template <class Accept>
Admitted admitDownload(Cache& cache, const Generation& fresh, Accept accept) {
  if (!accept(fresh)) return Admitted::Unreadable;
  return cache.commit(fresh.issueId, fresh.sha256, fresh.bytes) ? Admitted::Cached : Admitted::NotCached;
}

// The newest generation that verifies AND that this firmware can read: active, an
// interrupted replacement, then previous. A generation `accept` rejects (an issue
// schema newer than this firmware reads, say) is passed over, never deleted, so a
// device on older firmware keeps showing its last readable issue. Returns the
// manifest the generation came from, or nullptr.
template <class Accept>
const char* loadNewestReadable(Cache& cache, Generation& out, Accept accept) {
  for (const char* manifest : {kActive, kActiveTmp, kPrevious}) {
    Generation g;
    if (cache.loadFrom(manifest, g) && accept(g)) {
      out = std::move(g);
      return manifest;
    }
  }
  return nullptr;
}

}  // namespace dogear_cache
