#include "ClockActivity.h"

#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <I18n.h>

#include <cstdio>
#include <cstdlib>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "activities/utilities/SegmentDigits.h"
#include "activities/utilities/UtilityDates.h"
#include "fontIds.h"

namespace {
constexpr int MARGIN = 24;
constexpr int HEADER_TITLE_Y = 8;
constexpr int HEADER_META_Y = 20;
constexpr int HEADER_RULE_Y = 50;
constexpr int HEADER_FONT = NOTOSANS_16_FONT_ID;
constexpr int LABEL_FONT = UI_10_FONT_ID;
constexpr int TIME_TOP = 78;
constexpr int TIME_LINE_HEIGHT = 250;
constexpr int TIME_LINE_GAP = 24;
constexpr int SUFFIX_FONT = NOTOSANS_16_FONT_ID;
constexpr int DATE_RULE_GAP = 32;
constexpr int WEEKDAY_FONT = NOTOSANS_18_FONT_ID;
constexpr int DATE_FONT = NOTOSANS_16_FONT_ID;
// A cleaner refresh every ten minutes clears the ghosting fast refreshes leave.
constexpr uint8_t HALF_REFRESH_AFTER_FAST = 9;
// Redraw just after the minute turns, never before it.
constexpr unsigned long MINUTE_BOUNDARY_SLACK_MS = 250;
constexpr unsigned long RETRY_MS = 60000;
}  // namespace

void ClockActivity::onEnter() {
  Activity::onEnter();
  cleanRefresh = true;
  updateTime();
  requestUpdate();
}

void ClockActivity::updateTime() {
  clockReady = utility_dates::trustedLocalNow(now);
  nextRefreshAtMs = millis() + (clockReady ? (60UL - now.second) * 1000UL + MINUTE_BOUNDARY_SLACK_MS : RETRY_MS);
}

void ClockActivity::loop() {
  using Button = MappedInputManager::Button;
  if (mappedInput.wasHomeGesture() || mappedInput.wasHomeKeyHold()) {
    activityManager.goHome();
    return;
  }
  if (mappedInput.wasReleased(Button::Back)) {
    activityManager.goToApps(AppId::Clock);
    return;
  }
  if (static_cast<long>(millis() - nextRefreshAtMs) >= 0) {
    updateTime();
    requestUpdate();
  }
}

// Hours and minutes stack as two poster-sized lines: hours on the left edge,
// minutes on the right. Returns the bottom of the time block.
int ClockActivity::drawTime(const int top) const {
  const bool twelveHour = SETTINGS.clockFormat == 1;
  int hour = now.hour;
  const char* suffix = nullptr;
  if (twelveHour) {
    suffix = hour < 12 ? tr(STR_CLOCK_AM) : tr(STR_CLOCK_PM);
    hour = hour % 12 == 0 ? 12 : hour % 12;
  }
  char hours[4];
  char minutes[4];
  snprintf(hours, sizeof(hours), twelveHour ? "%d" : "%02d", hour);
  snprintf(minutes, sizeof(minutes), "%02u", static_cast<unsigned>(now.minute));

  const int width = renderer.getScreenWidth();
  int height = TIME_LINE_HEIGHT;
  while (height > 40 && segment_digits::textWidth("88", height) > width - 2 * MARGIN) height -= 4;

  segment_digits::draw(renderer, MARGIN, top, height, hours);
  if (suffix) {
    renderer.drawText(SUFFIX_FONT, MARGIN + segment_digits::textWidth(hours, height) + 16, top, suffix, true,
                      EpdFontFamily::BOLD);
  }
  const int minutesTop = top + height + TIME_LINE_GAP;
  segment_digits::draw(renderer, width - MARGIN - segment_digits::textWidth(minutes, height), minutesTop, height,
                       minutes);
  return minutesTop + height;
}

void ClockActivity::drawDate(const int top) const {
  const int width = renderer.getScreenWidth();
  renderer.drawLine(MARGIN, top, width - MARGIN, top, 5, true);
  renderer.drawText(WEEKDAY_FONT, MARGIN, top + 22, utility_dates::weekdayName(now.weekday), true, EpdFontFamily::BOLD);
  char date[40];
  snprintf(date, sizeof(date), "%u %s %d", static_cast<unsigned>(now.date.day),
           utility_dates::monthName(now.date.month), static_cast<int>(now.date.year));
  renderer.drawText(DATE_FONT, MARGIN, top + 66, date);
}

void ClockActivity::drawNotSet() const {
  renderer.fillRect(MARGIN, 246, 12, 150, true);
  renderer.drawText(NOTOSANS_18_FONT_ID, 64, 262, tr(STR_CLOCK_NOT_SET_TITLE), true, EpdFontFamily::BOLD);
  renderer.drawText(NOTOSANS_14_FONT_ID, 64, 318, tr(STR_CLOCK_SYNC_HINT));
}

void ClockActivity::render(RenderLock&&) {
  renderer.clearScreen();
  const int width = renderer.getScreenWidth();
  renderer.drawText(HEADER_FONT, MARGIN, HEADER_TITLE_Y, tr(STR_CLOCK), true, EpdFontFamily::BOLD);

  // Annotation: the offset local time is computed with, so a wrong Settings
  // offset is visible rather than silently shown as local time.
  const int offset = utility_dates::localOffsetMinutes();
  char zone[16];
  snprintf(zone, sizeof(zone), "%s%c%02d:%02d", tr(STR_CLOCK_UTC), offset < 0 ? '-' : '+', std::abs(offset) / 60,
           std::abs(offset) % 60);
  renderer.drawText(LABEL_FONT, width - MARGIN - renderer.getTextWidth(LABEL_FONT, zone, EpdFontFamily::BOLD),
                    HEADER_META_Y, zone, true, EpdFontFamily::BOLD);
  renderer.drawLine(MARGIN, HEADER_RULE_Y, width - MARGIN, HEADER_RULE_Y, 5, true);

  if (!clockReady) {
    drawNotSet();
  } else {
    drawDate(drawTime(TIME_TOP) + DATE_RULE_GAP);
  }

  HalDisplay::RefreshMode mode = HalDisplay::FAST_REFRESH;
  if (cleanRefresh || fastRefreshCount >= HALF_REFRESH_AFTER_FAST) {
    mode = HalDisplay::HALF_REFRESH;
    cleanRefresh = false;
    fastRefreshCount = 0;
  } else {
    ++fastRefreshCount;
  }
  renderer.displayBuffer(mode);
}
