#pragma once

#include "GotoEdition.h"
#include "activities/Activity.h"

// GOTO morning edition reader: a finite, glanceable newspaper. One story per
// 800x480 page, showing masthead, section, headline, 1-2 source excerpt
// paragraphs (deterministic fit/fallback, never scrolls), source + time, page
// number, and a visual-only "FULL STORY" affordance. Physical page buttons move
// between stories (clamped at both ends); Back returns Home; re-entering resets
// to page 1. E0.5: static compiled-in fixture, built-in fonts only, no network.
class GotoActivity final : public Activity {
  GotoEdition edition;
  int pageIndex = 0;
  bool loaded = false;

  void drawStoryPage(const GotoStory& story);
  void drawFullStoryAffordance(int rightEdge, int rowTop);

 public:
  GotoActivity(GfxRenderer& renderer, MappedInputManager& mappedInput) : Activity("Goto", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
};
