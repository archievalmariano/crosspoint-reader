#pragma once

#include <cstdint>

// Pure DOGEAR page model (no device deps, host-testable).
//
// An issue reads as a short printed digest:
//   page 1         cover / contents
//   pages 2..N-1   one item each; an item the editor allowed a second screen may
//                  take two screens, which share that item's page number
//   page N         EVERYTHING IS DOGEARED HERE (QR to the full web edition)
// so N = items + 2 whatever the screen count. Reading QRs are secondary views
// opened from an item and are not in this sequence.
//
// Navigation is circular over the screens, like GOTO: the final page's Next
// returns to the cover.
namespace dogear_pages {

enum class Kind : uint8_t { Cover, Item, Final };

struct Screen {
  Kind kind = Kind::Cover;
  uint8_t entry = 0;   // item index (Kind::Item only)
  uint8_t part = 0;    // 0 = the item's first screen, 1 = its continuation
  uint8_t parts = 1;   // screens this item takes (1 or 2)
  uint8_t number = 1;  // the printed page number, 1-based
};

inline constexpr int kMaxScreens = 32;

// Printed page count: cover + one per item + final.
inline int pageCount(int entries) { return entries + 2; }

// Fills out[] from each item's screen count (clamped to 1..2) and returns the
// number of screens, or 0 when they would not fit in `cap`.
inline int build(const uint8_t* partsPerEntry, int entries, Screen* out, int cap) {
  int n = 0;
  auto push = [&](const Screen& s) {
    if (n >= cap) return false;
    out[n++] = s;
    return true;
  };
  Screen cover;
  if (!push(cover)) return 0;
  for (int i = 0; i < entries; ++i) {
    const uint8_t parts = partsPerEntry[i] >= 2 ? 2 : 1;
    for (uint8_t p = 0; p < parts; ++p) {
      Screen s;
      s.kind = Kind::Item;
      s.entry = static_cast<uint8_t>(i);
      s.part = p;
      s.parts = parts;
      s.number = static_cast<uint8_t>(i + 2);
      if (!push(s)) return 0;
    }
  }
  Screen fin;
  fin.kind = Kind::Final;
  fin.number = static_cast<uint8_t>(pageCount(entries));
  if (!push(fin)) return 0;
  return n;
}

inline int nextIndex(int index, int screens) { return screens > 0 ? (index + 1) % screens : 0; }
inline int prevIndex(int index, int screens) { return screens > 0 ? (index - 1 + screens) % screens : 0; }

// The action label shows on an item's last screen only (the continuation of a
// two-screen item carries it, not the first half).
inline bool showsAction(const Screen& s) { return s.kind == Kind::Item && s.part + 1 == s.parts; }

// A button release that counts only if this screen saw its press (GOTO's rule):
// the Wi-Fi picker DOGEAR opens over itself acts on the press, so that press's
// release must not also act here. A press and release in one frame counts.
struct PressGate {
  bool pressSeen = false;

  void reset() { pressSeen = false; }

  bool update(bool pressed, bool released) {
    if (pressed) pressSeen = true;
    if (!released) return false;
    const bool counts = pressSeen;
    pressSeen = false;
    return counts;
  }
};

}  // namespace dogear_pages
