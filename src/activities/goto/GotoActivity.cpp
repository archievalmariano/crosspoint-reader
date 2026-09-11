#include "GotoActivity.h"

#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <I18n.h>

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
// --- Fonts (built-in only; E0.5 is honest to built-in constraints). ---------
constexpr int kHeadlineFont = NOTOSERIF_18_FONT_ID;  // serif bold headline
constexpr int kBodyFont = NOTOSANS_14_FONT_ID;       // excerpt paragraphs
constexpr int kMetaFont = NOTOSANS_12_FONT_ID;       // kicker, source, pager, affordance

// --- Tunable layout constants (device pixels). Grouped for visual tuning. ---
// Vertical positions are derived at render time from the active theme metrics
// and these gaps, so nothing is hard-coded to 800x480 and themes stay honored.
constexpr int kHeadlineMaxLines = 4;  // headline wraps to at most this many lines
constexpr int kSectionGap = 12;       // section kicker -> headline
constexpr int kHeadlineBodyGap = 14;  // headline -> first excerpt paragraph
constexpr int kParagraphGap = 10;     // between the two excerpt paragraphs
constexpr int kBodySourceGap = 14;    // body bottom boundary -> source line
constexpr int kSourceRuleGap = 6;     // source line -> footer rule
constexpr int kRulePagerGap = 8;      // footer rule <-> pager row spacing
constexpr int kChevronHalf = 5;       // half-height of the drawn "FULL STORY" chevron
constexpr int kChevronWidth = 5;      // horizontal reach of the chevron
constexpr int kChevronGap = 8;        // gap between "FULL STORY" and its chevron

void toUpperAscii(const char* in, char* out, size_t outSize) {
  size_t i = 0;
  for (; in[i] != '\0' && i + 1 < outSize; ++i)
    out[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(in[i])));
  out[i] = '\0';
}
}  // namespace

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
  // FULL STORY is a visual-only affordance in E0.5, so Confirm stays unbound.
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

// Draw the "FULL STORY" affordance right-aligned on the pager row. Visual only:
// the chevron is drawn (built-in fonts have no reliable "›" glyph) and nothing
// is bound to it yet.
void GotoActivity::drawFullStoryAffordance(int rightEdge, int rowTop) {
  const int labelWidth = renderer.getTextWidth(kMetaFont, "FULL STORY");
  const int chevronRight = rightEdge;
  const int chevronLeft = chevronRight - kChevronWidth;
  const int labelX = chevronLeft - kChevronGap - labelWidth;
  renderer.drawText(kMetaFont, labelX, rowTop, "FULL STORY", true);
  const int cy = rowTop + renderer.getLineHeight(kMetaFont) / 2;
  renderer.drawLine(chevronLeft, cy - kChevronHalf, chevronRight, cy, true);
  renderer.drawLine(chevronLeft, cy + kChevronHalf, chevronRight, cy, true);
}

void GotoActivity::drawStoryPage(const GotoStory& story) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int left = metrics.contentSidePadding;
  const int right = renderer.getScreenWidth() - metrics.contentSidePadding;
  const int width = right - left;

  const int metaLH = renderer.getLineHeight(kMetaFont);
  const int bodyLH = renderer.getLineHeight(kBodyFont);
  const int headlineLH = renderer.getLineHeight(kHeadlineFont);
  const int screenHeight = renderer.getScreenHeight();

  // --- Bottom-anchored footer zone (fixed, independent of body length). ---
  const int hintsTop = screenHeight - metrics.buttonHintsHeight;
  const int pagerY = hintsTop - metaLH - kRulePagerGap;
  const int footerRuleY = pagerY - kRulePagerGap;
  const int sourceY = footerRuleY - kSourceRuleGap - metaLH;
  const int bodyBottom = sourceY - kBodySourceGap;  // paragraphs must end at/above this

  // --- Top-anchored content. ---
  int y = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;

  // Section kicker (uppercase).
  char kicker[40];
  toUpperAscii(story.section.c_str(), kicker, sizeof(kicker));
  renderer.drawText(kMetaFont, left, y, kicker, true, EpdFontFamily::BOLD);
  y += metaLH + kSectionGap;

  // Headline (serif bold, wrapped; auto-ellipsized past kHeadlineMaxLines).
  const std::vector<std::string> headlineLines =
      renderer.wrappedText(kHeadlineFont, story.headline.c_str(), width, kHeadlineMaxLines, EpdFontFamily::BOLD);
  for (const std::string& line : headlineLines) {
    renderer.drawText(kHeadlineFont, left, y, line.c_str(), true, EpdFontFamily::BOLD);
    y += headlineLH;
  }
  y += kHeadlineBodyGap;

  // --- Excerpt paragraphs with deterministic fit/fallback. ---
  // Budget = vertical space between the headline and the source line. Try two
  // paragraphs; if they don't fit, drop the second; if the first alone still
  // overflows, bound it to the available lines (wrappedText adds an ellipsis).
  // Never shrink fonts, never scroll, never overlap the source/footer.
  const int bodyTop = y;
  const int bodyAvail = bodyBottom - bodyTop;

  auto drawParagraph = [&](const std::vector<std::string>& lines, int top) {
    int ly = top;
    for (const std::string& line : lines) {
      renderer.drawText(kBodyFont, left, ly, line.c_str(), true);
      ly += bodyLH;
    }
  };

  if (bodyAvail >= bodyLH && !story.excerptParagraphs.empty()) {
    // Measure each paragraph's TRUE line count: scan with a bound larger than
    // any body region can hold, so a long paragraph is never mis-measured as
    // pre-ellipsized and the truncation branch below engages correctly.
    const int scanLines = screenHeight / bodyLH + 2;
    const std::vector<std::string> p1 =
        renderer.wrappedText(kBodyFont, story.excerptParagraphs[0].c_str(), width, scanLines);
    const std::vector<std::string> p2 =
        story.excerptParagraphs.size() >= 2
            ? renderer.wrappedText(kBodyFont, story.excerptParagraphs[1].c_str(), width, scanLines)
            : std::vector<std::string>{};

    const int h1 = static_cast<int>(p1.size()) * bodyLH;
    const int h2 = static_cast<int>(p2.size()) * bodyLH;

    if (!p2.empty() && h1 + kParagraphGap + h2 <= bodyAvail) {
      drawParagraph(p1, bodyTop);  // both paragraphs
      drawParagraph(p2, bodyTop + h1 + kParagraphGap);
    } else if (h1 <= bodyAvail) {
      drawParagraph(p1, bodyTop);  // first paragraph only
    } else {
      const int maxLines = bodyAvail / bodyLH;  // >= 1 (guarded above)
      drawParagraph(renderer.wrappedText(kBodyFont, story.excerptParagraphs[0].c_str(), width, maxLines), bodyTop);
    }
  }

  // --- Source attribution + publication time (understated). ---
  char source[48];
  toUpperAscii(story.source.c_str(), source, sizeof(source));
  char sourceLine[96];
  if (!story.publishedAt.empty() && source[0] != '\0') {
    snprintf(sourceLine, sizeof(sourceLine), "%s · %s", source, story.publishedAt.c_str());
  } else {
    snprintf(sourceLine, sizeof(sourceLine), "%s%s", source, story.publishedAt.c_str());
  }
  if (sourceLine[0] != '\0') renderer.drawText(kMetaFont, left, sourceY, sourceLine, true);

  // --- Footer rule + pager + FULL STORY affordance. ---
  renderer.drawLine(left, footerRuleY, right, footerRuleY, true);
  char pager[16];
  snprintf(pager, sizeof(pager), "%d / %d", pageIndex + 1, static_cast<int>(edition.stories.size()));
  renderer.drawText(kMetaFont, left, pagerY, pager, true);
  drawFullStoryAffordance(right, pagerY);
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
    renderer.drawCenteredText(kBodyFont, pageHeight / 2, tr(STR_PAGE_LOAD_ERROR));
    const auto errLabels = mappedInput.mapLabels(tr(STR_HOME), "", "", "");
    GUI.drawButtonHints(renderer, errLabels.btn1, errLabels.btn2, errLabels.btn3, errLabels.btn4);
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
    return;
  }

  const int lastPage = static_cast<int>(edition.stories.size()) - 1;
  drawStoryPage(edition.stories[pageIndex]);

  // Button hints reflect the clamp: prev/next blank at the ends.
  const auto labels = mappedInput.mapLabels(tr(STR_HOME), "", pageIndex > 0 ? tr(STR_PREV_PAGE) : "",
                                            pageIndex < lastPage ? tr(STR_NEXT_PAGE) : "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}
