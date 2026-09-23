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
constexpr int ROUTE_FONT = NOTOSANS_16_FONT_ID;
constexpr int TIME_FONT = NOTOSANS_18_FONT_ID;
constexpr int FOLLOWING_FONT = NOTOSANS_14_FONT_ID;
constexpr int LABEL_FONT = UI_10_FONT_ID;
constexpr int HEADER_RULE_Y = 50;
constexpr int ROUTE_ORIGIN_Y = 72;
constexpr int ROUTE_LINE_Y = 130;
constexpr int ROUTE_DESTINATION_Y = 145;
constexpr int ROUTE_TOUCH_TOP = 58;
constexpr int ROUTE_TOUCH_HEIGHT = 132;
constexpr int ROUTE_LIST_LABEL_Y = 76;
constexpr int ROUTE_LIST_ROW_TOP = 104;
constexpr int ROUTE_LIST_ROW_HEIGHT = 140;
constexpr int ROUTE_LIST_ROW_GAP = 6;
constexpr size_t ROUTE_LIST_VISIBLE_ROWS = 4;
constexpr int COUNTDOWN_LABEL_Y = 204;
constexpr int COUNTDOWN_Y = 252;
constexpr int COUNTDOWN_HEIGHT = 250;
constexpr int COUNTDOWN_MAX_WIDTH = 330;
constexpr int COUNTDOWN_UNIT_X = 354;
constexpr int COUNTDOWN_UNIT_Y = 476;
constexpr int HOURGLASS_X = 382;
constexpr int HOURGLASS_Y = 294;
constexpr int HOURGLASS_WIDTH = 62;
constexpr int HOURGLASS_HEIGHT = 96;
constexpr int HOURGLASS_STROKE = 6;
constexpr int LEDGER_RULE_Y = 536;
constexpr int FOOTER_RULE_Y = 726;
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

void formatDate(const on_point::CivilDateTime& dateTime, char* buffer, const size_t size) {
  snprintf(buffer, size, "%04d-%02u-%02u", dateTime.year, dateTime.month, dateTime.day);
}

bool sameDate(const on_point::CivilDateTime& a, const on_point::CivilDateTime& b) {
  return a.year == b.year && a.month == b.month && a.day == b.day;
}

}  // namespace

void OnPointActivity::onEnter() {
  Activity::onEnter();
  view = View::RouteList;
  routeIndex = 0;
  reversed = false;
  cleanRefresh = true;
  fastRefreshCount = 0;
  updateState();
  requestUpdate();
}

const on_point::Schedule& OnPointActivity::selectedSchedule() const {
  return on_point::scheduleForRoute(routeIndex, reversed);
}

size_t OnPointActivity::routeListFirstIndex() const {
  return routeIndex / ROUTE_LIST_VISIBLE_ROWS * ROUTE_LIST_VISIBLE_ROWS;
}

size_t OnPointActivity::routeListVisibleCount() const {
  return std::min(ROUTE_LIST_VISIBLE_ROWS, on_point::routeCount() - routeListFirstIndex());
}

void OnPointActivity::openSelectedRoute() {
  view = View::Departure;
  cleanRefresh = true;
  updateState();
  requestUpdate();
}

void OnPointActivity::reverseDirection() {
  reversed = !reversed;
  cleanRefresh = true;
  updateState();
  requestUpdate();
}

void OnPointActivity::updateState() {
  on_point::CivilDateTime nowUtc;
  clockReady = clock.nowUtc(nowUtc);
  state = clockReady ? on_point::getDepartureState(selectedSchedule(), nowUtc) : on_point::DepartureState{};
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
    if (view == View::Departure) {
      view = View::RouteList;
      cleanRefresh = true;
      requestUpdate();
      return;
    }
    activityManager.goHome(HomeMenuItem::ON_POINT);
    return;
  }

  if (RenderLock::peek()) return;

  if (view == View::RouteList) {
    if (mappedInput.wasPressed(Button::NavNext)) {
      ++routeIndex;
      if (routeIndex >= on_point::routeCount()) routeIndex = 0;
      reversed = false;
      cleanRefresh = true;
      updateState();
      requestUpdate();
      return;
    }
    if (mappedInput.wasPressed(Button::NavPrevious)) {
      routeIndex = routeIndex == 0 ? on_point::routeCount() - 1 : routeIndex - 1;
      reversed = false;
      cleanRefresh = true;
      updateState();
      requestUpdate();
      return;
    }
    if (mappedInput.wasReleased(Button::Confirm)) {
      openSelectedRoute();
      return;
    }

    int touchedRow = -1;
    const auto routeTouch = mappedInput.rowTouch(touchedRow, ROUTE_LIST_ROW_TOP,
                                                 ROUTE_LIST_ROW_HEIGHT + ROUTE_LIST_ROW_GAP,
                                                 static_cast<int>(routeListVisibleCount()), 0,
                                                 renderer.getScreenWidth(), ROUTE_LIST_ROW_HEIGHT);
    if (routeTouch != MappedInputManager::RowTouch::None) {
      routeIndex = routeListFirstIndex() + static_cast<size_t>(touchedRow);
      reversed = false;
      if (routeTouch == MappedInputManager::RowTouch::Tap) openSelectedRoute();
      return;
    }

    const auto swipe = mappedInput.wasSwipe();
    if (swipe == MappedInputManager::SwipeDir::Up || swipe == MappedInputManager::SwipeDir::Down) {
      const size_t first = routeListFirstIndex();
      if (swipe == MappedInputManager::SwipeDir::Up && first + ROUTE_LIST_VISIBLE_ROWS < on_point::routeCount()) {
        routeIndex = first + ROUTE_LIST_VISIBLE_ROWS;
      } else if (swipe == MappedInputManager::SwipeDir::Down && first >= ROUTE_LIST_VISIBLE_ROWS) {
        routeIndex = first - ROUTE_LIST_VISIBLE_ROWS;
      } else {
        return;
      }
      reversed = false;
      cleanRefresh = true;
      updateState();
      requestUpdate();
    }
    return;
  }

  if (mappedInput.wasPressed(Button::NavNext) || mappedInput.wasPressed(Button::NavPrevious) ||
      mappedInput.wasTapInRect(0, ROUTE_TOUCH_TOP, renderer.getScreenWidth(), ROUTE_TOUCH_HEIGHT)) {
    reverseDirection();
    return;
  }

  if (view == View::Departure && static_cast<long>(millis() - nextRefreshAtMs) >= 0) {
    updateState();
    requestUpdate();
  }
}

void OnPointActivity::drawLargeNumber(const int value, const int x, const int y, const int maxWidth,
                                      const int height) const {
  char number[12];
  const int safeValue = std::max(0, value);
  snprintf(number, sizeof(number), safeValue < 100 ? "%02d" : "%d", safeValue);

  int digitCount = 0;
  while (number[digitCount] != '\0') ++digitCount;
  if (digitCount == 0) return;

  // Each digit is 15 units wide and each gap is 2 units when expressed in
  // fifths of a cell, allowing three-digit pre-service waits to scale down.
  const int widthUnits = digitCount * 15 + (digitCount - 1) * 2;
  const int cellFromWidth = std::max(2, maxWidth * 5 / widthUnits);
  const int cell = std::min(std::max(2, height / 5), cellFromWidth);
  const int digitWidth = cell * 3;
  const int gap = std::max(4, cell * 2 / 5);
  const int totalWidth = digitCount * digitWidth + (digitCount - 1) * gap;
  int drawX = x + (maxWidth - totalWidth) / 2;

  for (int digitIndex = 0; digitIndex < digitCount; ++digitIndex) {
    const int digit = number[digitIndex] - '0';
    for (int row = 0; row < 5; ++row) {
      for (int col = 0; col < 3; ++col) {
        if ((DIGITS[digit][row] & (1u << (2 - col))) != 0)
          renderer.fillRect(drawX + col * cell, y + row * cell, cell - 3, cell - 3, true);
      }
    }
    drawX += digitWidth + gap;
  }
}

void OnPointActivity::drawHourglass(const int x, const int y, const int width, const int height) const {
  const int centerX = x + width / 2;
  const int centerY = y + height / 2;
  const int bottom = y + height;
  renderer.drawLine(x, y, x + width, y, HOURGLASS_STROKE, true);
  renderer.drawLine(x, y, centerX, centerY, HOURGLASS_STROKE, true);
  renderer.drawLine(x + width, y, centerX, centerY, HOURGLASS_STROKE, true);
  renderer.drawLine(centerX, centerY, x, bottom, HOURGLASS_STROKE, true);
  renderer.drawLine(centerX, centerY, x + width, bottom, HOURGLASS_STROKE, true);
  renderer.drawLine(x, bottom, x + width, bottom, HOURGLASS_STROKE, true);

  uint16_t intervalMinutes = state.intervalMinutes;
  if (intervalMinutes == 0 && state.next.valid && state.followingCount > 0 && state.following[0].valid &&
      sameDate(state.next.local, state.following[0].local) &&
      state.following[0].serviceMinute > state.next.serviceMinute) {
    intervalMinutes = state.following[0].serviceMinute - state.next.serviceMinute;
  }
  if (intervalMinutes == 0) return;

  constexpr int FILL_STEPS = 5;
  const int interval = static_cast<int>(intervalMinutes);
  const int remaining = static_cast<int>(std::clamp<int32_t>(state.minutesToNext, 0, intervalMinutes));
  const int elapsed = interval - remaining;
  const int lowerSteps = std::clamp((elapsed * FILL_STEPS + interval / 2) / interval, 0, FILL_STEPS);
  const int upperSteps = FILL_STEPS - lowerSteps;

  if (upperSteps > 0) {
    const int fillY = centerY - 5 - upperSteps * (height / 12);
    const int halfWidth = std::max(5, upperSteps * width / 12);
    const int xs[] = {centerX, centerX - halfWidth, centerX + halfWidth};
    const int ys[] = {centerY - 5, fillY, fillY};
    renderer.fillPolygon(xs, ys, 3, true);
  }
  if (lowerSteps > 0) {
    const int fillY = bottom - 5 - lowerSteps * (height / 12);
    const int halfWidth = std::max(5, lowerSteps * width / 12);
    const int xs[] = {centerX, centerX - halfWidth, centerX + halfWidth};
    const int ys[] = {fillY, bottom - 5, bottom - 5};
    renderer.fillPolygon(xs, ys, 3, true);
  }
}

void OnPointActivity::drawHeaderAndRoute(const on_point::Schedule& schedule) const {
  const int width = renderer.getScreenWidth();
  const char* timetableLabel = tr(STR_ON_POINT_P2P_TIMETABLE);
  renderer.drawText(UI_12_FONT_ID, MARGIN, TOP, tr(STR_ON_POINT), true, EpdFontFamily::BOLD);
  renderer.drawText(LABEL_FONT, width - MARGIN - renderer.getTextWidth(LABEL_FONT, timetableLabel), TOP + 2,
                    timetableLabel, true, EpdFontFamily::BOLD);
  renderer.drawLine(MARGIN, HEADER_RULE_Y, width - MARGIN, HEADER_RULE_Y, 5, true);

  renderer.drawText(ROUTE_FONT, MARGIN, ROUTE_ORIGIN_Y, schedule.origin, true, EpdFontFamily::BOLD);
  renderer.drawLine(MARGIN, ROUTE_LINE_Y, width - MARGIN - 18, ROUTE_LINE_Y, 4, true);
  const int arrowX = width - MARGIN + 4;
  const int arrowXs[] = {arrowX - 22, arrowX, arrowX - 22};
  const int arrowYs[] = {ROUTE_LINE_Y - 9, ROUTE_LINE_Y, ROUTE_LINE_Y + 9};
  renderer.fillPolygon(arrowXs, arrowYs, 3, true);
  renderer.drawText(ROUTE_FONT, width - MARGIN - renderer.getTextWidth(ROUTE_FONT, schedule.destination),
                    ROUTE_DESTINATION_Y, schedule.destination, true, EpdFontFamily::BOLD);
  drawReverseAffordance();
}

void OnPointActivity::drawReverseAffordance() const {
  const int width = renderer.getScreenWidth();
  const char* label = tr(STR_ON_POINT_REVERSE);
  const int textWidth = renderer.getTextWidth(LABEL_FONT, label, EpdFontFamily::BOLD);
  constexpr int ICON_WIDTH = 26;
  constexpr int ICON_GAP = 8;
  const int x = width - MARGIN - textWidth - ICON_GAP - ICON_WIDTH;
  const int iconLeft = x + textWidth + ICON_GAP;
  const int iconRight = iconLeft + ICON_WIDTH;
  constexpr int TOP_ARROW_Y = 79;
  constexpr int BOTTOM_ARROW_Y = 88;
  constexpr int ARROW_HEAD = 5;

  renderer.drawText(LABEL_FONT, x, ROUTE_ORIGIN_Y + 2, label, true, EpdFontFamily::BOLD);
  renderer.drawLine(iconLeft, TOP_ARROW_Y, iconRight, TOP_ARROW_Y, 3, true);
  renderer.drawLine(iconRight, TOP_ARROW_Y, iconRight - ARROW_HEAD, TOP_ARROW_Y - ARROW_HEAD, 3, true);
  renderer.drawLine(iconRight, TOP_ARROW_Y, iconRight - ARROW_HEAD, TOP_ARROW_Y + ARROW_HEAD, 3, true);
  renderer.drawLine(iconRight, BOTTOM_ARROW_Y, iconLeft, BOTTOM_ARROW_Y, 3, true);
  renderer.drawLine(iconLeft, BOTTOM_ARROW_Y, iconLeft + ARROW_HEAD, BOTTOM_ARROW_Y - ARROW_HEAD, 3, true);
  renderer.drawLine(iconLeft, BOTTOM_ARROW_Y, iconLeft + ARROW_HEAD, BOTTOM_ARROW_Y + ARROW_HEAD, 3, true);
}

void OnPointActivity::drawRouteList() const {
  const int width = renderer.getScreenWidth();
  const char* timetableLabel = tr(STR_ON_POINT_P2P_TIMETABLE);

  renderer.drawText(UI_12_FONT_ID, MARGIN, TOP, tr(STR_ON_POINT), true, EpdFontFamily::BOLD);
  renderer.drawText(LABEL_FONT, width - MARGIN - renderer.getTextWidth(LABEL_FONT, timetableLabel), TOP + 2,
                    timetableLabel, true, EpdFontFamily::BOLD);
  renderer.drawLine(MARGIN, HEADER_RULE_Y, width - MARGIN, HEADER_RULE_Y, 5, true);
  renderer.drawText(LABEL_FONT, MARGIN, ROUTE_LIST_LABEL_Y, tr(STR_ON_POINT_SELECT_ROUTE), true,
                    EpdFontFamily::BOLD);

  const size_t pageCount = (on_point::routeCount() + ROUTE_LIST_VISIBLE_ROWS - 1) / ROUTE_LIST_VISIBLE_ROWS;
  if (pageCount > 1) {
    char page[12];
    snprintf(page, sizeof(page), "%u / %u", static_cast<unsigned>(routeListFirstIndex() / ROUTE_LIST_VISIBLE_ROWS + 1),
             static_cast<unsigned>(pageCount));
    renderer.drawText(LABEL_FONT, width - MARGIN - renderer.getTextWidth(LABEL_FONT, page, EpdFontFamily::BOLD),
                      ROUTE_LIST_LABEL_Y, page, true, EpdFontFamily::BOLD);
  }

  const size_t first = routeListFirstIndex();
  const size_t visibleCount = routeListVisibleCount();
  for (size_t row = 0; row < visibleCount; ++row) {
    const size_t index = first + row;
    const on_point::Schedule& route = on_point::scheduleForRoute(index, false);
    const int y = ROUTE_LIST_ROW_TOP + static_cast<int>(row) * (ROUTE_LIST_ROW_HEIGHT + ROUTE_LIST_ROW_GAP);
    drawRouteListRow(route, index, y);
  }

  drawFooter(on_point::scheduleForRoute(routeIndex, false), tr(STR_ON_POINT_SELECT_ROUTE));
}

void OnPointActivity::drawRouteListRow(const on_point::Schedule& route, const size_t index, const int y) const {
  const int width = renderer.getScreenWidth();
  const on_point::RoutePair& routePair = on_point::routeAt(index);
  renderer.fillRect(MARGIN, y, index == routeIndex ? 10 : 3, ROUTE_LIST_ROW_HEIGHT, true);
  renderer.drawText(ROUTE_FONT, MARGIN + 28, y + 6, route.origin, true, EpdFontFamily::BOLD);
  renderer.drawText(LABEL_FONT, MARGIN + 28, y + 36, routePair.originArea, true, EpdFontFamily::BOLD);
  const int lineY = y + 59;
  renderer.drawLine(MARGIN + 28, lineY, width - MARGIN - 18, lineY, 4, true);
  const int arrowX = width - MARGIN + 4;
  const int arrowXs[] = {arrowX - 22, arrowX, arrowX - 22};
  const int arrowYs[] = {lineY - 9, lineY, lineY + 9};
  renderer.fillPolygon(arrowXs, arrowYs, 3, true);
  renderer.drawText(ROUTE_FONT, width - MARGIN - renderer.getTextWidth(ROUTE_FONT, route.destination),
                    y + 68, route.destination, true, EpdFontFamily::BOLD);
  renderer.drawText(LABEL_FONT,
                    width - MARGIN - renderer.getTextWidth(LABEL_FONT, routePair.destinationArea, EpdFontFamily::BOLD),
                    y + 98, routePair.destinationArea, true, EpdFontFamily::BOLD);
  renderer.drawText(LABEL_FONT, MARGIN + 28, y + 120, route.routeName, true, EpdFontFamily::BOLD);
}

void OnPointActivity::drawFooter(const on_point::Schedule& schedule, const char* leftLabel) const {
  const int width = renderer.getScreenWidth();
  renderer.drawLine(MARGIN, FOOTER_RULE_Y, width - MARGIN, FOOTER_RULE_Y, 3, true);
  renderer.drawText(LABEL_FONT, MARGIN, 744, leftLabel, true, EpdFontFamily::BOLD);

  if (schedule.sourceStatus == on_point::SourceStatus::Provisional) {
    const char* provisional = tr(STR_ON_POINT_PROVISIONAL);
    renderer.drawText(LABEL_FONT, width - MARGIN - renderer.getTextWidth(LABEL_FONT, provisional), 744, provisional,
                      true, EpdFontFamily::BOLD);
  }
}

void OnPointActivity::drawBadge(const char* label) const {
  constexpr int BADGE_PADDING_X = 12;
  constexpr int BADGE_HEIGHT = 30;
  const int width = renderer.getScreenWidth();
  const int textWidth = renderer.getTextWidth(LABEL_FONT, label, EpdFontFamily::BOLD);
  const int badgeWidth = textWidth + BADGE_PADDING_X * 2;
  const int badgeX = width - MARGIN - badgeWidth;
  renderer.fillRect(badgeX, 194, badgeWidth, BADGE_HEIGHT, true);
  renderer.drawText(LABEL_FONT, badgeX + BADGE_PADDING_X, 200, label, false, EpdFontFamily::BOLD);
}

void OnPointActivity::drawPrimaryScreen(const on_point::Schedule& schedule) {
  const int width = renderer.getScreenWidth();
  renderer.drawText(LABEL_FONT, MARGIN, COUNTDOWN_LABEL_Y, tr(STR_ON_POINT_UNTIL_DEPARTURE), true, EpdFontFamily::BOLD);

  if (state.isLastDeparture) {
    drawBadge(tr(STR_ON_POINT_LAST));
  } else if (state.serviceStatus == on_point::ServiceStatus::BeforeFirst) {
    drawBadge(tr(STR_ON_POINT_FIRST_TRIP));
  }

  drawLargeNumber(state.minutesToNext, MARGIN, COUNTDOWN_Y, COUNTDOWN_MAX_WIDTH, COUNTDOWN_HEIGHT);
  drawHourglass(HOURGLASS_X, HOURGLASS_Y, HOURGLASS_WIDTH, HOURGLASS_HEIGHT);
  renderer.drawText(UI_12_FONT_ID, COUNTDOWN_UNIT_X, COUNTDOWN_UNIT_Y, tr(STR_ON_POINT_MIN), true, EpdFontFamily::BOLD);

  renderer.drawLine(MARGIN, LEDGER_RULE_Y, width - MARGIN, LEDGER_RULE_Y, 5, true);
  renderer.drawText(LABEL_FONT, MARGIN, 562, tr(STR_ON_POINT_SCHEDULED), true, EpdFontFamily::BOLD);
  renderer.drawText(LABEL_FONT, 236, 562, tr(STR_ON_POINT_NEXT), true, EpdFontFamily::BOLD);
  const char* lastLabel = tr(STR_ON_POINT_LAST);
  renderer.drawText(LABEL_FONT, width - MARGIN - renderer.getTextWidth(LABEL_FONT, lastLabel, EpdFontFamily::BOLD),
                    562, lastLabel, true, EpdFontFamily::BOLD);

  char departureTime[8];
  formatTime(state.next, departureTime, sizeof(departureTime));
  renderer.drawText(TIME_FONT, MARGIN, 590, departureTime, true, EpdFontFamily::BOLD);

  const bool hasFollowing = state.followingCount > 0 && sameDate(state.following[0].local, state.next.local);
  if (!hasFollowing) {
    renderer.drawText(FOLLOWING_FONT, 236, 594, tr(STR_ON_POINT_NONE), true, EpdFontFamily::BOLD);
  } else {
    char firstTime[8];
    formatTime(state.following[0], firstTime, sizeof(firstTime));
    renderer.drawText(FOLLOWING_FONT, 236, 594, firstTime, true, EpdFontFamily::BOLD);
  }

  if (state.last.valid) {
    char lastTime[8];
    formatTime(state.last, lastTime, sizeof(lastTime));
    renderer.drawText(FOLLOWING_FONT,
                      width - MARGIN - renderer.getTextWidth(FOLLOWING_FONT, lastTime, EpdFontFamily::BOLD), 594,
                      lastTime, true, EpdFontFamily::BOLD);
  }

  if (state.isLastDeparture) {
    renderer.drawText(LABEL_FONT, MARGIN, 670, tr(STR_ON_POINT_FINAL_TRIP_TODAY), true, EpdFontFamily::BOLD);
  } else if (state.serviceStatus == on_point::ServiceStatus::BeforeFirst) {
    renderer.drawText(LABEL_FONT, MARGIN, 670, tr(STR_ON_POINT_FIRST_SCHEDULED_TRIP), true, EpdFontFamily::BOLD);
  } else if (state.intervalMinutes > 0) {
    char interval[40];
    snprintf(interval, sizeof(interval), "%s %u %s", tr(STR_ON_POINT_EVERY), state.intervalMinutes,
             tr(STR_ON_POINT_MIN));
    renderer.drawText(LABEL_FONT, MARGIN, 670, interval, true, EpdFontFamily::BOLD);
  }

  char date[16];
  formatDate(state.localNow, date, sizeof(date));
  drawFooter(schedule, date);
}

void OnPointActivity::drawEndedScreen(const on_point::Schedule& schedule) {
  const int width = renderer.getScreenWidth();
  const bool noServiceToday = state.serviceStatus == on_point::ServiceStatus::NoServiceToday;
  renderer.drawText(TIME_FONT, MARGIN, 250, noServiceToday ? tr(STR_ON_POINT_NO_SERVICE) : tr(STR_ON_POINT_SERVICE),
                    true, EpdFontFamily::BOLD);
  renderer.drawText(TIME_FONT, MARGIN, 303, noServiceToday ? tr(STR_ON_POINT_TODAY) : tr(STR_ON_POINT_ENDED), true,
                    EpdFontFamily::BOLD);
  renderer.drawLine(MARGIN, 392, width - MARGIN, 392, 5, true);
  renderer.drawText(LABEL_FONT, MARGIN, 420, tr(STR_ON_POINT_NEXT_SERVICE), true, EpdFontFamily::BOLD);

  if (state.next.valid) {
    char time[8];
    char date[16];
    formatTime(state.next, time, sizeof(time));
    formatDate(state.next.local, date, sizeof(date));
    renderer.drawText(TIME_FONT, MARGIN, 464, time, true, EpdFontFamily::BOLD);
    renderer.drawText(FOLLOWING_FONT, width - MARGIN - renderer.getTextWidth(FOLLOWING_FONT, date), 465, date, true,
                      EpdFontFamily::BOLD);
    renderer.drawText(
        LABEL_FONT,
        width - MARGIN - renderer.getTextWidth(LABEL_FONT, tr(STR_ON_POINT_FIRST_TRIP), EpdFontFamily::BOLD), 511,
        tr(STR_ON_POINT_FIRST_TRIP), true, EpdFontFamily::BOLD);
    renderer.drawText(NOTOSANS_12_FONT_ID, MARGIN, 610, tr(STR_ON_POINT_NO_DEPARTURE_BEFORE));
  } else {
    renderer.drawText(FOLLOWING_FONT, MARGIN, 464, tr(STR_ON_POINT_NO_FUTURE_SERVICE), true, EpdFontFamily::BOLD);
  }

  char currentDate[16];
  formatDate(state.localNow, currentDate, sizeof(currentDate));
  drawFooter(schedule, currentDate);
}

void OnPointActivity::drawClockUnavailableScreen(const on_point::Schedule& schedule) const {
  renderer.fillRect(MARGIN, 246, 12, 214, true);
  renderer.drawText(TIME_FONT, 64, 280, tr(STR_ON_POINT_CLOCK), true, EpdFontFamily::BOLD);
  renderer.drawText(TIME_FONT, 64, 333, tr(STR_ON_POINT_NOT_SET), true, EpdFontFamily::BOLD);
  renderer.drawLine(64, 408, renderer.getScreenWidth() - MARGIN, 408, 5, true);
  renderer.drawText(LABEL_FONT, 64, 428, tr(STR_ON_POINT_NO_COUNTDOWN), true, EpdFontFamily::BOLD);
  renderer.drawText(NOTOSANS_14_FONT_ID, 64, 500, tr(STR_ON_POINT_SYNC_CLOCK_HINT), true, EpdFontFamily::BOLD);
  renderer.drawText(NOTOSANS_12_FONT_ID, 64, 545, tr(STR_ON_POINT_CLOCK_GUESS));
  drawFooter(schedule, tr(STR_ON_POINT_RTC_REQUIRED));
}

void OnPointActivity::drawTimetableUnavailableScreen(const on_point::Schedule& schedule) const {
  renderer.fillRect(MARGIN, 246, 12, 214, true);
  renderer.drawText(FOLLOWING_FONT, 64, 290, tr(STR_ON_POINT_TIMETABLE_UNAVAILABLE), true, EpdFontFamily::BOLD);
  renderer.drawLine(64, 342, renderer.getScreenWidth() - MARGIN, 342, 5, true);

  char date[16];
  formatDate(state.localNow, date, sizeof(date));
  drawFooter(schedule, date);
}

void OnPointActivity::render(RenderLock&&) {
  renderer.clearScreen();
  if (view == View::RouteList) {
    drawRouteList();
  } else {
    const on_point::Schedule& schedule = selectedSchedule();
    drawHeaderAndRoute(schedule);

    if (!clockReady) {
      drawClockUnavailableScreen(schedule);
    } else if (state.serviceStatus == on_point::ServiceStatus::InvalidSchedule) {
      drawTimetableUnavailableScreen(schedule);
    } else if (state.serviceStatus == on_point::ServiceStatus::ServiceEnded ||
               state.serviceStatus == on_point::ServiceStatus::NoServiceToday ||
               state.serviceStatus == on_point::ServiceStatus::NoFutureService) {
      drawEndedScreen(schedule);
    } else {
      drawPrimaryScreen(schedule);
    }
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
