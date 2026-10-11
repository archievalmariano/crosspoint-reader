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

// The touch target of the footer's « BACK label: from the screen's left edge to
// past the label's end, and from the footer rule to the bottom edge, so a
// fingertip anywhere on it counts. It never reaches the footer's action (BACK
// screens carry none). Touch boards have no front Back button; this is their Back.
struct TapRect {
  int x;
  int y;
  int w;
  int h;
};

inline TapRect backTapRect(int margin, int labelWidth, int ruleTop, int screenHeight) {
  const int pad = 24;
  return TapRect{0, ruleTop, margin + labelWidth + pad, screenHeight - ruleTop};
}

// Which DOGEAR view is showing: a status screen (no readable issue), the QR
// view, or the paged issue (its footer shows a page number, never BACK).
enum class BackView { Status, Qr, Paged };
enum class BackAction { None, CloseQr, Exit };

// The « BACK tap target, tagged with the view whose footer drew it. The render
// task installs it; the loop task routes taps against it. A tap counts only
// while the view that drew the target is the view showing, so a target left
// over from a screen that has gone (or drawn late by a render already under
// way when the screen changed) can never act on the next one.
struct BackTarget {
  bool valid = false;
  BackView drawnFor = BackView::Paged;
  TapRect rect{0, 0, 0, 0};

  void install(BackView view, TapRect r) {
    rect = r;
    drawnFor = view;
    valid = view != BackView::Paged;
  }
  void clear() { valid = false; }

  bool hits(BackView showing, int x, int y) const {
    return valid && drawnFor == showing && showing != BackView::Paged && x >= rect.x && x < rect.x + rect.w &&
           y >= rect.y && y < rect.y + rect.h;
  }
};

// One input frame's Back decision. On the QR view, Back, a tap or (on touch
// boards) a side key closes it, and the target is cleared here, synchronously,
// before the caller requests the redraw: until the destination screen draws its
// own footer, no tap can reach DOGEAR's Back. Elsewhere, the Back key (or the
// back swipe, folded into it) exits, and so does a tap on the « BACK that the
// showing screen drew.
inline BackAction routeBack(BackView showing, bool backKey, bool tapped, int x, int y, BackTarget& target,
                            bool otherQrClose = false) {
  if (showing == BackView::Qr) {
    if (!(backKey || tapped || otherQrClose)) return BackAction::None;
    target.clear();
    return BackAction::CloseQr;
  }
  if (backKey || (tapped && target.hits(showing, x, y))) return BackAction::Exit;
  return BackAction::None;
}

}  // namespace dogear_pages
