#include "GotoActivity.h"

#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <I18n.h>

#include <cctype>
#include <cstdio>
#include <string>
#include <vector>

#include "GotoEditionSource.h"
#include "MappedInputManager.h"
#include "fontIds.h"
#include "util/QrUtils.h"

namespace {
// --- Fonts (built-in only; fixed GOTO choices, never theme-derived). --------
constexpr int kMastheadFont = NOTOSANS_16_FONT_ID;   // "GOTO" wordmark
constexpr int kHeadlineFont = NOTOSERIF_18_FONT_ID;  // preferred serif headline
// Adaptive serif headline ladder (largest first): fit the COMPLETE headline at
// the biggest size that stays within the headline height budget, stepping down
// before ellipsizing. Built-in serif faces only; no external font assets.
constexpr int kHeadlineFonts[] = {NOTOSERIF_18_FONT_ID, NOTOSERIF_16_FONT_ID, NOTOSERIF_14_FONT_ID};
constexpr int kBodyFont = NOTOSANS_14_FONT_ID;          // lead paragraph (normal)
constexpr int kBodyFallbackFont = NOTOSANS_12_FONT_ID;  // lead paragraph (one step smaller)
constexpr int kMetaFont = NOTOSANS_12_FONT_ID;          // dateline, kicker, source, pager

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
constexpr int kHeadlineBodyGap = 12;     // headline -> lead paragraph (body reclaims 2px)
constexpr int kBodySourceGap = 12;       // body bottom boundary -> source line (body reclaims 4px; source/footer fixed)
constexpr int kSourceRuleGap = 8;        // source line -> footer rule
constexpr int kRulePagerGap = 10;        // footer rule -> pager row
constexpr int kHeadlineMaxLines = 4;     // headline wraps to at most this many lines
// Defensive layout bound: an 800x480 page holds well under this many lead
// characters even at the smallest body size, so measuring beyond it is wasted
// work. Caps render cost for any pathological over-long lead.
constexpr size_t kBodyMeasureCapChars = 1200;

constexpr int kChevronReach = 5;    // horizontal length of a drawn "‹"/"›"
constexpr int kChevronHalf = 4;     // half-height of a drawn chevron
constexpr int kChevronTextGap = 6;  // gap between a chevron and its label

void toUpperAscii(const char* in, char* out, size_t outSize) {
  size_t i = 0;
  for (; in[i] != '\0' && i + 1 < outSize; ++i)
    out[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(in[i])));
  out[i] = '\0';
}

// Minimal V1 sanity for the Full Story QR: a non-empty http(s) URL within the
// QR encoder's byte capacity. Not a general URL/security validator.
bool storyUrlIsValid(const std::string& url) {
  if (url.empty() || url.size() > 2953) return false;
  return url.rfind("https://", 0) == 0 || url.rfind("http://", 0) == 0;
}
}  // namespace

void GotoActivity::onEnter() {
  Activity::onEnter();
  // Load once per session (offline-first: network -> SD cache -> builtin). The
  // edition is fixed for this reading session; a server-side change is only
  // picked up on the next entry (Back -> Home -> GOTO), never mid-session.
  const GotoLoadResult r = loadCurrentGotoEdition(edition);
  loaded = r.origin != GotoEditionOrigin::None && !edition.stories.empty();
  origin = r.origin;  // Network/CacheCurrent -> no marker; CacheStale -> CACHED; Builtin -> OFFLINE
  pageIndex = 0;      // reset to the first page on every (re-)entry
  requestUpdate();
}

void GotoActivity::loop() {
  using Button = MappedInputManager::Button;

  // Dedicated Home key (X4 Pro / any home-key board): exit GOTO to CrossPoint
  // home from ANY screen. Board-agnostic — boards without a home key return
  // false here, so this is inert on the X4 (which exits via Back below).
  if (mappedInput.wasHomeGesture() || mappedInput.wasHomeKeyHold()) {
    activityManager.goHome();
    return;
  }

  // Full Story QR is a modal detail of the current article. Handle it FIRST,
  // BEFORE the RenderLock gate, so backing out stays responsive even while the
  // QR's slow full refresh is still painting — otherwise a press/tap that lands
  // during that refresh is dropped and the exit feels laggy and inconsistent
  // (the front Back button never had this because it, too, is handled pre-gate).
  // Back out to the SAME article (no reload/network/page reset) via: the front
  // Back button (X4) or left-edge back swipe / screen tap (touch), or a side key
  // (touch boards only — they have no front Back button; matches the Left key's
  // "go back" role on the terminal page). Nothing else acts while the QR shows.
  if (showingQr) {
    int tx = 0;
    int ty = 0;
    const bool backOut = mappedInput.wasReleased(Button::Back) || mappedInput.wasScreenTapped(tx, ty) ||
                         (mappedInput.hasTouch() &&
                          (mappedInput.wasPressed(Button::NavNext) || mappedInput.wasPressed(Button::NavPrevious)));
    if (backOut) {
      showingQr = false;
      cleanArticleRefresh = true;  // one HALF_REFRESH to scrub QR ghosting
      requestUpdate();
    }
    return;
  }

  // Back on a story/terminal page = exit GOTO to Home (front Back button on X4,
  // or a left-edge back swipe on touch boards). Pre-gate so it stays responsive.
  if (mappedInput.wasReleased(Button::Back)) {
    activityManager.goHome();
    return;
  }

  if (!loaded || edition.stories.empty()) return;

  // One gesture -> one transition. render() runs on a separate render task;
  // while it holds the RenderLock (a refresh is in progress) ignore paging input
  // so presses during a slow refresh cannot queue a burst. Back and the QR modal
  // are handled above and stay responsive.
  if (RenderLock::peek()) return;

  // Open the per-story FULL STORY QR handoff: the front Confirm button (X4), or a
  // tap on the on-screen FULL STORY affordance (touch boards / X4 Pro). Only on a
  // story page; the terminal edition page is itself a QR, so this is inert there.
  if (!onTerminalPage() && (mappedInput.wasReleased(Button::Confirm) ||
                            (fullStoryTapValid && mappedInput.wasTapInRect(fsTapX, fsTapY, fsTapW, fsTapH)))) {
    showingQr = true;
    requestUpdate();
    return;
  }

  // Navigation via CrossPoint's native NavNext/NavPrevious, which resolve
  // (MappedInputManager::mapButton) to BOTH the side page buttons (Up/Down) and
  // the front right rocker (Left/Right), with orientation handling. Circular
  // within the loaded edition (never between editions). The ring is the stories
  // PLUS one terminal edition page after the last story: last story -> Next ->
  // terminal; terminal -> Previous -> last story; terminal -> Next -> story 1.
  const int storyCount = static_cast<int>(edition.stories.size());
  const bool leavingTerminal = onTerminalPage();

  // Terminal edition-QR page: a screen tap (its BACK cue) returns to the last
  // story — the same result as the Left key here and as tapping BACK on the
  // Full Story QR. Inert on button boards (no touch), which back out via the
  // side keys as before.
  if (leavingTerminal) {
    int tx = 0;
    int ty = 0;
    if (mappedInput.wasScreenTapped(tx, ty)) {
      pageIndex = goto_nav::prevIndex(pageIndex, storyCount);
      cleanArticleRefresh = true;  // scrub QR ghosting on the story we land on
      requestUpdate();
      return;
    }
  }

  if (mappedInput.wasPressed(Button::NavNext)) {
    pageIndex = goto_nav::nextIndex(pageIndex, storyCount);
    if (leavingTerminal) cleanArticleRefresh = true;  // scrub QR ghosting on the story we land on
    requestUpdate();
    return;
  }
  if (mappedInput.wasPressed(Button::NavPrevious)) {
    pageIndex = goto_nav::prevIndex(pageIndex, storyCount);
    if (leavingTerminal) cleanArticleRefresh = true;
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

void GotoActivity::chooseHeadline(const std::string& headline, int width, int budgetPx, int& outFont,
                                  std::vector<std::string>& outLines) const {
  // Largest serif size whose COMPLETE wrap fits inside budgetPx wins.
  for (const int font : kHeadlineFonts) {
    const int lh = renderer.getLineHeight(font);
    const int maxLines = lh > 0 ? budgetPx / lh : 1;
    std::vector<std::string> lines =
        renderer.wrappedText(font, headline.c_str(), width, maxLines + 1, EpdFontFamily::BOLD);
    if (static_cast<int>(lines.size()) <= maxLines) {
      outFont = font;
      outLines = std::move(lines);
      return;
    }
  }
  // Nothing fit in full: ellipsize at the smallest serif size, bounded to the
  // budget (wrappedText appends U+2026 on the last visible line).
  outFont = kHeadlineFonts[sizeof(kHeadlineFonts) / sizeof(kHeadlineFonts[0]) - 1];
  const int lh = renderer.getLineHeight(outFont);
  const int maxLines = lh > 0 ? budgetPx / lh : 1;
  outLines = renderer.wrappedText(outFont, headline.c_str(), width, maxLines, EpdFontFamily::BOLD);
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

  // --- Bottom-anchored footer (fixed; independent of theme + body length).
  // The pager row is the bottom-most element now (no persistent nav hints);
  // that reclaims a row of vertical space for the lead paragraph. ---
  const int pagerRowTop = screenHeight - kBottomSafe - metaLH;
  const int footerRuleY = pagerRowTop - kRulePagerGap;
  const int sourceTop = footerRuleY - kSourceRuleGap - metaLH;
  const int bodyBottom = sourceTop - kBodySourceGap;  // the lead paragraph must end at/above this

  // --- Top-anchored content. ---
  int y = contentTopY();

  // Section kicker (uppercase).
  char kicker[40];
  toUpperAscii(story.section.c_str(), kicker, sizeof(kicker));
  renderer.drawText(kMetaFont, kMargin, y, kicker, true, EpdFontFamily::BOLD);
  y += metaLH + kSectionGap;

  // Headline (serif bold). Adaptive: prefer the large size, but fit the COMPLETE
  // headline within a fixed vertical budget by stepping the serif size down
  // before ellipsizing (last resort). The budget is the preferred size's max
  // lines, so a smaller size buys extra lines from the same height — body space
  // below stays bounded (deterministic), so the lead never gets squeezed.
  const int headlineBudgetPx = kHeadlineMaxLines * headlineLH;
  int headlineFont = kHeadlineFont;
  std::vector<std::string> headlineLines;
  chooseHeadline(story.headline, width, headlineBudgetPx, headlineFont, headlineLines);
  const int chosenHeadlineLH = renderer.getLineHeight(headlineFont);
  for (const std::string& line : headlineLines) {
    renderer.drawText(headlineFont, kMargin, y, line.c_str(), true, EpdFontFamily::BOLD);
    y += chosenHeadlineLH;
  }
  y += kHeadlineBodyGap;

  // --- Lead paragraph (V1). Prefer the COMPLETE first paragraph: (A) try the
  // normal body font; (B) if it doesn't fully fit, retry once at the one smaller
  // built-in body size; (C) only if it still doesn't fit, truncate at the
  // smaller size with an ellipsis. Ellipsis is a last resort; fonts never shrink
  // beyond the single approved fallback; never scrolls, never overlaps footer. ---
  const int bodyTop = y;
  const int bodyAvail = bodyBottom - bodyTop;
  const int fallbackLH = renderer.getLineHeight(kBodyFallbackFont);
  const char* bodyMode = "empty";  // instrumentation: which fit branch was taken
  if (bodyAvail >= fallbackLH && !story.excerptParagraphs.empty()) {
    // Defensive measure-bound: never lay out more than can possibly display.
    // wrappedText scans the whole input, so a pathological multi-hundred-word
    // lead (e.g. the old flattened live-update payload, now fixed upstream) made
    // page-1 layout cost seconds. The corrected extractor keeps leads short;
    // this cap guarantees the render path stays bounded regardless of input.
    const std::string& src = story.excerptParagraphs[0];
    std::string capped;
    const char* lead = src.c_str();
    if (src.size() > kBodyMeasureCapChars) {
      const size_t space = src.rfind(' ', kBodyMeasureCapChars);
      capped = src.substr(0, space == std::string::npos ? kBodyMeasureCapChars : space);
      lead = capped.c_str();
    }
    const uint32_t wrapStartMs = millis();

    // True (unbounded) wraps at each size, to know whether the whole lead fits.
    const std::vector<std::string> normalLines =
        renderer.wrappedText(kBodyFont, lead, width, screenHeight / bodyLH + 2);

    int bodyFont = kBodyFont;
    int lineHeight = bodyLH;
    std::vector<std::string> lines;
    if (static_cast<int>(normalLines.size()) * bodyLH <= bodyAvail) {
      lines = normalLines;  // STEP A: full paragraph at the normal size
      bodyMode = "normal-14";
    } else {
      const std::vector<std::string> fallbackLines =
          renderer.wrappedText(kBodyFallbackFont, lead, width, screenHeight / fallbackLH + 2);
      bodyFont = kBodyFallbackFont;
      lineHeight = fallbackLH;
      if (static_cast<int>(fallbackLines.size()) * fallbackLH <= bodyAvail) {
        lines = fallbackLines;  // STEP B: full paragraph at the smaller size
        bodyMode = "fallback-12";
      } else {
        // STEP C (last resort): bound to the fitting lines; wrappedText ellipsizes.
        lines = renderer.wrappedText(kBodyFallbackFont, lead, width, bodyAvail / fallbackLH);
        bodyMode = "truncated-12";  // content contract (full first paragraph) is the limiter
      }
    }

    int ly = bodyTop;
    for (const std::string& line : lines) {
      renderer.drawText(bodyFont, kMargin, ly, line.c_str(), true);
      ly += lineHeight;
    }
    // Instrumentation (compiled out at LOG_LEVEL=0): confirm on hardware that
    // refresh mode is uniform (always FAST) and see the per-page fit decision +
    // the actual body layout cost (the suspected page-1 latency source).
    LOG_DBG("GOTO",
            "page %d/%d refresh=FAST headline=%dpx x%d bodyAvail=%dpx body=%s lines14=%d srcChars=%d wrap=%lums",
            pageIndex + 1, static_cast<int>(edition.stories.size()), chosenHeadlineLH,
            static_cast<int>(headlineLines.size()), bodyAvail, bodyMode, static_cast<int>(normalLines.size()),
            static_cast<int>(story.excerptParagraphs[0].size()), millis() - wrapStartMs);
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
  // No persistent PREV/NEXT hints: navigation uses the X4's native front rocker
  // and side buttons (see loop()), which CrossPoint does not label on-screen.

  // Record the FULL STORY tap target for touch boards (X4 Pro): a generous
  // bottom-right band from the footer rule down, covering the label + chevron.
  // Button boards never read it (they open the QR with Confirm).
  fsTapX = fsLabelX - kChevronTextGap;
  fsTapY = footerRuleY;
  fsTapW = renderer.getScreenWidth() - fsTapX;
  fsTapH = screenHeight - footerRuleY;
  fullStoryTapValid = true;

  // --- Subtle liveness marker (centered between pager and FULL STORY). Live
  // states show nothing: a fresh network fetch (Network) OR a cached edition the
  // server manifest confirmed current this session (CacheCurrent). CACHED is
  // reserved for a stale/unverified SD edition (network/server failed); OFFLINE
  // for the builtin fixture. Keeps the post-reboot state unambiguous. ---
  const char* marker = origin == GotoEditionOrigin::CacheStale ? "CACHED"
                       : origin == GotoEditionOrigin::Builtin  ? "OFFLINE"
                                                               : nullptr;
  if (marker != nullptr) {
    const int markerX = (renderer.getScreenWidth() - renderer.getTextWidth(kMetaFont, marker)) / 2;
    renderer.drawText(kMetaFont, markerX, pagerRowTop, marker, true);
  }
}

// A subtle, unboxed Back cue (no touch-style chrome). STR_BACK already carries a
// guillemet, so no drawn chevron is needed here.
void GotoActivity::drawBackHint(int rowTop) { renderer.drawCenteredText(kMetaFont, rowTop, tr(STR_BACK)); }

// FULL STORY: a local, offline QR of the story's canonical GMA URL. The QR is the
// dominant element; no network, no browser, no backend. Theme-independent, GOTO-
// styled (masthead already drawn by render()).
void GotoActivity::drawQrScreen(const GotoStory& story) {
  const int screenWidth = renderer.getScreenWidth();
  const int screenHeight = renderer.getScreenHeight();
  const int metaLH = renderer.getLineHeight(kMetaFont);

  int y = contentTopY();
  char kicker[24];
  toUpperAscii(tr(STR_GOTO_FULL_STORY), kicker, sizeof(kicker));
  renderer.drawText(kMetaFont, kMargin, y, kicker, true, EpdFontFamily::BOLD);

  // Fixed bottom-anchored rows: Back hint, and above it the scan caption.
  const int backRowTop = screenHeight - kBottomSafe - metaLH;
  const int captionTop = backRowTop - kRulePagerGap - metaLH;

  if (!storyUrlIsValid(story.url)) {
    renderer.drawCenteredText(kBodyFont, screenHeight / 2, tr(STR_GOTO_LINK_UNAVAILABLE));
    drawBackHint(backRowTop);
    return;
  }

  // QR centered in the space between the kicker and the caption. Integer module
  // scaling comes from QrUtils; the surrounding white (the cleared page minus
  // this inset box) is the quiet zone.
  const int qrTop = y + metaLH + kSectionGap;
  const int qrBottom = captionTop - kBodySourceGap;
  const Rect qrBounds(kMargin, qrTop, screenWidth - 2 * kMargin, qrBottom - qrTop);
  QrUtils::drawQrCode(renderer, qrBounds, story.url);

  // "Scan to read on <source>" — edition-agnostic; source is display DATA.
  char caption[64];
  snprintf(caption, sizeof(caption), "%s %s", tr(STR_GOTO_SCAN_TO_READ), story.source.c_str());
  renderer.drawCenteredText(kMetaFont, captionTop, caption);

  drawBackHint(backRowTop);
}

// Terminal edition page (outside story pagination): "YOUR GOTO/TOGO IS READY" +
// a whole-edition QR of the hosted companion page (edition.companionUrl) + "SCAN
// FOR THIS EDITION". Offline: companionUrl comes from the cached manifest. Mirrors
// drawQrScreen's geometry; masthead already drawn by render(). No pager here.
void GotoActivity::drawEditionQrScreen() {
  const int screenWidth = renderer.getScreenWidth();
  const int screenHeight = renderer.getScreenHeight();
  const int metaLH = renderer.getLineHeight(kMetaFont);

  int y = contentTopY();
  char kicker[32];
  snprintf(kicker, sizeof(kicker), tr(STR_GOTO_EDITION_READY), edition.label.c_str());
  renderer.drawText(kMetaFont, kMargin, y, kicker, true, EpdFontFamily::BOLD);

  const int backRowTop = screenHeight - kBottomSafe - metaLH;
  const int captionTop = backRowTop - kRulePagerGap - metaLH;

  if (!storyUrlIsValid(edition.companionUrl)) {
    renderer.drawCenteredText(kBodyFont, screenHeight / 2, tr(STR_GOTO_LINK_UNAVAILABLE));
    drawBackHint(backRowTop);
    return;
  }

  const int qrTop = y + metaLH + kSectionGap;
  const int qrBottom = captionTop - kBodySourceGap;
  const Rect qrBounds(kMargin, qrTop, screenWidth - 2 * kMargin, qrBottom - qrTop);
  QrUtils::drawQrCode(renderer, qrBounds, edition.companionUrl);

  renderer.drawCenteredText(kMetaFont, captionTop, tr(STR_GOTO_SCAN_EDITION));

  drawBackHint(backRowTop);
}

void GotoActivity::render(RenderLock&&) {
  // The FULL STORY tap target is only valid while a story page is on screen;
  // drawStoryPage re-arms it below. QR / terminal pages leave it disarmed.
  fullStoryTapValid = false;
  renderer.clearScreen();
  drawMasthead();

  if (!loaded || edition.stories.empty()) {
    renderer.drawCenteredText(kBodyFont, renderer.getScreenHeight() / 2, tr(STR_PAGE_LOAD_ERROR));
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
    return;
  }

  if (onTerminalPage()) {
    // The whole-edition QR page after the last story (outside story pagination).
    // Dense QR geometry scans best off a complete waveform, like the FULL STORY QR.
    drawEditionQrScreen();
    renderer.displayBuffer(HalDisplay::FULL_REFRESH);
    return;
  }

  if (showingQr) {
    // Dense QR geometry scans best off a complete waveform; scan reliability
    // outranks speed here, so a full refresh on QR entry is worth the flash.
    drawQrScreen(edition.stories[pageIndex]);
    renderer.displayBuffer(HalDisplay::FULL_REFRESH);
    return;
  }

  drawStoryPage(edition.stories[pageIndex]);
  // Returning from the QR uses one HALF_REFRESH to scrub QR ghosting; ordinary
  // page turns stay on the fast path.
  const HalDisplay::RefreshMode mode = cleanArticleRefresh ? HalDisplay::HALF_REFRESH : HalDisplay::FAST_REFRESH;
  cleanArticleRefresh = false;
  renderer.displayBuffer(mode);
}
