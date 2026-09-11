#include "GotoActivity.h"

#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <I18n.h>

#include <cctype>
#include <cstdio>
#include <string>
#include <vector>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

void GotoActivity::onEnter() {
  Activity::onEnter();
  loaded = loadBuiltinGotoEdition(edition);
  pageIndex = 0;  // reset to the first page on every (re-)entry
  requestUpdate();
}

void GotoActivity::loop() {
  // Back always returns Home, even before content loads.
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    activityManager.goHome();
    return;
  }

  if (!loaded || edition.stories.empty()) return;

  const int lastPage = static_cast<int>(edition.stories.size()) - 1;

  // Forward / back paging, clamped at both ends (no wrap, no infinite feed).
  if (mappedInput.wasPressed(MappedInputManager::Button::PageForward)) {
    if (pageIndex < lastPage) {
      pageIndex++;
      requestUpdate();
    }
    return;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::PageBack)) {
    if (pageIndex > 0) {
      pageIndex--;
      requestUpdate();
    }
    return;
  }
}

void GotoActivity::drawStoryPage(const GotoStory& story) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int contentLeft = metrics.contentSidePadding;
  const int contentWidth = pageWidth - 2 * metrics.contentSidePadding;

  int y = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;

  // Section kicker (uppercase). Built-in Noto Sans has the glyphs; the mock's
  // letter-spacing is cosmetic and intentionally omitted on device for E0.
  char kicker[40];
  snprintf(kicker, sizeof(kicker), "%s", story.section.c_str());
  for (char* c = kicker; *c; ++c) *c = static_cast<char>(std::toupper(static_cast<unsigned char>(*c)));
  renderer.drawText(NOTOSANS_12_FONT_ID, contentLeft, y, kicker, true, EpdFontFamily::BOLD);
  y += renderer.getLineHeight(NOTOSANS_12_FONT_ID) + metrics.verticalSpacing * 2;

  // Headline (serif bold, wrapped).
  const int headlineLineHeight = renderer.getLineHeight(NOTOSERIF_18_FONT_ID);
  const std::vector<std::string> headlineLines =
      renderer.wrappedText(NOTOSERIF_18_FONT_ID, story.headline.c_str(), contentWidth, 4, EpdFontFamily::BOLD);
  for (const std::string& line : headlineLines) {
    renderer.drawText(NOTOSERIF_18_FONT_ID, contentLeft, y, line.c_str(), true, EpdFontFamily::BOLD);
    y += headlineLineHeight;
  }
  y += metrics.verticalSpacing * 2;

  // Dek (sans, wrapped).
  const int dekLineHeight = renderer.getLineHeight(NOTOSANS_14_FONT_ID);
  const std::vector<std::string> dekLines =
      renderer.wrappedText(NOTOSANS_14_FONT_ID, story.dek.c_str(), contentWidth, 6);
  for (const std::string& line : dekLines) {
    renderer.drawText(NOTOSANS_14_FONT_ID, contentLeft, y, line.c_str(), true);
    y += dekLineHeight;
  }
}

void GotoActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();

  // Masthead: edition label + display-ready dateline, via the theme header so
  // it stays orientation-aware and consistent with the rest of the UI.
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, edition.label.c_str(),
                 edition.dateline.empty() ? nullptr : edition.dateline.c_str());

  if (!loaded || edition.stories.empty()) {
    renderer.drawCenteredText(NOTOSANS_14_FONT_ID, pageHeight / 2, tr(STR_PAGE_LOAD_ERROR));
    const auto errLabels = mappedInput.mapLabels(tr(STR_HOME), "", "", "");
    GUI.drawButtonHints(renderer, errLabels.btn1, errLabels.btn2, errLabels.btn3, errLabels.btn4);
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
    return;
  }

  const int lastPage = static_cast<int>(edition.stories.size()) - 1;
  drawStoryPage(edition.stories[pageIndex]);

  // Page indicator "n / N", centered just above the button hints.
  char pager[16];
  snprintf(pager, sizeof(pager), "%d / %d", pageIndex + 1, static_cast<int>(edition.stories.size()));
  const int pagerY = pageHeight - metrics.buttonHintsHeight - renderer.getLineHeight(NOTOSANS_12_FONT_ID);
  renderer.drawCenteredText(NOTOSANS_12_FONT_ID, pagerY, pager);

  // Button hints reflect the clamp: prev/next blank at the ends.
  const auto labels = mappedInput.mapLabels(tr(STR_HOME), "", pageIndex > 0 ? tr(STR_PREV_PAGE) : "",
                                            pageIndex < lastPage ? tr(STR_NEXT_PAGE) : "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}
