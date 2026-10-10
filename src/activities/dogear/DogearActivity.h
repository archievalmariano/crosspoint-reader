#pragma once

#include <GfxRenderer.h>

#include <string>
#include <vector>

#include "DogearIssue.h"
#include "DogearPages.h"
#include "DogearSource.h"
#include "activities/Activity.h"

// DOGEAR, a limited weekly reading digest. One literary item per portrait
// screen between a cover/contents page and a final page whose QR opens the
// week's full web edition. An item with a reading link opens a secondary QR
// view (READ ONLINE / FIND THE BOOK) that Back closes.
//
// Like GOTO, the layout is theme-independent (built-in Noto fonts, fixed
// margins) and the issue is loaded once per session, offline-first. The device
// only lays out display-ready data; selection happens off-device.
class DogearActivity final : public Activity {
  // How one item lands on screen, measured once per session.
  struct ItemLayout {
    std::vector<std::string> kicker;
    int headlineFont = 0;
    std::vector<std::string> headline;
    int bodyFont = 0;
    std::vector<std::string> parts[2];  // body lines per screen
    uint8_t partCount = 1;
  };

  DogearIssue issue;
  std::vector<ItemLayout> layouts;
  dogear_pages::Screen screens[dogear_pages::kMaxScreens];
  int screenCount = 0;
  int index = 0;
  bool loaded = false;
  DogearOrigin origin = DogearOrigin::None;
  dogear_state::Screen screen = dogear_state::Screen::CantLoad;

  bool showingQr = false;     // the current item's reading view
  bool cleanRefresh = false;  // one HALF_REFRESH after a QR screen, against ghosting
  bool firstRender = true;
  dogear_pages::PressGate backGate;
  dogear_pages::PressGate confirmGate;
  // The footer action's touch target, valid while a screen with one is drawn.
  bool actionTapValid = false;
  int actionX = 0;
  int actionY = 0;
  int actionW = 0;
  int actionH = 0;

  void loadIssue();
  void layoutIssue();
  const DogearEntry* currentEntry() const;
  bool currentHasAction() const;

  // Layout helpers.
  int headBottom() const;
  int pagerTop() const;
  int bodyBottom() const;
  int continuationTop() const;
  std::vector<std::string> wrap(int font, const std::string& text, int maxLines,
                                EpdFontFamily::Style style = EpdFontFamily::REGULAR) const;
  // A cover title within maxLines, cut at a word boundary (DogearText.h).
  std::vector<std::string> coverTitle(int font, const std::string& text, int maxLines,
                                      EpdFontFamily::Style style = EpdFontFamily::REGULAR) const;
  int centeredFit(int y, const char* text, const int* fonts, int fontCount, EpdFontFamily::Style style) const;
  int drawQr(int top, int maxSide, const std::string& url) const;

  // Screens.
  void drawRunningHead() const;
  void drawFooter(const char* left, bool leftBold, const char* right);
  void drawCover();
  void drawQuietContents(int y);  // a quiet week's cover below the dateline (Quiet A)
  void drawItem(const dogear_pages::Screen& s);
  void drawFinal();
  void drawReadingQr(const DogearEntry& e);
  void drawStatus();  // NO ISSUE YET, NO ISSUE ON THIS DEVICE, CAN'T LOAD AN ISSUE

 public:
  DogearActivity(GfxRenderer& renderer, MappedInputManager& mappedInput) : Activity("Dogear", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
};
