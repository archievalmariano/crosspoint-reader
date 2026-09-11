#include "GotoActivity.h"

#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <I18n.h>

#include <cctype>
#include <cstdio>
#include <string>
#include <vector>

#include "MappedInputManager.h"
#include "fontIds.h"

namespace {
// --- Fonts (built-in only; fixed GOTO choices, never theme-derived). --------
constexpr int kMastheadFont = NOTOSANS_16_FONT_ID;   // "GOTO" wordmark
constexpr int kHeadlineFont = NOTOSERIF_18_FONT_ID;  // serif bold headline
constexpr int kBodyFont = NOTOSANS_14_FONT_ID;       // lead paragraph
constexpr int kMetaFont = NOTOSANS_12_FONT_ID;       // dateline, kicker, source, pager, hints

// --- GOTO layout constants (device pixels; theme-INDEPENDENT). --------------
// Vertical positions derive only from these and the fixed font line heights, so
// no theme metric can shift the page or clip the bottom controls. Top and
// bottom safe zones keep all content clear of the physical panel edges.
constexpr int kMargin = 24;              // left/right content margin
constexpr int kTopSafe = 16;             // top safe zone (masthead baseline)
constexpr int kBottomSafe = 28;          // bottom safe zone (below the last hint row)
constexpr int kMastheadRuleGap = 8;      // masthead text -> rule
constexpr int kMastheadContentGap = 16;  // rule -> section kicker
constexpr int kSectionGap = 12;          // section kicker -> headline
constexpr int kHeadlineBodyGap = 14;     // headline -> lead paragraph
constexpr int kBodySourceGap = 16;       // body bottom boundary -> source line
constexpr int kSourceRuleGap = 8;        // source line -> footer rule
constexpr int kFooterRowGap = 10;        // footer rule <-> pager <-> nav row spacing
constexpr int kHeadlineMaxLines = 4;     // headline wraps to at most this many lines

constexpr int kChevronReach = 5;    // horizontal length of a drawn "‹"/"›"
constexpr int kChevronHalf = 4;     // half-height of a drawn chevron
constexpr int kChevronTextGap = 6;  // gap between a chevron and its label

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

  // Circular paging within the loaded edition (never between editions). Both
  // directions always move, so no control ever disappears at an "end".
  const int count = static_cast<int>(edition.stories.size());
  if (mappedInput.wasPressed(MappedInputManager::Button::PageForward)) {
    pageIndex = (pageIndex + 1) % count;
    requestUpdate();
    return;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::PageBack)) {
    pageIndex = (pageIndex - 1 + count) % count;
    requestUpdate();
    return;
  }
}

// A minimal "‹"/"›" chevron drawn from primitives (built-in fonts have no
// reliable single-angle glyph). `leftX` is the chevron box's left edge; the tip
// points right (pointRight) or left.
void GotoActivity::drawChevron(int leftX, int cy, bool pointRight) {
  if (pointRight) {
    renderer.drawLine(leftX, cy - kChevronHalf, leftX + kChevronReach, cy, true);
    renderer.drawLine(leftX, cy + kChevronHalf, leftX + kChevronReach, cy, true);
  } else {
    renderer.drawLine(leftX + kChevronReach, cy - kChevronHalf, leftX, cy, true);
    renderer.drawLine(leftX + kChevronReach, cy + kChevronHalf, leftX, cy, true);
  }
}

int GotoActivity::contentTopY() const {
  return kTopSafe + renderer.getLineHeight(kMastheadFont) + kMastheadRuleGap + kMastheadContentGap;
}

// GOTO masthead: "GOTO" wordmark + right-aligned dateline + a thin rule. Drawn
// directly so it is identical under every theme (no GUI.drawHeader dependency).
void GotoActivity::drawMasthead() {
  const int right = renderer.getScreenWidth() - kMargin;
  const int mastheadLH = renderer.getLineHeight(kMastheadFont);
  const int metaLH = renderer.getLineHeight(kMetaFont);

  renderer.drawText(kMastheadFont, kMargin, kTopSafe, edition.label.c_str(), true, EpdFontFamily::BOLD);
  if (!edition.dateline.empty()) {
    const int dateWidth = renderer.getTextWidth(kMetaFont, edition.dateline.c_str());
    // Bottom-align the smaller dateline with the wordmark.
    renderer.drawText(kMetaFont, right - dateWidth, kTopSafe + (mastheadLH - metaLH), edition.dateline.c_str(), true);
  }
  const int ruleY = kTopSafe + mastheadLH + kMastheadRuleGap;
  renderer.drawLine(kMargin, ruleY, right, ruleY, true);
}

void GotoActivity::drawStoryPage(const GotoStory& story) {
  const int right = renderer.getScreenWidth() - kMargin;
  const int width = right - kMargin;
  const int screenHeight = renderer.getScreenHeight();
  const int metaLH = renderer.getLineHeight(kMetaFont);
  const int bodyLH = renderer.getLineHeight(kBodyFont);
  const int headlineLH = renderer.getLineHeight(kHeadlineFont);

  // --- Bottom-anchored footer (fixed; independent of theme + body length). ---
  const int navRowTop = screenHeight - kBottomSafe - metaLH;
  const int pagerRowTop = navRowTop - kFooterRowGap - metaLH;
  const int footerRuleY = pagerRowTop - kFooterRowGap;
  const int sourceTop = footerRuleY - kSourceRuleGap - metaLH;
  const int bodyBottom = sourceTop - kBodySourceGap;  // the lead paragraph must end at/above this

  // --- Top-anchored content. ---
  int y = contentTopY();

  // Section kicker (uppercase).
  char kicker[40];
  toUpperAscii(story.section.c_str(), kicker, sizeof(kicker));
  renderer.drawText(kMetaFont, kMargin, y, kicker, true, EpdFontFamily::BOLD);
  y += metaLH + kSectionGap;

  // Headline (serif bold, wrapped; auto-ellipsized past kHeadlineMaxLines).
  const std::vector<std::string> headlineLines =
      renderer.wrappedText(kHeadlineFont, story.headline.c_str(), width, kHeadlineMaxLines, EpdFontFamily::BOLD);
  for (const std::string& line : headlineLines) {
    renderer.drawText(kHeadlineFont, kMargin, y, line.c_str(), true, EpdFontFamily::BOLD);
    y += headlineLH;
  }
  y += kHeadlineBodyGap;

  // --- Lead paragraph only (V1). Wrap to the fixed body area; wrappedText adds
  // an ellipsis on the last line only if the paragraph overflows. Never shrinks
  // fonts, never scrolls, never overlaps the source/footer. ---
  const int bodyTop = y;
  const int bodyAvail = bodyBottom - bodyTop;
  if (bodyAvail >= bodyLH && !story.excerptParagraphs.empty()) {
    const int maxLines = bodyAvail / bodyLH;
    const std::vector<std::string> lines =
        renderer.wrappedText(kBodyFont, story.excerptParagraphs[0].c_str(), width, maxLines);
    int ly = bodyTop;
    for (const std::string& line : lines) {
      renderer.drawText(kBodyFont, kMargin, ly, line.c_str(), true);
      ly += bodyLH;
    }
  }

  // --- Source attribution + publication time (understated). ---
  char source[48];
  toUpperAscii(story.source.c_str(), source, sizeof(source));
  char sourceLine[96];
  if (source[0] != '\0' && !story.publishedAt.empty()) {
    snprintf(sourceLine, sizeof(sourceLine), "%s · %s", source, story.publishedAt.c_str());
  } else {
    snprintf(sourceLine, sizeof(sourceLine), "%s%s", source, story.publishedAt.c_str());
  }
  if (sourceLine[0] != '\0') renderer.drawText(kMetaFont, kMargin, sourceTop, sourceLine, true);

  // --- Footer rule. ---
  renderer.drawLine(kMargin, footerRuleY, right, footerRuleY, true);

  // --- Pager + FULL STORY affordance (visual only; Confirm stays unbound). ---
  char pager[16];
  snprintf(pager, sizeof(pager), "%d / %d", pageIndex + 1, static_cast<int>(edition.stories.size()));
  renderer.drawText(kMetaFont, kMargin, pagerRowTop, pager, true);
  char fullStory[24];
  toUpperAscii(tr(STR_GOTO_FULL_STORY), fullStory, sizeof(fullStory));
  const int fsChevronLeft = right - kChevronReach;
  const int fsLabelX = fsChevronLeft - kChevronTextGap - renderer.getTextWidth(kMetaFont, fullStory);
  renderer.drawText(kMetaFont, fsLabelX, pagerRowTop, fullStory, true);
  drawChevron(fsChevronLeft, pagerRowTop + metaLH / 2, true);

  // --- Minimal unboxed nav hints: "‹ PREV" ... "NEXT ›" (both always shown,
  // since paging is circular). No boxes, no wrapping, no theme dependency. ---
  const int hintCy = navRowTop + metaLH / 2;
  char prev[16];
  toUpperAscii(tr(STR_GOTO_PREV), prev, sizeof(prev));
  drawChevron(kMargin, hintCy, false);
  renderer.drawText(kMetaFont, kMargin + kChevronReach + kChevronTextGap, navRowTop, prev, true);
  char next[16];
  toUpperAscii(tr(STR_GOTO_NEXT), next, sizeof(next));
  const int nextChevronLeft = right - kChevronReach;
  const int nextLabelX = nextChevronLeft - kChevronTextGap - renderer.getTextWidth(kMetaFont, next);
  renderer.drawText(kMetaFont, nextLabelX, navRowTop, next, true);
  drawChevron(nextChevronLeft, hintCy, true);
}

void GotoActivity::render(RenderLock&&) {
  renderer.clearScreen();
  drawMasthead();

  if (!loaded || edition.stories.empty()) {
    renderer.drawCenteredText(kBodyFont, renderer.getScreenHeight() / 2, tr(STR_PAGE_LOAD_ERROR));
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
    return;
  }

  drawStoryPage(edition.stories[pageIndex]);
  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}
