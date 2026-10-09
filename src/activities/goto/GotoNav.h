#pragma once

// Pure GOTO/TOGO page-ring navigation (no device deps, host-testable).
//
// The ring is the story pages (indices 0 .. storyCount-1) PLUS one terminal
// edition page at index == storyCount, which sits OUTSIDE story pagination (it is
// never shown in the n/N pager). Navigation is circular over the whole ring:
//   last story (storyCount-1) --Next--> terminal (storyCount)
//   terminal --Previous--> last story
//   terminal --Next--> story 1 (index 0)
namespace goto_nav {

// Number of navigable pages: the stories plus the one terminal edition page.
inline int ringSize(int storyCount) { return storyCount + 1; }

// Index of the terminal edition page (one past the last story).
inline int terminalIndex(int storyCount) { return storyCount; }

// True when `index` is the terminal edition page (not a story).
inline bool isTerminal(int index, int storyCount) { return index == storyCount; }

// Next / previous index in the circular ring (stories + terminal page).
inline int nextIndex(int index, int storyCount) {
  const int r = ringSize(storyCount);
  return (index + 1) % r;
}
inline int prevIndex(int index, int storyCount) {
  const int r = ringSize(storyCount);
  return (index - 1 + r) % r;
}

// A button release that counts only if this screen also saw its press. Screens
// GOTO opens over itself (the Wi-Fi picker) act on the press, so the release of
// that same press must not also act in GOTO. A press and release reported in
// one frame (the touch back gesture) counts.
struct PressGate {
  bool pressSeen = false;

  void reset() { pressSeen = false; }

  // Feed one frame's edges; true when this frame's release completes a press
  // seen here.
  bool update(bool pressed, bool released) {
    if (pressed) pressSeen = true;
    if (!released) return false;
    const bool counts = pressSeen;
    pressSeen = false;
    return counts;
  }
};

}  // namespace goto_nav
