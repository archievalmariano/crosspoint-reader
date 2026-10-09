#pragma once

#include <cstddef>
#include <string>

// Defensive bounds for UNTRUSTED remote DOGEAR input (current.json + issue JSON),
// copied from GOTO's pattern (goto_limits) rather than shared with it. A weekly
// issue is ~5 KB with 3-8 items; every cap below leaves several times that, and
// an issue over any cap is rejected, never truncated.
namespace dogear_limits {

// Cumulative download caps, enforced on bytes actually received.
inline constexpr size_t kMaxManifestBytes = 4 * 1024;
inline constexpr size_t kMaxIssueBytes = 32 * 1024;

// Issue schemas this firmware reads: 2 (a regular week) and 3 (a quiet week:
// v2 plus the cover's quiet heading and note). Older firmware reads only 2, so
// it rejects a quiet issue and keeps its verified cache.
inline constexpr size_t kIssueSchemaMin = 2;
inline constexpr size_t kIssueSchemaMax = 3;

// A quiet issue's contract, as the publisher writes it (dogear/quiet.py and
// dogear/select.py): 3-5 items, exactly one featured and first, every other item
// "standard", and exactly the approved cover copy. Anything else is not a quiet
// issue this firmware reads.
inline constexpr size_t kQuietMinEntries = 3;
inline constexpr size_t kQuietMaxEntries = 5;
inline constexpr size_t kQuietFeatured = 1;
inline constexpr char kQuietHeading[] = "SOME QUIET THIS WEEK";
inline constexpr char kQuietNote[] = "Not much on our calendar, but still a few things worth a dogear for you.";

// Structure. The selector publishes at most 8 items.
inline constexpr size_t kMaxEntries = 12;

// Field lengths.
inline constexpr size_t kMaxIssueIdLen = 17;  // "dogear-YYYY-MM-DD"
inline constexpr size_t kMaxIssuePathLen = 128;
inline constexpr size_t kMaxShaLen = 64;
inline constexpr size_t kMaxShortLen = 64;  // label, datelines, kickers, CTA label
inline constexpr size_t kMaxLineLen = 160;  // strapline, next issue, titles, notes
inline constexpr size_t kMaxHeadlineLen = 240;
inline constexpr size_t kMaxBodyLen = 2048;  // 40-70 words, or a rare two-screen item
inline constexpr size_t kMaxUrlLen = 2953;   // == QR byte capacity (ECC_LOW, v40)

// An issue id becomes an SD path (/dogear/issues/<id>.json), so it must be
// exactly "dogear-" + an ISO date: no '/', '.', or anything path-shaped.
inline bool isSafeIssueId(const std::string& id) {
  static constexpr char kPrefix[] = "dogear-";
  if (id.size() != kMaxIssueIdLen || id.rfind(kPrefix, 0) != 0) return false;
  for (size_t i = sizeof(kPrefix) - 1; i < id.size(); ++i) {
    const size_t k = i - (sizeof(kPrefix) - 1);  // position within YYYY-MM-DD
    const char c = id[i];
    if (k == 4 || k == 7) {
      if (c != '-') return false;
    } else if (c < '0' || c > '9') {
      return false;
    }
  }
  return true;
}

// A link the device may render as a QR: non-empty http(s), within QR capacity.
inline bool isQrUrl(const std::string& url) {
  if (url.empty() || url.size() > kMaxUrlLen) return false;
  return url.rfind("https://", 0) == 0 || url.rfind("http://", 0) == 0;
}

}  // namespace dogear_limits
