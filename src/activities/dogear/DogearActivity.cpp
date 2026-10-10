// Compiled only into builds that enable DOGEAR (see [dogear] in platformio.ini).
#ifdef DOGEAR_ENABLED

#include "DogearActivity.h"

#include <HalDisplay.h>
#include <I18n.h>
#include <Logging.h>
#include <Memory.h>
#include <WiFi.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "DogearLimits.h"
#include "DogearText.h"
#include "MappedInputManager.h"
#include "activities/network/WifiSelectionActivity.h"
#include "activities/reader/ReaderUtils.h"
#include "fontIds.h"
#include "util/QrUtils.h"
#include "util/QrVersionSelect.h"

namespace {
using dogear_pages::Kind;
using dogear_pages::Screen;

// Fonts: built-in Noto only, never theme-derived. Body copy is serif.
constexpr int kMeta = NOTOSANS_12_FONT_ID;  // running head, kickers, footer
constexpr int kSans14 = NOTOSANS_14_FONT_ID;
constexpr int kSans16 = NOTOSANS_16_FONT_ID;
constexpr int kSans18 = NOTOSANS_18_FONT_ID;
constexpr int kSerif12 = NOTOSERIF_12_FONT_ID;
constexpr int kSerif14 = NOTOSERIF_14_FONT_ID;
constexpr int kSerif16 = NOTOSERIF_16_FONT_ID;
constexpr int kSerif18 = NOTOSERIF_18_FONT_ID;
constexpr int kFeaturedHeadline[] = {kSerif18, kSerif16};
constexpr int kStandardHeadline[] = {kSerif16, kSerif14};
constexpr int kBody[] = {kSerif14, kSerif12};  // 14 preferred; 12 the controlled fallback

// Layout (device px), the same constants as dogear/mock/render_dogear.py.
constexpr int kMargin = 24;
constexpr int kTopSafe = 16;
constexpr int kBottomSafe = 28;
constexpr int kHeadRuleGap = 8;
constexpr int kRuleContentGap = 18;
constexpr int kFeaturedRule = 3;
constexpr int kFeaturedRuleGap = 14;
constexpr int kKickerHeadlineGap = 10;
constexpr int kHeadlineBodyGap = 14;
constexpr int kFeaturedHeadlineBodyGap = 20;
constexpr int kBodyFooterGap = 12;
constexpr int kRulePagerGap = 10;
constexpr int kHeadlineMaxLines = 3;
constexpr int kMaxWrapLines = 64;  // more than any screen holds; measures true length
constexpr int kFinalQrMax = 330;
// Cover: a contents label's step is its cap height plus a few pixels, so the
// label and its title read as one unit.
constexpr int kContentsLabelStep = 22;
constexpr int kCoverDatelineGap = 18;
constexpr int kCoverLeadGap = 14;
constexpr int kCoverRuleGap = 12;
constexpr int kCoverHeadGap = 8;
constexpr int kQrQuietModules = 4;

std::string upperAscii(const std::string& in) {
  std::string out = in;
  for (char& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return out;
}

void fillRule(const GfxRenderer& r, int y, int weight, int x0, int x1) { r.fillRect(x0, y, x1 - x0 + 1, weight, true); }
}  // namespace

// ---- lifecycle --------------------------------------------------------------

void DogearActivity::onEnter() {
  Activity::onEnter();
  backGate.reset();
  confirmGate.reset();
  if (dogearWifiConnected()) {
    loadIssue();
    return;
  }
  // GOTO's tested path: CrossPoint's own Wi-Fi picker tries the saved network;
  // Back skips it and the cached issue loads.
  auto picker = makeUniqueNoThrow<WifiSelectionActivity>(renderer, mappedInput);
  if (!picker) {
    LOG_ERR("DOGEAR", "OOM: WifiSelectionActivity; loading offline");
    loadIssue();
    return;
  }
  WiFi.mode(WIFI_STA);
  startActivityForResult(std::move(picker), [this](const ActivityResult&) { loadIssue(); });
}

void DogearActivity::loadIssue() {
  const DogearLoad load = loadCurrentDogearIssue(issue);
  origin = load.origin;
  screen = load.screen;
  loaded = !dogear_state::isStatus(screen) && !issue.entries.empty();
  if (loaded) layoutIssue();
  // An issue that parsed but cannot be laid out is one DOGEAR could not load.
  if (!loaded && !dogear_state::isStatus(screen)) screen = dogear_state::Screen::CantLoad;
  index = 0;
  showingQr = false;
  firstRender = true;
  requestUpdate();
}

// Measure every item once: kicker, headline size, body size and screens.
void DogearActivity::layoutIssue() {
  layouts.clear();
  layouts.reserve(issue.entries.size());  // bounded by dogear_limits::kMaxEntries
  uint8_t parts[dogear_limits::kMaxEntries];
  for (size_t i = 0; i < issue.entries.size(); ++i) {
    const DogearEntry& e = issue.entries[i];
    ItemLayout lay;
    lay.kicker = wrap(kMeta, upperAscii(e.dateLabel) + " · " + e.kicker, 2, EpdFontFamily::BOLD);

    const int* sizes = e.featured ? kFeaturedHeadline : kStandardHeadline;
    lay.headlineFont = sizes[1];
    lay.headline.clear();
    for (int k = 0; k < 2; ++k) {
      auto lines = wrap(sizes[k], e.headline, kHeadlineMaxLines + 1, EpdFontFamily::BOLD);
      if (static_cast<int>(lines.size()) <= kHeadlineMaxLines) {
        lay.headlineFont = sizes[k];
        lay.headline = std::move(lines);
        break;
      }
    }
    if (lay.headline.empty()) lay.headline = wrap(lay.headlineFont, e.headline, kHeadlineMaxLines, EpdFontFamily::BOLD);

    int top = headBottom();
    if (e.featured) top += kFeaturedRule + kFeaturedRuleGap;
    top += static_cast<int>(lay.kicker.size()) * renderer.getLineHeight(kMeta) + kKickerHeadlineGap;
    top += static_cast<int>(lay.headline.size()) * renderer.getLineHeight(lay.headlineFont);
    top += e.featured ? kFeaturedHeadlineBodyGap : kHeadlineBodyGap;
    const int avail = bodyBottom() - top;

    bool placed = false;
    for (const int font : kBody) {  // one screen at 14, then 12
      auto lines = wrap(font, e.body, kMaxWrapLines);
      if (static_cast<int>(lines.size()) * renderer.getLineHeight(font) <= avail) {
        lay.bodyFont = font;
        lay.parts[0] = std::move(lines);
        lay.partCount = 1;
        placed = true;
        break;
      }
    }
    if (!placed && e.secondPage) {  // the rare second screen the editor allowed
      for (const int font : kBody) {
        auto lines = wrap(font, e.body, kMaxWrapLines);
        const int lh = renderer.getLineHeight(font);
        const size_t first = static_cast<size_t>(std::max(0, avail / lh));
        const size_t second = static_cast<size_t>(std::max(0, (bodyBottom() - continuationTop()) / lh));
        if (lines.size() <= first + second) {
          lay.bodyFont = font;
          lay.parts[0].assign(lines.begin(), lines.begin() + std::min(first, lines.size()));
          lay.parts[1].assign(lines.begin() + std::min(first, lines.size()), lines.end());
          lay.partCount = 2;
          placed = true;
          break;
        }
      }
    }
    if (!placed) {  // never expected (the dataset fit check blocks it): truncate at 12
      lay.bodyFont = kSerif12;
      lay.parts[0] = wrap(kSerif12, e.body, std::max(1, avail / renderer.getLineHeight(kSerif12)));
      lay.partCount = 1;
      LOG_ERR("DOGEAR", "item %s overflows its screen; truncated", e.id.c_str());
    }
    parts[i] = lay.partCount;
    LOG_DBG("DOGEAR", "item %u %s: headline %d lines, body font %d, %u screen(s)", static_cast<unsigned>(i + 1),
            e.id.c_str(), static_cast<int>(lay.headline.size()), lay.bodyFont, lay.partCount);
    layouts.push_back(std::move(lay));
  }
  screenCount = dogear_pages::build(parts, static_cast<int>(issue.entries.size()), screens, dogear_pages::kMaxScreens);
  if (screenCount == 0) loaded = false;
}

// ---- input ------------------------------------------------------------------

const DogearEntry* DogearActivity::currentEntry() const {
  if (!loaded || index >= screenCount || screens[index].kind != Kind::Item) return nullptr;
  return &issue.entries[screens[index].entry];
}

bool DogearActivity::currentHasAction() const {
  const DogearEntry* e = currentEntry();
  return e && e->hasCta && dogear_pages::showsAction(screens[index]);
}

void DogearActivity::loop() {
  using Button = MappedInputManager::Button;
  const bool backReleased =
      backGate.update(mappedInput.wasPressed(Button::Back), mappedInput.wasReleased(Button::Back));
  const bool confirmReleased =
      confirmGate.update(mappedInput.wasPressed(Button::Confirm), mappedInput.wasReleased(Button::Confirm));

  if (mappedInput.wasHomeGesture()) {
    activityManager.goHome();
    return;
  }

  // The reading view closes before the RenderLock gate so Back stays
  // responsive during its slow full refresh: Back (X4), a tap or back swipe, or
  // a side key on touch boards, which have no front Back button.
  if (showingQr) {
    int tx = 0;
    int ty = 0;
    const bool close = backReleased || mappedInput.wasScreenTapped(tx, ty) ||
                       (mappedInput.hasTouch() &&
                        (mappedInput.wasPressed(Button::NavNext) || mappedInput.wasPressed(Button::NavPrevious)));
    if (close) {
      showingQr = false;
      cleanRefresh = true;
      requestUpdate();
    }
    return;
  }

  if (backReleased) {
    activityManager.goToApps(AppId::Dogear);
    return;
  }
  if (!loaded || screenCount == 0) return;
  if (RenderLock::peek()) return;  // one gesture, one page turn

  const bool tappedAction = actionTapValid && mappedInput.wasTapInRect(actionX, actionY, actionW, actionH);
  if (currentHasAction() && (confirmReleased || tappedAction)) {
    showingQr = true;
    requestUpdate();
    return;
  }

  const bool onFinal = screens[index].kind == Kind::Final;
  const auto touch = ReaderUtils::detectTouchPageTurn(renderer, mappedInput);
  // Confirm, or a tap on NEXT ›, also turns the cover.
  const bool coverNext = screens[index].kind == Kind::Cover && (confirmReleased || tappedAction);
  if (mappedInput.wasPressed(Button::NavNext) || touch.next || coverNext) {
    index = dogear_pages::nextIndex(index, screenCount);
    cleanRefresh = cleanRefresh || onFinal;
    requestUpdate();
    return;
  }
  if (mappedInput.wasPressed(Button::NavPrevious) || touch.prev) {
    index = dogear_pages::prevIndex(index, screenCount);
    cleanRefresh = cleanRefresh || onFinal;
    requestUpdate();
  }
}

// ---- layout helpers -----------------------------------------------------------

std::vector<std::string> DogearActivity::wrap(int font, const std::string& text, int maxLines,
                                              EpdFontFamily::Style style) const {
  return renderer.wrappedText(font, text.c_str(), renderer.getScreenWidth() - 2 * kMargin, maxLines, style);
}

std::vector<std::string> DogearActivity::coverTitle(int font, const std::string& text, int maxLines,
                                                    EpdFontFamily::Style style) const {
  const int width = renderer.getScreenWidth() - 2 * kMargin;
  return dogear_text::ellipsizeAtWord(
      wrap(font, text, kMaxWrapLines, style), static_cast<size_t>(maxLines),
      [&](const std::string& s) { return renderer.getTextWidth(font, s.c_str(), style) <= width; });
}

int DogearActivity::headBottom() const {
  return kTopSafe + renderer.getLineHeight(kMeta) + kHeadRuleGap + 1 + kRuleContentGap;
}

int DogearActivity::pagerTop() const {
  return renderer.getScreenHeight() - kBottomSafe - renderer.getLineHeight(kMeta);
}

int DogearActivity::bodyBottom() const { return pagerTop() - kRulePagerGap - kBodyFooterGap; }

int DogearActivity::continuationTop() const {
  return headBottom() + renderer.getLineHeight(kSerif12) + kHeadlineBodyGap;
}

// Centered on one line at the first font that fits; else the last font,
// balanced over two lines. Returns the y below the text.
int DogearActivity::centeredFit(int y, const char* text, const int* fonts, int fontCount,
                                EpdFontFamily::Style style) const {
  const int width = renderer.getScreenWidth() - 2 * kMargin;
  for (int i = 0; i < fontCount; ++i) {
    if (renderer.getTextWidth(fonts[i], text, style) <= width) {
      renderer.drawCenteredText(fonts[i], y, text, true, style);
      return y + renderer.getLineHeight(fonts[i]);
    }
  }
  const int font = fonts[fontCount - 1];
  const std::string s(text);
  size_t best = std::string::npos;
  int bestDiff = 0;
  for (size_t pos = s.find(' '); pos != std::string::npos; pos = s.find(' ', pos + 1)) {
    const int a = renderer.getTextWidth(font, s.substr(0, pos).c_str(), style);
    const int b = renderer.getTextWidth(font, s.substr(pos + 1).c_str(), style);
    if (a <= width && b <= width && (best == std::string::npos || std::abs(a - b) < bestDiff)) {
      best = pos;
      bestDiff = std::abs(a - b);
    }
  }
  const int lh = renderer.getLineHeight(font);
  if (best == std::string::npos) {
    for (const auto& line : wrap(font, s, 3, style)) {
      renderer.drawCenteredText(font, y, line.c_str(), true, style);
      y += lh;
    }
    return y;
  }
  renderer.drawCenteredText(font, y, s.substr(0, best).c_str(), true, style);
  renderer.drawCenteredText(font, y + lh, s.substr(best + 1).c_str(), true, style);
  return y + 2 * lh;
}

// A centered QR with an integer module size and a 4-module quiet zone inside
// `maxSide`. Returns the height used (quiet zone included).
int DogearActivity::drawQr(int top, int maxSide, const std::string& url) const {
  const int version = QrUtils::selectQrVersionEccLow(url.size());
  if (version == 0 || !dogear_limits::isQrUrl(url)) {
    renderer.drawCenteredText(kSans14, top + maxSide / 2, tr(STR_GOTO_LINK_UNAVAILABLE));
    return maxSide;
  }
  const int modules = 4 * version + 17;
  const int scale = std::max(1, maxSide / (modules + 2 * kQrQuietModules));
  const int inner = modules * scale;
  const int x = (renderer.getScreenWidth() - inner) / 2;
  QrUtils::drawQrCode(renderer, Rect(x, top + kQrQuietModules * scale, inner, inner), url);
  return inner + 2 * kQrQuietModules * scale;
}

// ---- screens ------------------------------------------------------------------

void DogearActivity::drawRunningHead() const {
  const int right = renderer.getScreenWidth() - kMargin;
  const int width = right - kMargin;
  const int labelW = renderer.getTextAdvanceX(kMeta, issue.label.c_str(), EpdFontFamily::BOLD, 2);
  renderer.drawText(kMeta, kMargin, kTopSafe, issue.label.c_str(), true, EpdFontFamily::BOLD,
                    BidiUtils::BidiBaseDir::AUTO, 2);
  const int room = width - labelW - 16;
  const std::string& line =
      renderer.getTextWidth(kMeta, issue.dateline.c_str()) <= room ? issue.dateline : issue.datelineShort;
  renderer.drawText(kMeta, right - renderer.getTextWidth(kMeta, line.c_str()), kTopSafe, line.c_str(), true);
  fillRule(renderer, kTopSafe + renderer.getLineHeight(kMeta) + kHeadRuleGap, 1, kMargin, right);
}

// Footer: rule, the page number (or « BACK) on the left, an action on the
// right. Records the action's touch target. A cached issue's marker follows
// the page number ("2 / 8 · CACHED"), so it never meets the action.
void DogearActivity::drawFooter(const char* left, bool leftBold, const char* right) {
  const int screenRight = renderer.getScreenWidth() - kMargin;
  const int y = pagerTop();
  fillRule(renderer, y - kRulePagerGap, 1, kMargin, screenRight);
  const char* marker = !showingQr && origin == DogearOrigin::CacheStale ? "CACHED" : nullptr;
  const EpdFontFamily::Style leftStyle = leftBold ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
  const int rightX = right ? screenRight - renderer.getTextWidth(kMeta, right, EpdFontFamily::BOLD) : screenRight;
  char leftText[48];
  snprintf(leftText, sizeof(leftText), "%s", left);
  if (marker) {
    char marked[48];
    snprintf(marked, sizeof(marked), "%s · %s", left, marker);
    // The generator's actions always leave room; an unexpected label keeps the action whole.
    if (kMargin + renderer.getTextWidth(kMeta, marked, leftStyle) + 24 <= rightX) {
      snprintf(leftText, sizeof(leftText), "%s", marked);
    }
  }
  renderer.drawText(kMeta, kMargin, y, leftText, true, leftStyle);
  if (right) {
    renderer.drawText(kMeta, rightX, y, right, true, EpdFontFamily::BOLD);
    actionX = rightX - 12;
    actionY = y - kRulePagerGap;
    actionW = renderer.getScreenWidth() - actionX;
    actionH = renderer.getScreenHeight() - actionY;
    actionTapValid = true;
  }
}

void DogearActivity::drawCover() {
  const int width = renderer.getScreenWidth() - 2 * kMargin;
  const int metaLH = renderer.getLineHeight(kMeta);
  int y = kTopSafe;
  renderer.drawText(kSans18, kMargin, y, issue.label.c_str(), true, EpdFontFamily::BOLD, BidiUtils::BidiBaseDir::AUTO,
                    6);
  y += renderer.getLineHeight(kSans18) + 2;
  renderer.drawText(kSerif12, kMargin, y, issue.strapline.c_str(), true, EpdFontFamily::ITALIC);
  y += renderer.getLineHeight(kSerif12) + 12;
  fillRule(renderer, y, kFeaturedRule, kMargin, kMargin + width);
  y += kFeaturedRule + 14;

  // The dateline at the largest size that fits; the short form for long weeks.
  const std::string* lines[] = {&issue.dateline, &issue.dateline, &issue.datelineShort, &issue.datelineShort};
  const int sizes[] = {kSans16, kSans14, kSans16, kSans14};
  int pick = 3;
  for (int i = 0; i < 4; ++i) {
    if (renderer.getTextWidth(sizes[i], lines[i]->c_str(), EpdFontFamily::BOLD) <= width) {
      pick = i;
      break;
    }
  }
  renderer.drawText(sizes[pick], kMargin, y, lines[pick]->c_str(), true, EpdFontFamily::BOLD);
  y += renderer.getLineHeight(sizes[pick]) + kCoverDatelineGap;
  if (issue.quiet) {
    drawQuietContents(y);
    return;
  }

  // Contents: the lead as kicker + large title, then each other item as a
  // small regular label ("24 NOV · BORN") close over its title.
  struct Plan {
    int featFont, rowFont, rowGap;
  };
  constexpr Plan kPlans[] = {
      {kSerif18, kSerif14, 6}, {kSerif16, kSerif14, 4}, {kSerif16, kSerif14, 2}, {kSerif16, kSerif12, 6}};
  Plan plan = kPlans[3];
  for (const Plan& p : kPlans) {
    int h = 0;
    bool anyRow = false;
    for (size_t i = 0; i < issue.entries.size(); ++i) {
      const auto& e = issue.entries[i];
      if (e.featured) {
        h += static_cast<int>(layouts[i].kicker.size()) * metaLH + 4 +
             static_cast<int>(coverTitle(p.featFont, e.title, 2, EpdFontFamily::BOLD).size()) *
                 renderer.getLineHeight(p.featFont) +
             kCoverLeadGap;
      } else {
        anyRow = true;
        h += kContentsLabelStep +
             static_cast<int>(coverTitle(p.rowFont, e.title, 2).size()) * renderer.getLineHeight(p.rowFont) + p.rowGap;
      }
    }
    if (anyRow) h += kCoverRuleGap + metaLH + kCoverHeadGap;
    plan = p;
    if (y + h <= bodyBottom()) break;
  }

  bool anyRow = false;
  for (size_t i = 0; i < issue.entries.size(); ++i) {
    const auto& e = issue.entries[i];
    if (!e.featured) {
      anyRow = true;
      continue;
    }
    for (const auto& line : layouts[i].kicker) {
      renderer.drawText(kMeta, kMargin, y, line.c_str(), true, EpdFontFamily::BOLD);
      y += metaLH;
    }
    y += 4;
    for (const auto& line : coverTitle(plan.featFont, e.title, 2, EpdFontFamily::BOLD)) {
      renderer.drawText(plan.featFont, kMargin, y, line.c_str(), true, EpdFontFamily::BOLD);
      y += renderer.getLineHeight(plan.featFont);
    }
    y += kCoverLeadGap;
  }
  if (!anyRow) return;
  fillRule(renderer, y, 1, kMargin, kMargin + width);
  y += kCoverRuleGap;
  renderer.drawText(kMeta, kMargin, y, tr(STR_DOGEAR_ALSO_THIS_WEEK), true, EpdFontFamily::BOLD,
                    BidiUtils::BidiBaseDir::AUTO, 1);
  y += metaLH + kCoverHeadGap;
  const int rowLH = renderer.getLineHeight(plan.rowFont);
  for (const auto& e : issue.entries) {
    if (e.featured) continue;
    const std::string label = upperAscii(e.dateLabel) + " · " + (e.contentsLabel.empty() ? e.kicker : e.contentsLabel);
    renderer.drawText(kMeta, kMargin, y, renderer.truncatedText(kMeta, label.c_str(), width).c_str(), true);
    y += kContentsLabelStep;
    for (const auto& line : coverTitle(plan.rowFont, e.title, 2)) {
      renderer.drawText(plan.rowFont, kMargin, y, line.c_str(), true);
      y += rowLH;
    }
    y += plan.rowGap;
  }
}

// A quiet week (Quiet A, owner-approved): the issue's quiet heading and note where
// the lead would start, a rule, then the lead in its usual treatment and the other
// picks as contents rows with their original dates. No ALSO THIS WEEK head: a quiet
// issue has none. Size plans: the regular cover's, then two quiet-only last resorts,
// tighter rows and then one-line (ellipsized) supporting titles. The lead's kicker
// and title keep the regular cover's limits, spacing is never tightened, and every
// plan keeps a clean bottom margin. If even the last plan cannot, the cover lists as
// many supporting rows as keep that margin; the rest stay full items with their own
// pages (the cover is not a complete list). Same as draw_quiet_contents,
// QUIET_LADDER and QUIET_MIN_MARGIN in dogear's mock/render_dogear.py.
void DogearActivity::drawQuietContents(int y) {
  constexpr int kQuietMinMargin = 12;
  const int width = renderer.getScreenWidth() - 2 * kMargin;
  const int metaLH = renderer.getLineHeight(kMeta);
  renderer.drawText(kSans14, kMargin, y, issue.quietHeading.c_str(), true, EpdFontFamily::BOLD,
                    BidiUtils::BidiBaseDir::AUTO, 2);
  y += renderer.getLineHeight(kSans14) + 2;
  for (const auto& line : wrap(kSerif12, issue.quietNote, 4, EpdFontFamily::ITALIC)) {
    renderer.drawText(kSerif12, kMargin, y, line.c_str(), true, EpdFontFamily::ITALIC);
    y += renderer.getLineHeight(kSerif12);
  }
  y += 14;
  fillRule(renderer, y, 1, kMargin, kMargin + width);
  y += kCoverRuleGap + 4;

  struct Plan {
    int featFont, rowFont, rowGap, rowLines;
  };
  constexpr Plan kPlans[] = {{kSerif18, kSerif14, 6, 2}, {kSerif16, kSerif14, 4, 2}, {kSerif16, kSerif14, 2, 2},
                             {kSerif16, kSerif12, 6, 2}, {kSerif16, kSerif12, 2, 2}, {kSerif16, kSerif12, 2, 1}};
  const int limit = bodyBottom() - kQuietMinMargin;
  Plan plan = kPlans[5];
  std::vector<int> rowH;
  int leadH = 0;
  bool fits = false;
  for (const Plan& p : kPlans) {
    leadH = 0;
    rowH.clear();
    for (size_t i = 0; i < issue.entries.size(); ++i) {
      const auto& e = issue.entries[i];
      if (e.featured) {
        leadH += static_cast<int>(layouts[i].kicker.size()) * metaLH + 4 +
                 static_cast<int>(coverTitle(p.featFont, e.title, 2, EpdFontFamily::BOLD).size()) *
                     renderer.getLineHeight(p.featFont) +
                 kCoverLeadGap;
      } else {
        rowH.push_back(kContentsLabelStep +
                       static_cast<int>(coverTitle(p.rowFont, e.title, p.rowLines).size()) *
                           renderer.getLineHeight(p.rowFont) +
                       p.rowGap);
      }
    }
    plan = p;
    int h = leadH;
    for (const int r : rowH) h += r;
    if (y + h <= limit) {
      fits = true;
      break;
    }
  }
  // Still too tall on the last plan: list the leading rows that keep the margin.
  size_t shown = rowH.size();
  if (!fits) {
    int h = leadH;
    shown = 0;
    while (shown < rowH.size() && y + h + rowH[shown] <= limit) h += rowH[shown++];
  }

  for (size_t i = 0; i < issue.entries.size(); ++i) {
    const auto& e = issue.entries[i];
    if (!e.featured) continue;
    for (const auto& line : layouts[i].kicker) {
      renderer.drawText(kMeta, kMargin, y, line.c_str(), true, EpdFontFamily::BOLD);
      y += metaLH;
    }
    y += 4;
    for (const auto& line : coverTitle(plan.featFont, e.title, 2, EpdFontFamily::BOLD)) {
      renderer.drawText(plan.featFont, kMargin, y, line.c_str(), true, EpdFontFamily::BOLD);
      y += renderer.getLineHeight(plan.featFont);
    }
    y += kCoverLeadGap;
  }
  const int rowLH = renderer.getLineHeight(plan.rowFont);
  size_t listed = 0;
  for (const auto& e : issue.entries) {
    if (e.featured) continue;
    if (listed++ == shown) break;
    const std::string label = upperAscii(e.dateLabel) + " · " + (e.contentsLabel.empty() ? e.kicker : e.contentsLabel);
    renderer.drawText(kMeta, kMargin, y, renderer.truncatedText(kMeta, label.c_str(), width).c_str(), true);
    y += kContentsLabelStep;
    for (const auto& line : coverTitle(plan.rowFont, e.title, plan.rowLines)) {
      renderer.drawText(plan.rowFont, kMargin, y, line.c_str(), true);
      y += rowLH;
    }
    y += plan.rowGap;
  }
}

void DogearActivity::drawItem(const Screen& s) {
  const DogearEntry& e = issue.entries[s.entry];
  const ItemLayout& lay = layouts[s.entry];
  int y = headBottom();
  if (s.part > 0) {
    // Quiet continuation: the item's title in a reduced header, nothing else.
    renderer.drawText(kSerif12, kMargin, y, e.title.c_str(), true, EpdFontFamily::ITALIC);
    y = continuationTop();
  } else {
    if (e.featured) {
      fillRule(renderer, y, kFeaturedRule, kMargin, renderer.getScreenWidth() - kMargin);
      y += kFeaturedRule + kFeaturedRuleGap;
    }
    for (const auto& line : lay.kicker) {
      renderer.drawText(kMeta, kMargin, y, line.c_str(), true, EpdFontFamily::BOLD);
      y += renderer.getLineHeight(kMeta);
    }
    y += kKickerHeadlineGap;
    for (const auto& line : lay.headline) {
      renderer.drawText(lay.headlineFont, kMargin, y, line.c_str(), true, EpdFontFamily::BOLD);
      y += renderer.getLineHeight(lay.headlineFont);
    }
    y += e.featured ? kFeaturedHeadlineBodyGap : kHeadlineBodyGap;
  }
  const int lh = renderer.getLineHeight(lay.bodyFont);
  for (const auto& line : lay.parts[s.part]) {
    renderer.drawText(lay.bodyFont, kMargin, y, line.c_str(), true);
    y += lh;
  }
}

void DogearActivity::drawFinal() {
  int y = headBottom() + 10;
  constexpr int kTitleFonts[] = {kSans16, kSans14, kMeta};
  y = centeredFit(y, tr(STR_DOGEAR_EVERYTHING_HERE), kTitleFonts, 3, EpdFontFamily::BOLD);
  y += 12;
  y += drawQr(y, kFinalQrMax, issue.editionUrl) + 10;
  constexpr int kMetaOnly[] = {kMeta};
  y = centeredFit(y, tr(STR_DOGEAR_SCAN_EDITION), kMetaOnly, 1, EpdFontFamily::BOLD);
  constexpr int kNoteFont[] = {kSerif12};
  y = centeredFit(y + 2, tr(STR_DOGEAR_ONLINE_NOTE), kNoteFont, 1, EpdFontFamily::ITALIC);
  y += 22;
  const int mid = renderer.getScreenWidth() / 2;
  fillRule(renderer, y, 1, mid - 40, mid + 40);
  y += 20;
  renderer.drawCenteredText(kMeta, y, tr(STR_DOGEAR_NEXT_ISSUE), true, EpdFontFamily::BOLD);
  y += renderer.getLineHeight(kMeta);
  constexpr int kNextFonts[] = {kSerif16, kSerif14};
  centeredFit(y + 2, issue.nextIssue.c_str(), kNextFonts, 2, EpdFontFamily::BOLD);
}

// The secondary reading view: which work opens, by whom, and where.
void DogearActivity::drawReadingQr(const DogearEntry& e) {
  const DogearCta& cta = e.cta;
  int y = headBottom();
  renderer.drawText(kMeta, kMargin, y, cta.label.c_str(), true, EpdFontFamily::BOLD, BidiUtils::BidiBaseDir::AUTO, 1);
  y += renderer.getLineHeight(kMeta) + kKickerHeadlineGap;
  for (const auto& line : wrap(kSerif16, cta.title, 2, EpdFontFamily::BOLD)) {
    renderer.drawText(kSerif16, kMargin, y, line.c_str(), true, EpdFontFamily::BOLD);
    y += renderer.getLineHeight(kSerif16);
  }
  if (!cta.author.empty()) {
    renderer.drawText(
        kSerif14, kMargin, y,
        renderer.truncatedText(kSerif14, cta.author.c_str(), renderer.getScreenWidth() - 2 * kMargin).c_str(), true);
    y += renderer.getLineHeight(kSerif14);
  }
  std::string edition = cta.edition;
  if (!edition.empty()) edition[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(edition[0])));
  const std::string notes[] = {edition, cta.representative ? std::string(tr(STR_DOGEAR_REPRESENTATIVE)) : ""};
  for (const auto& note : notes) {
    if (note.empty()) continue;
    for (const auto& line : wrap(kSerif12, note, 2, EpdFontFamily::ITALIC)) {
      renderer.drawText(kSerif12, kMargin, y, line.c_str(), true, EpdFontFamily::ITALIC);
      y += renderer.getLineHeight(kSerif12);
    }
  }
  y += 20;
  const int room = bodyBottom() - 20 - renderer.getLineHeight(kMeta) - 12 - y;
  y += drawQr(y, std::min(renderer.getScreenWidth() - 2 * kMargin, room), cta.url) + 12;
  if (!cta.note.empty()) renderer.drawCenteredText(kMeta, y, cta.note.c_str());
}

// A status screen (no readable issue anywhere): the issue masthead with
// DOGEAR's own label and strapline, the heading and note in the Quiet A cover's
// treatment, and a footer with only BACK. No pager and no marker.
void DogearActivity::drawStatus() {
  StrId heading = StrId::STR_DOGEAR_CANT_LOAD;
  StrId note = StrId::STR_DOGEAR_CANT_LOAD_NOTE;
  dogear_state::statusCopy(screen, heading, note);
  const int width = renderer.getScreenWidth() - 2 * kMargin;
  int y = kTopSafe;
  renderer.drawText(kSans18, kMargin, y, tr(STR_DOGEAR), true, EpdFontFamily::BOLD, BidiUtils::BidiBaseDir::AUTO, 6);
  y += renderer.getLineHeight(kSans18) + 2;
  renderer.drawText(kSerif12, kMargin, y, tr(STR_DOGEAR_STRAPLINE), true, EpdFontFamily::ITALIC);
  y += renderer.getLineHeight(kSerif12) + 12;
  fillRule(renderer, y, kFeaturedRule, kMargin, kMargin + width);
  y += kFeaturedRule + 14;
  renderer.drawText(kSans14, kMargin, y, I18n::getInstance().get(heading), true, EpdFontFamily::BOLD,
                    BidiUtils::BidiBaseDir::AUTO, 2);
  y += renderer.getLineHeight(kSans14) + 2;
  for (const auto& line : wrap(kSerif12, I18n::getInstance().get(note), 4, EpdFontFamily::ITALIC)) {
    renderer.drawText(kSerif12, kMargin, y, line.c_str(), true, EpdFontFamily::ITALIC);
    y += renderer.getLineHeight(kSerif12);
  }
  const std::string back = upperAscii(tr(STR_BACK));
  drawFooter(back.c_str(), true, nullptr);
}

void DogearActivity::render(RenderLock&&) {
  actionTapValid = false;
  renderer.clearScreen();

  if (!loaded || screenCount == 0) {
    drawStatus();
    renderer.displayBuffer(firstRender ? HalDisplay::HALF_REFRESH : HalDisplay::FAST_REFRESH);
    firstRender = false;
    return;
  }

  const Screen& s = screens[index];
  char pager[16];
  snprintf(pager, sizeof(pager), "%d / %d", s.number, dogear_pages::pageCount(static_cast<int>(issue.entries.size())));

  if (showingQr && currentHasAction()) {
    drawRunningHead();
    drawReadingQr(issue.entries[s.entry]);
    const std::string back = upperAscii(tr(STR_BACK));
    drawFooter(back.c_str(), true, nullptr);
    // A dense QR scans best off a complete waveform.
    renderer.displayBuffer(HalDisplay::FULL_REFRESH);
    return;
  }

  char action[48] = "";
  switch (s.kind) {
    case Kind::Cover:
      drawCover();
      snprintf(action, sizeof(action), "%s ›", tr(STR_DOGEAR_NEXT));
      break;
    case Kind::Item:
      drawRunningHead();
      drawItem(s);
      if (currentHasAction()) snprintf(action, sizeof(action), "%s ›", issue.entries[s.entry].cta.label.c_str());
      break;
    case Kind::Final:
      drawRunningHead();
      drawFinal();
      break;
  }
  drawFooter(pager, false, action[0] ? action : nullptr);

  HalDisplay::RefreshMode mode = HalDisplay::FAST_REFRESH;
  if (s.kind == Kind::Final) {
    mode = HalDisplay::FULL_REFRESH;  // the edition QR, like GOTO's terminal page
  } else if (cleanRefresh || firstRender) {
    mode = HalDisplay::HALF_REFRESH;  // scrub ghosting after a QR or on entry
  }
  cleanRefresh = false;
  firstRender = false;
  renderer.displayBuffer(mode);
}

#endif  // DOGEAR_ENABLED
