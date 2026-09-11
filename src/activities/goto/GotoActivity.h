#pragma once

#include "GotoEdition.h"
#include "activities/Activity.h"

// GOTO morning edition reader: a finite, glanceable newspaper. One story per
// 800x480 page; physical page buttons move between stories (clamped at both
// ends); Back returns Home. Re-entering resets to page 1. E0: static
// compiled-in fixture, built-in fonts only, no scrolling, no network.
class GotoActivity final : public Activity {
  GotoEdition edition;
  int pageIndex = 0;
  bool loaded = false;

  void drawStoryPage(const GotoStory& story);

 public:
  GotoActivity(GfxRenderer& renderer, MappedInputManager& mappedInput) : Activity("Goto", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
};
