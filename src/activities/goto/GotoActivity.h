#pragma once

#include <string>
#include <vector>

#include "GotoEdition.h"
#include "GotoEditionSource.h"
#include "GotoNav.h"
#include "activities/Activity.h"

// GOTO morning edition reader: a finite, glanceable newspaper. One story per
// 800x480 page, showing masthead, section, headline, the first source excerpt
// paragraph (never scrolls), source + time, page number, and a visual-only
// "FULL STORY" affordance. Physical page buttons cycle between stories
// (circular, both directions); Back returns Home; re-entering resets to page 1.
//
// E0.6: the page geometry is deliberately theme-INDEPENDENT — GOTO draws its own
// masthead/rules with fixed layout constants and built-in font IDs, and uses
// fixed top/bottom safe zones, so the selected CrossPoint theme never shifts the
// headline, body, source, pager, or hints or clips the bottom controls. Built-in
// fonts only; static compiled-in fixture; no network.
class GotoActivity final : public Activity {
  GotoEdition edition;
  int pageIndex = 0;
  bool loaded = false;
  GotoEditionOrigin origin = GotoEditionOrigin::None;  // drives the CACHED/OFFLINE marker
  // FULL STORY QR is a detail state OF the current article (not a child
  // activity), so the edition/pageIndex/session are untouched while it shows.
  bool showingQr = false;
  bool cleanArticleRefresh = false;  // one HALF_REFRESH when returning from QR, to clear ghosting

  int contentTopY() const;
  void drawMasthead();
  void drawStoryPage(const GotoStory& story);
  void drawChevron(int leftX, int cy, bool pointRight);
  void drawQrScreen(const GotoStory& story);  // FULL STORY QR (offline, from story.url)
  void drawEditionQrScreen();  // terminal "YOUR GOTO/TOGO IS READY" whole-edition QR (from edition.companionUrl)
  void drawBackHint(int rowTop);
  // The terminal edition page sits at index == story count: one past the last
  // story, OUTSIDE story pagination (never shown in the n/N pager).
  bool onTerminalPage() const {
    return loaded && goto_nav::isTerminal(pageIndex, static_cast<int>(edition.stories.size()));
  }
  // Pick the largest built-in serif size whose full headline fits within
  // budgetPx; ellipsize at the smallest size only as a last resort.
  void chooseHeadline(const std::string& headline, int width, int budgetPx, int& outFont,
                      std::vector<std::string>& outLines) const;

 public:
  GotoActivity(GfxRenderer& renderer, MappedInputManager& mappedInput) : Activity("Goto", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
};
