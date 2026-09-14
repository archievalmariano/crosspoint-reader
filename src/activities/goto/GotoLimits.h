#pragma once

#include <cstddef>
#include <string>

// Defensive bounds for UNTRUSTED remote GOTO input (current.json + edition JSON).
// The device has a ~380 KB RAM ceiling; malformed/oversized input must be rejected
// well before it can exhaust the heap, and must never build an unsafe SD path. All
// values are comfortably above realistic production payloads (a current.json is
// ~0.6 KB; a full 10-story edition is ~7 KB), not barely above today's examples.
namespace goto_limits {

// Cumulative download caps (bytes). Enforced on ACTUAL received bytes, so they
// hold whether the server declares Content-Length, uses chunked transfer, or
// lies. 8 KB manifest (~13x headroom) / 48 KB edition (~7x headroom) both leave
// the string + JSON document well within the C3 heap.
inline constexpr size_t kMaxManifestBytes = 8 * 1024;
inline constexpr size_t kMaxEditionBytes = 48 * 1024;

// Structural ceilings (product ships 5-10 stories; ~2x headroom). The story cap
// bounds the per-story vector allocation, the main OOM vector.
inline constexpr size_t kMaxStories = 20;
inline constexpr size_t kMaxParagraphs = 8;

// Field-length caps. Beyond these an edition is treated as malformed and rejected
// (never truncated), so an identifier/URL can't be silently changed in meaning.
inline constexpr size_t kMaxEditionIdLen = 40;
inline constexpr size_t kMaxEditionPathLen = 128;
inline constexpr size_t kMaxShaLen = 64;
inline constexpr size_t kMaxLabelLen = 16;
inline constexpr size_t kMaxDatelineLen = 128;
inline constexpr size_t kMaxSectionLen = 64;
inline constexpr size_t kMaxHeadlineLen = 512;
inline constexpr size_t kMaxParagraphLen = 4096;
inline constexpr size_t kMaxSourceLen = 64;
inline constexpr size_t kMaxTimeLen = 64;
inline constexpr size_t kMaxUrlLen = 2953;  // == QR byte capacity (ECC_LOW, v40)

// A remote editionId is concatenated into an SD cache path
// (/goto/editions/<editionId>.json), so it must be a safe filename: a
// "goto-"/"togo-" prefix, only lowercase [a-z0-9-], bounded length, and therefore
// no '/', '.', '..' or other traversal/escape characters. Valid production IDs
// (goto-2026-09-14, togo-2026-09-14) pass; anything path-shaped is rejected.
inline bool isSafeEditionId(const std::string& id) {
  if (id.size() < 6 || id.size() > kMaxEditionIdLen) return false;
  if (id.rfind("goto-", 0) != 0 && id.rfind("togo-", 0) != 0) return false;
  for (const char c : id) {
    const bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
    if (!ok) return false;
  }
  return true;
}

}  // namespace goto_limits
