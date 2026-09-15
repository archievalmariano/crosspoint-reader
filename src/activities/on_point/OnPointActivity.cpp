#include "OnPointActivity.h"

#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>

#include "MappedInputManager.h"
#include "OnPointScheduleData.h"
#include "fontIds.h"

namespace {

constexpr int MARGIN = 24;
constexpr int TOP = 18;
constexpr int HEADER_FONT = NOTOSANS_16_FONT_ID;
constexpr int BODY_FONT = NOTOSANS_14_FONT_ID;
constexpr int META_FONT = NOTOSANS_12_FONT_ID;
constexpr int ROUTE_BAND_TOP = 58;
constexpr int ROUTE_BAND_HEIGHT = 124;
constexpr uint8_t HALF_REFRESH_AFTER_FAST = 6;

// Three columns by five rows. Rendering these as solid blocks produces a
// display-sized numeric face without adding another font asset.
constexpr uint8_t DIGITS[10][5] = {
    {0b111, 0b101, 0b101, 0b101, 0b111}, {0b010, 0b110, 0b010, 0b010, 0b111}, {0b111, 0b001, 0b111, 0b100, 0b111},
    {0b111, 0b001, 0b111, 0b001, 0b111}, {0b101, 0b101, 0b111, 0b001, 0b001}, {0b111, 0b100, 0b111, 0b001, 0b111},
    {0b111, 0b100, 0b111, 0b101, 0b111}, {0b111, 0b001, 0b010, 0b010, 0b010}, {0b111, 0b101, 0b111, 0b101, 0b111},
    {0b111, 0b101, 0b111, 0b001, 0b111},
};

void formatTime(const on_point::DepartureOccurrence& departure, char* buffer, const size_t size) {
  if (!departure.valid) {
    if (size > 0) buffer[0] = '\0';
    return;
  }
  snprintf(buffer, size, "%02u:%02u", departure.local.hour, departure.local.minute);
}

void formatClock(const on_point::CivilDateTime& dateTime, char* buffer, const size_t size) {
  snprintf(buffer, size, "%02u:%02u", dateTime.hour, dateTime.minute);
}

void formatDate(const on_point::CivilDateTime& dateTime, char* buffer, const size_t size) {
  snprintf(buffer, size, "%04d-%02u-%02u", dateTime.year, dateTime.month, dateTime.day);
}

bool sameDate(const on_point::CivilDateTime& a, const on_point::CivilDateTime& b) {
  return a.year == b.year && a.month == b.month && a.day == b.day;
}

}  // namespace

void OnPointActivity::onEnter() {
  Activity::onEnter();
  scheduleIndex = 0;
  cleanRefresh = true;
  fastRefreshCount = 0;
  updateState();
  requestUpdate();
}

void OnPointActivity::updateState() {
  on_point::CivilDateTime nowUtc;
  clockReady = clock.nowUtc(nowUtc);
  state = clockReady ? on_point::getDepartureState(on_point::scheduleAt(scheduleIndex), nowUtc)
                     : on_point::DepartureState{};
  scheduleNextRefresh();
}

void OnPointActivity::scheduleNextRefresh() {
  unsigned long delaySeconds = 60;
  if (!clockReady || !state.next.valid) {
    delaySeconds = 60;
  } else if (state.minutesToNext == 0) {
    // The exact HH:MM:00 state belongs to that departure. Re-evaluate one
    // second later so the next scheduled departure replaces it immediately.
    delaySeconds = 1;
  } else {
    int stepMinutes = 1;
    if (state.minutesToNext > 30) {
      stepMinutes = static_cast<int>(std::min<int32_t>(10, state.minutesToNext - 30));
    } else if (state.minutesToNext > 10) {
      stepMinutes = static_cast<int>(std::min<int32_t>(5, state.minutesToNext - 10));
    }
    delaySeconds = static_cast<unsigned long>(stepMinutes * 60 - state.localNow.second);
    if (delaySeconds == 0) delaySeconds = 1;
  }
  nextRefreshAtMs = millis() + delaySeconds * 1000UL;
}

void OnPointActivity::loop() {
  using Button = MappedInputManager::Button;

  if (mappedInput.wasReleased(Button::Back)) {
    activityManager.goHome(HomeMenuItem::ON_POINT);
    return;
  }

  if (RenderLock::peek()) return;

  if (mappedInput.wasPressed(Button::NavNext) || mappedInput.wasPressed(Button::NavPrevious)) {
    scheduleIndex = (scheduleIndex + 1) % on_point::SCHEDULE_COUNT;
    cleanRefresh = true;
    updateState();
    requestUpdate();
    return;
  }

  if (static_cast<long>(millis() - nextRefreshAtMs) >= 0) {
    updateState();
    requestUpdate();
  }
}

void OnPointActivity::drawLargeNumber(const int value, const int x, const int y, const int maxWidth,
                                      const int height) const {
  char text[12];
  snprintf(text, sizeof(text), "%d", std::max(0, value));
  int digitCount = 0;
  while (text[digitCount] != '\0') ++digitCount;
  if (digitCount == 0) return;

  const int cellFromHeight = std::max(2, height / 5);
  const int cellFromWidth = std::max(2, maxWidth / (digitCount * 4 - 1));
  const int cell = std::min(cellFromHeight, cellFromWidth);
  const int digitWidth = cell * 3;
  const int gap = cell;
  const int totalWidth = digitCount * digitWidth + (digitCount - 1) * gap;
  int drawX = x + (maxWidth - totalWidth) / 2;

  for (int digitIndex = 0; digitIndex < digitCount; ++digitIndex) {
    const int digit = text[digitIndex] - '0';
    for (int row = 0; row < 5; ++row) {
      for (int col = 0; col < 3; ++col) {
        if ((DIGITS[digit][row] & (1u << (2 - col))) != 0)
          renderer.fillRect(drawX + col * cell, y + row * cell, cell - 2, cell - 2, true);
      }
    }
    drawX += digitWidth + gap;
  }
}

void OnPointActivity::drawHourglass(const int x, const int y, const int width, const int height) const {
  const int centerX = x + width / 2;
  const int centerY = y + height / 2;
  const int bottom = y + height;
  renderer.drawLine(x, y, x + width, y, 3, true);
  renderer.drawLine(x, y, centerX, centerY, 3, true);
  renderer.drawLine(x + width, y, centerX, centerY, 3, true);
  renderer.drawLine(centerX, centerY, x, bottom, 3, true);
  renderer.drawLine(centerX, centerY, x + width, bottom, 3, true);
  renderer.drawLine(x, bottom, x + width, bottom, 3, true);

  if (state.intervalMinutes == 0) return;
  const int remaining =
      static_cast<int>(std::clamp<int32_t>(state.minutesToNext, 0, static_cast<int32_t>(state.intervalMinutes)));
  const int elapsed = static_cast<int>(state.intervalMinutes) - remaining;
  const int chamberHeight = height / 2 - 5;
  const int upperHeight = chamberHeight * remaining / state.intervalMinutes;
  const int lowerHeight = chamberHeight * elapsed / state.intervalMinutes;

  if (upperHeight > 0) {
    const int baseY = centerY - upperHeight;
    const int halfWidth = std::max(2, width * upperHeight / height);
    const int xs[] = {centerX, centerX - halfWidth, centerX + halfWidth};
    const int ys[] = {centerY - 3, baseY, baseY};
    renderer.fillPolygon(xs, ys, 3, true);
  }
  if (lowerHeight > 0) {
    const int topY = bottom - lowerHeight;
    const int halfWidth = std::max(2, width * lowerHeight / height);
    const int xs[] = {centerX, centerX - halfWidth, centerX + halfWidth};
    const int ys[] = {topY, bottom - 3, bottom - 3};
    renderer.fillPolygon(xs, ys, 3, true);
  }
}

void OnPointActivity::drawPrimaryScreen() {
  const int width = renderer.getScreenWidth();
  char departureTime[8];
  formatTime(state.next, departureTime, sizeof(departureTime));

  renderer.drawText(META_FONT, MARGIN, 204, tr(STR_ON_POINT_SCHEDULED), true, EpdFontFamily::BOLD);
  drawLargeNumber(state.minutesToNext, MARGIN, 236, width - 168, 180);
  renderer.drawText(HEADER_FONT, width - 132, 360, tr(STR_ON_POINT_MIN), true, EpdFontFamily::BOLD);
  drawHourglass(width - 128, 222, 86, 122);

  const int departureY = 438;
  renderer.drawText(META_FONT, MARGIN, departureY, tr(STR_ON_POINT_SCHEDULED_DEPARTURE));
  renderer.drawText(HEADER_FONT, MARGIN, departureY + 25, departureTime, true, EpdFontFamily::BOLD);
  if (state.isLastDeparture)
    renderer.drawText(META_FONT, MARGIN + 96, departureY + 28, tr(STR_ON_POINT_LAST), true, EpdFontFamily::BOLD);

  const int ruleY = 512;
  renderer.drawLine(MARGIN, ruleY, width - MARGIN, ruleY, 2, true);
  size_t sameDayFollowing = 0;
  while (sameDayFollowing < state.followingCount && sameDate(state.following[sameDayFollowing].local, state.next.local))
    ++sameDayFollowing;
  renderer.drawText(META_FONT, MARGIN, ruleY + 18,
                    sameDayFollowing > 0 ? tr(STR_ON_POINT_FOLLOWING) : tr(STR_ON_POINT_NEXT_SERVICE), true,
                    EpdFontFamily::BOLD);
  int itemX = MARGIN;
  for (size_t i = 0; i < sameDayFollowing; ++i) {
    char time[8];
    formatTime(state.following[i], time, sizeof(time));
    renderer.drawText(BODY_FONT, itemX, ruleY + 48, time, true, EpdFontFamily::BOLD);
    if (state.following[i].isLast)
      renderer.drawText(META_FONT, itemX, ruleY + 75, tr(STR_ON_POINT_LAST), true, EpdFontFamily::BOLD);
    itemX += 116;
  }
  if (sameDayFollowing == 0 && state.followingCount > 0) {
    char time[8];
    char date[16];
    char nextService[32];
    formatTime(state.following[0], time, sizeof(time));
    formatDate(state.following[0].local, date, sizeof(date));
    snprintf(nextService, sizeof(nextService), "%s  %s", date, time);
    renderer.drawText(BODY_FONT, MARGIN, ruleY + 48, nextService, true, EpdFontFamily::BOLD);
  }

  char first[8];
  char last[8];
  formatTime(state.first, first, sizeof(first));
  formatTime(state.last, last, sizeof(last));
  char serviceSpan[64];
  snprintf(serviceSpan, sizeof(serviceSpan), "%s %s    %s %s", tr(STR_ON_POINT_FIRST), first, tr(STR_ON_POINT_LAST),
           last);
  renderer.drawText(META_FONT, MARGIN, 636, serviceSpan);
  if (state.minutesToLast >= 0) {
    char finalService[64];
    snprintf(finalService, sizeof(finalService), "%s %ld %s", tr(STR_ON_POINT_FINAL_SERVICE_IN),
             static_cast<long>(state.minutesToLast), tr(STR_ON_POINT_MIN));
    renderer.drawText(META_FONT, MARGIN, 661, finalService, true, EpdFontFamily::BOLD);
  }
}

void OnPointActivity::drawEndedScreen() {
  const int width = renderer.getScreenWidth();
  const char* status = state.serviceStatus == on_point::ServiceStatus::NoServiceToday
                           ? tr(STR_ON_POINT_NO_SERVICE_TODAY)
                           : tr(STR_ON_POINT_SERVICE_ENDED);
  renderer.drawCenteredText(HEADER_FONT, 245, status, true, EpdFontFamily::BOLD);
  renderer.drawLine(MARGIN, 292, width - MARGIN, 292, 3, true);
  renderer.drawText(META_FONT, MARGIN, 332, tr(STR_ON_POINT_NEXT_SERVICE), true, EpdFontFamily::BOLD);

  if (state.next.valid) {
    char time[8];
    char date[16];
    formatTime(state.next, time, sizeof(time));
    formatDate(state.next.local, date, sizeof(date));
    renderer.drawText(HEADER_FONT, MARGIN, 370, time, true, EpdFontFamily::BOLD);
    renderer.drawText(BODY_FONT, MARGIN, 410, date, true, EpdFontFamily::BOLD);
    renderer.drawText(META_FONT, MARGIN, 455, tr(STR_ON_POINT_SCHEDULED_DEPARTURE));
  } else {
    renderer.drawText(BODY_FONT, MARGIN, 370, tr(STR_ON_POINT_NO_FUTURE_SERVICE), true, EpdFontFamily::BOLD);
  }
}

void OnPointActivity::render(RenderLock&&) {
  const int width = renderer.getScreenWidth();
  renderer.clearScreen();

  renderer.drawText(HEADER_FONT, MARGIN, TOP, tr(STR_ON_POINT), true, EpdFontFamily::BOLD);
  if (clockReady) {
    char now[8];
    formatClock(state.localNow, now, sizeof(now));
    renderer.drawText(META_FONT, width - MARGIN - renderer.getTextWidth(META_FONT, now), TOP + 3, now, true,
                      EpdFontFamily::BOLD);
  }

  renderer.fillRect(0, ROUTE_BAND_TOP, width, ROUTE_BAND_HEIGHT, true);
  const on_point::Schedule& schedule = on_point::scheduleAt(scheduleIndex);
  renderer.drawText(HEADER_FONT, MARGIN, ROUTE_BAND_TOP + 18, schedule.origin, false, EpdFontFamily::BOLD);
  renderer.drawLine(MARGIN + 8, ROUTE_BAND_TOP + 50, MARGIN + 8, ROUTE_BAND_TOP + 72, 3, false);
  renderer.drawLine(MARGIN + 3, ROUTE_BAND_TOP + 66, MARGIN + 8, ROUTE_BAND_TOP + 72, 3, false);
  renderer.drawLine(MARGIN + 13, ROUTE_BAND_TOP + 66, MARGIN + 8, ROUTE_BAND_TOP + 72, 3, false);
  renderer.drawText(HEADER_FONT, MARGIN + 28, ROUTE_BAND_TOP + 76, schedule.destination, false, EpdFontFamily::BOLD);
  renderer.drawText(META_FONT, width - MARGIN - renderer.getTextWidth(META_FONT, tr(STR_ON_POINT_PROVISIONAL)),
                    ROUTE_BAND_TOP + 88, tr(STR_ON_POINT_PROVISIONAL), false, EpdFontFamily::BOLD);

  if (!clockReady) {
    renderer.drawCenteredText(HEADER_FONT, 290, tr(STR_ON_POINT_CLOCK_UNAVAILABLE), true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(META_FONT, 340, tr(STR_ON_POINT_SYNC_CLOCK_HINT));
  } else if (state.serviceStatus == on_point::ServiceStatus::InvalidSchedule) {
    renderer.drawCenteredText(HEADER_FONT, 290, tr(STR_ON_POINT_TIMETABLE_UNAVAILABLE), true, EpdFontFamily::BOLD);
  } else if (state.serviceStatus == on_point::ServiceStatus::ServiceEnded ||
             state.serviceStatus == on_point::ServiceStatus::NoServiceToday ||
             state.serviceStatus == on_point::ServiceStatus::NoFutureService) {
    drawEndedScreen();
  } else {
    drawPrimaryScreen();
  }

  renderer.drawLine(MARGIN, 718, width - MARGIN, 718, true);
  renderer.drawText(META_FONT, MARGIN, 738, tr(STR_ON_POINT_SWITCH_DIRECTION), true, EpdFontFamily::BOLD);
  renderer.drawText(META_FONT, width - MARGIN - renderer.getTextWidth(META_FONT, tr(STR_HOME)), 738, tr(STR_HOME));

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
