#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

// Pure Apps-launcher ordering (no device deps, host-testable).
//
// Custom firmware apps live behind the single Home "Apps" entry. Each app has a
// stable internal sort key that is independent of its rendered label, so an app
// whose label changes at runtime (GOTO <-> TOGO) never moves in the list. Keys
// are ASCII, uppercase by convention, and never localized.

// Stable app identity. `None` means "no app to focus" (e.g. Home -> Apps).
// Values are never reordered: they are also the sort tie-breaker.
enum class AppId : uint8_t { None, Goto, OnPoint, Gate };

namespace app_registry {

struct AppKey {
  AppId id;
  const char* sortKey;
};

// ASCII case-insensitive strcmp. Non-ASCII bytes compare by raw value.
inline int compareSortKeys(const char* a, const char* b) {
  auto fold = [](unsigned char c) -> unsigned char { return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c; };
  while (*a && fold(*a) == fold(*b)) {
    ++a;
    ++b;
  }
  return static_cast<int>(fold(*a)) - static_cast<int>(fold(*b));
}

// Strict weak ordering: sort key first, AppId as a deterministic tie-break.
inline bool keyLess(const AppKey& a, const AppKey& b) {
  const int c = compareSortKeys(a.sortKey, b.sortKey);
  if (c != 0) return c < 0;
  return static_cast<uint8_t>(a.id) < static_cast<uint8_t>(b.id);
}

// Fills out[0..n) with indices into keys[], ordered by keyLess. Insertion sort:
// n is a handful of compile-time entries, so no allocation and no <algorithm>.
// n must be <= 256 (indices are uint8_t).
inline void sortedOrder(const AppKey* keys, size_t n, uint8_t* out) {
  for (size_t i = 0; i < n; ++i) {
    const auto idx = static_cast<uint8_t>(i);
    size_t j = i;
    while (j > 0 && keyLess(keys[idx], keys[out[j - 1]])) {
      out[j] = out[j - 1];
      --j;
    }
    out[j] = idx;
  }
}

// Row (position in the sorted order) of the app `id`, or 0 when that app is not
// in this build (or id is None) so focus always lands on a valid row.
inline int rowForApp(const AppKey* keys, const uint8_t* order, size_t n, AppId id) {
  for (size_t row = 0; row < n; ++row) {
    if (keys[order[row]].id == id) return static_cast<int>(row);
  }
  return 0;
}

// Which activity a launcher transition installed.
enum class LaunchOutcome : uint8_t { Primary, Fallback, None };

// Allocates the primary activity; only if that fails, allocates the fallback.
// Both factories must be non-throwing (return null on OOM). Returns the first
// successful allocation, or null when both fail -- the caller then leaves the
// current activity running rather than tearing it down.
template <typename Base, typename MakePrimary, typename MakeFallback>
std::unique_ptr<Base> launchWithFallback(MakePrimary&& makePrimary, MakeFallback&& makeFallback,
                                         LaunchOutcome& outcome) {
  std::unique_ptr<Base> activity = makePrimary();
  if (activity) {
    outcome = LaunchOutcome::Primary;
    return activity;
  }
  activity = makeFallback();
  outcome = activity ? LaunchOutcome::Fallback : LaunchOutcome::None;
  return activity;
}

}  // namespace app_registry
