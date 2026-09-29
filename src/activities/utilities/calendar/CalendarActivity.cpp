#include "CalendarActivity.h"

#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>
#include <iterator>

#include "CrossPointState.h"
#include "MappedInputManager.h"
#include "activities/utilities/UtilityDates.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "util/MarkedDates.h"

namespace {
constexpr int MARGIN = 24;
constexpr int HEADER_TITLE_Y = 8;
constexpr int HEADER_META_Y = 20;
constexpr int HEADER_RULE_Y = 50;
constexpr int HEADER_FONT = NOTOSANS_16_FONT_ID;
constexpr int LABEL_FONT = UI_10_FONT_ID;
constexpr int TITLE_FONT = NOTOSANS_18_FONT_ID;
constexpr int TITLE_Y = 64;
constexpr int CHEVRON_SIZE = 40;
constexpr int WEEK_ROW_Y = 112;
constexpr int WEEK_RULE_Y = 136;
constexpr int GRID_TOP = 142;
constexpr int CELL_HEIGHT = 54;
constexpr int GRID_ROWS = 6;
constexpr int DAY_FONT = NOTOSANS_14_FONT_ID;
constexpr int MARK_SIZE = 10;  // today's month in the picker
constexpr int MARK_THICKNESS = 3;
constexpr int DETAILS_TOP = GRID_TOP + GRID_ROWS * CELL_HEIGHT + 16;
constexpr int DETAIL_NAME_FONT = NOTOSANS_16_FONT_ID;
constexpr int DETAIL_NOTE_FONT = NOTOSANS_12_FONT_ID;
constexpr size_t MAX_DETAILS = 3;
constexpr int MAX_NAME_LINES = 2;
constexpr int MAX_NOTE_LINES = 2;
// Largest first: the picker uses the first that fits a month name in its box.
constexpr int PICKER_NAME_FONTS[] = {NOTOSANS_14_FONT_ID, NOTOSANS_12_FONT_ID, UI_10_FONT_ID};
constexpr int PICKER_TOP = 140;
constexpr int PICKER_COLS = 3;
constexpr int PICKER_GAP = 8;
constexpr int MIN_YEAR = 1900;
constexpr int MAX_YEAR = 2199;
constexpr uint8_t HALF_REFRESH_AFTER_FAST = 8;
// First-launch browsing year on boards without a trusted date: the year the
// built-in official holidays cover.
constexpr int16_t DEFAULT_BROWSE_YEAR = marked_dates::PHILIPPINES_2026[0].year;

const char* kindLabel(const marked_dates::Kind kind) {
  switch (kind) {
    case marked_dates::Kind::RegularHoliday:
      return tr(STR_CALENDAR_REGULAR_HOLIDAY);
    case marked_dates::Kind::SpecialNonWorking:
      return tr(STR_CALENDAR_SPECIAL_NON_WORKING);
    case marked_dates::Kind::SpecialWorking:
      return tr(STR_CALENDAR_SPECIAL_WORKING);
    case marked_dates::Kind::InternationalObservance:
      return tr(STR_CALENDAR_INTERNATIONAL_OBSERVANCE);
    case marked_dates::Kind::CulturalObservance:
      return tr(STR_CALENDAR_CULTURAL_OBSERVANCE);
  }
  return "";
}

bool inRect(const int px, const int py, const int x, const int y, const int w, const int h) {
  return px >= x && px < x + w && py >= y && py < y + h;
}
}  // namespace

void CalendarActivity::onEnter() {
  Activity::onEnter();
  civil_cal::DateTime now;
  hasToday = utility_dates::trustedLocalNow(now);
  if (hasToday) {
    today = now.date;
    showMonth(today.year, today.month, today.day);
  } else if (APP_STATE.calendarViewMonth != 0) {
    showMonth(static_cast<int16_t>(APP_STATE.calendarViewYear), APP_STATE.calendarViewMonth, 1);
  } else {
    pickerYear = DEFAULT_BROWSE_YEAR;
    pickerMonth = 1;
    view = View::Picker;
  }
  viewChanged = false;
  cleanRefresh = true;
  requestUpdate();
}

void CalendarActivity::onExit() {
  if (viewChanged && hasMonthView) {
    APP_STATE.calendarViewYear = static_cast<uint16_t>(viewYear);
    APP_STATE.calendarViewMonth = viewMonth;
    APP_STATE.saveToFile();
  }
  Activity::onExit();
}

void CalendarActivity::showMonth(const int16_t year, const uint8_t month, const uint8_t day) {
  viewYear = static_cast<int16_t>(std::clamp<int>(year, MIN_YEAR, MAX_YEAR));
  viewMonth = month;
  selectedDay = static_cast<uint8_t>(std::clamp<int>(day, 1, civil_cal::daysInMonth(viewYear, viewMonth)));
  view = View::Month;
  hasMonthView = true;
  viewChanged = true;
}

void CalendarActivity::moveSelectedDay(const int delta) {
  const int32_t days = civil_cal::daysFromCivil(viewYear, viewMonth, selectedDay) + delta;
  const civil_cal::Date date = civil_cal::civilFromDays(days);
  if (date.year < MIN_YEAR || date.year > MAX_YEAR) return;
  if (date.month != viewMonth || date.year != viewYear) cleanRefresh = true;
  showMonth(date.year, date.month, date.day);
  requestUpdate();
}

void CalendarActivity::stepMonth(const int delta) {
  int16_t year = viewYear;
  uint8_t month = viewMonth;
  civil_cal::addMonths(year, month, delta);
  if (year < MIN_YEAR || year > MAX_YEAR) return;
  const bool isTodaysMonth = hasToday && year == today.year && month == today.month;
  showMonth(year, month, isTodaysMonth ? today.day : 1);
  cleanRefresh = true;
  requestUpdate();
}

void CalendarActivity::openPicker() {
  pickerYear = viewYear;
  pickerMonth = viewMonth;
  view = View::Picker;
  cleanRefresh = true;
  requestUpdate();
}

void CalendarActivity::movePicker(const int deltaMonths) {
  int16_t year = pickerYear;
  uint8_t month = pickerMonth;
  civil_cal::addMonths(year, month, deltaMonths);
  if (year < MIN_YEAR || year > MAX_YEAR) return;
  pickerYear = year;
  pickerMonth = month;
  requestUpdate();
}

void CalendarActivity::goToToday() {
  if (!hasToday) return;
  showMonth(today.year, today.month, today.day);
  cleanRefresh = true;
  requestUpdate();
}

// --- Geometry ---

int CalendarActivity::cellWidth() const { return (renderer.getScreenWidth() - 2 * MARGIN) / 7; }

int CalendarActivity::gridLeft() const {
  return MARGIN + (renderer.getScreenWidth() - 2 * MARGIN - 7 * cellWidth()) / 2;
}

int CalendarActivity::contentBottom() const {
  const int hints = mappedInput.hasTouch() ? 0 : UITheme::getInstance().getMetrics().buttonHintsHeight;
  return renderer.getScreenHeight() - hints - 8;
}

bool CalendarActivity::dayAtPoint(const int x, const int y, uint8_t& day) const {
  const int w = cellWidth();
  if (x < gridLeft() || x >= gridLeft() + 7 * w || y < GRID_TOP || y >= GRID_TOP + GRID_ROWS * CELL_HEIGHT) {
    return false;
  }
  const int cell = (y - GRID_TOP) / CELL_HEIGHT * 7 + (x - gridLeft()) / w;
  const int value = cell - civil_cal::leadingBlankDays(viewYear, viewMonth) + 1;
  if (value < 1 || value > civil_cal::daysInMonth(viewYear, viewMonth)) return false;
  day = static_cast<uint8_t>(value);
  return true;
}

bool CalendarActivity::pickerMonthAtPoint(const int x, const int y, uint8_t& month) const {
  const int width = renderer.getScreenWidth() - 2 * MARGIN;
  const int rows = 12 / PICKER_COLS;
  const int cellW = (width - (PICKER_COLS - 1) * PICKER_GAP) / PICKER_COLS;
  const int cellH = (contentBottom() - PICKER_TOP - (rows - 1) * PICKER_GAP) / rows;
  if (x < MARGIN || y < PICKER_TOP) return false;
  const int col = (x - MARGIN) / (cellW + PICKER_GAP);
  const int row = (y - PICKER_TOP) / (cellH + PICKER_GAP);
  if (col >= PICKER_COLS || row >= rows) return false;
  month = static_cast<uint8_t>(row * PICKER_COLS + col + 1);
  return true;
}

// Previous/next chevrons sit at the right end of a title row (touch boards).
void CalendarActivity::chevronRects(const int top, int& prevX, int& nextX, int& y, int& size) const {
  size = CHEVRON_SIZE;
  nextX = renderer.getScreenWidth() - MARGIN - size;
  prevX = nextX - 8 - size;
  y = top;
}

bool CalendarActivity::showTodayChip() const {
  return hasToday && !(viewYear == today.year && viewMonth == today.month && selectedDay == today.day);
}

void CalendarActivity::todayChipRect(int& x, int& y, int& w, int& h) const {
  // Reserved at the right end of the detail panel's label row.
  w = renderer.getTextWidth(LABEL_FONT, tr(STR_CALENDAR_TODAY), EpdFontFamily::BOLD) + 20;
  h = renderer.getLineHeight(LABEL_FONT) + 10;
  y = DETAILS_TOP + 16;
  x = renderer.getScreenWidth() - MARGIN - w;
}

// --- Input ---

void CalendarActivity::handleMonthInput() {
  using Button = MappedInputManager::Button;
  int tx = 0;
  int ty = 0;
  if (mappedInput.wasScreenTapped(tx, ty)) {
    uint8_t day = 0;
    int prevX, nextX, chevronY, size;
    chevronRects(TITLE_Y - 4, prevX, nextX, chevronY, size);
    int chipX, chipY, chipW, chipH;
    todayChipRect(chipX, chipY, chipW, chipH);
    if (dayAtPoint(tx, ty, day)) {
      if (day != selectedDay) {
        selectedDay = day;
        requestUpdate();
      }
    } else if (inRect(tx, ty, prevX - 4, chevronY - 4, size + 8, size + 8)) {
      stepMonth(-1);
    } else if (inRect(tx, ty, nextX - 4, chevronY - 4, size + 8, size + 8)) {
      stepMonth(1);
    } else if (showTodayChip() && inRect(tx, ty, chipX, chipY, chipW, chipH)) {
      goToToday();
    } else if (ty >= TITLE_Y - 6 && ty < WEEK_ROW_Y - 6 && (!mappedInput.hasTouch() || tx < prevX - 8)) {
      openPicker();  // tapping the month title chooses another month
    }
    return;
  }
  if (mappedInput.wasReleased(Button::Confirm)) {
    openPicker();
    return;
  }
  if (mappedInput.wasPressed(Button::Left)) moveSelectedDay(-1);
  if (mappedInput.wasPressed(Button::Right)) moveSelectedDay(1);
  if (mappedInput.wasPressed(Button::Up)) moveSelectedDay(-7);
  if (mappedInput.wasPressed(Button::Down)) moveSelectedDay(7);
}

void CalendarActivity::handlePickerInput() {
  using Button = MappedInputManager::Button;
  int tx = 0;
  int ty = 0;
  if (mappedInput.wasScreenTapped(tx, ty)) {
    uint8_t month = 0;
    int prevX, nextX, chevronY, size;
    chevronRects(TITLE_Y - 4, prevX, nextX, chevronY, size);
    if (pickerMonthAtPoint(tx, ty, month)) {
      const bool isTodaysMonth = hasToday && pickerYear == today.year && month == today.month;
      showMonth(pickerYear, month, isTodaysMonth ? today.day : 1);
      cleanRefresh = true;
      requestUpdate();
    } else if (inRect(tx, ty, prevX - 4, chevronY - 4, size + 8, size + 8)) {
      movePicker(-12);
    } else if (inRect(tx, ty, nextX - 4, chevronY - 4, size + 8, size + 8)) {
      movePicker(12);
    }
    return;
  }
  if (mappedInput.wasReleased(Button::Confirm)) {
    const bool isTodaysMonth = hasToday && pickerYear == today.year && pickerMonth == today.month;
    showMonth(pickerYear, pickerMonth, isTodaysMonth ? today.day : 1);
    cleanRefresh = true;
    requestUpdate();
    return;
  }
  if (mappedInput.wasPressed(Button::Left)) movePicker(-1);
  if (mappedInput.wasPressed(Button::Right)) movePicker(1);
  if (mappedInput.wasPressed(Button::Up)) movePicker(-PICKER_COLS);
  if (mappedInput.wasPressed(Button::Down)) movePicker(PICKER_COLS);
}

void CalendarActivity::loop() {
  using Button = MappedInputManager::Button;
  if (mappedInput.wasHomeGesture() || mappedInput.wasHomeKeyHold()) {
    activityManager.goHome();
    return;
  }
  if (mappedInput.wasReleased(Button::Back)) {
    if (view == View::Picker && hasMonthView) {
      view = View::Month;
      cleanRefresh = true;
      requestUpdate();
      return;
    }
    activityManager.goToApps(AppId::Calendar);
    return;
  }
  // Input during a refresh waits for it, then applies, instead of being dropped.
  RenderLock lock(*this);
  if (view == View::Month) {
    handleMonthInput();
  } else {
    handlePickerInput();
  }
}

// --- Drawing ---

void CalendarActivity::drawMasthead(const char* meta) const {
  const int width = renderer.getScreenWidth();
  renderer.drawText(HEADER_FONT, MARGIN, HEADER_TITLE_Y, tr(STR_CALENDAR), true, EpdFontFamily::BOLD);
  if (meta != nullptr) {
    renderer.drawText(LABEL_FONT, width - MARGIN - renderer.getTextWidth(LABEL_FONT, meta, EpdFontFamily::BOLD),
                      HEADER_META_Y, meta, true, EpdFontFamily::BOLD);
  }
  renderer.drawLine(MARGIN, HEADER_RULE_Y, width - MARGIN, HEADER_RULE_Y, 5, true);
}

void CalendarActivity::drawChevron(const int x, const int y, const int size, const bool pointRight) const {
  renderer.drawRect(x, y, size, size, 2, true);
  const int cx = x + size / 2;
  const int cy = y + size / 2;
  const int arm = size / 5;
  const int tip = pointRight ? cx + arm / 2 : cx - arm / 2;
  const int back = pointRight ? tip - arm : tip + arm;
  renderer.drawLine(back, cy - arm, tip, cy, 3, true);
  renderer.drawLine(back, cy + arm, tip, cy, 3, true);
}

void CalendarActivity::drawDayCell(const uint8_t day, const int x, const int y, const int w, const int h) const {
  const bool isToday = hasToday && viewYear == today.year && viewMonth == today.month && day == today.day;
  const bool isSelected = day == selectedDay;
  const bool ink = !isToday;  // today is drawn inverted

  // Strong states: today is a filled cell, the selected day a heavy frame.
  if (isToday) renderer.fillRect(x + 2, y + 2, w - 4, h - 4, true);
  if (isSelected) {
    if (isToday) {
      renderer.drawRect(x + 5, y + 5, w - 10, h - 10, 2, false);
    } else {
      renderer.drawRect(x + 2, y + 2, w - 4, h - 4, 3, true);
    }
  }

  char number[3];
  snprintf(number, sizeof(number), "%u", static_cast<unsigned>(day));
  const auto style = civil_cal::weekday(viewYear, viewMonth, day) == 0 ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
  const int textW = renderer.getTextWidth(DAY_FONT, number, style);
  const int lineH = renderer.getLineHeight(DAY_FONT);
  const int textX = x + (w - textW) / 2;
  const int textY = y + (h - lineH - MARK_THICKNESS - 3) / 2;
  renderer.drawText(DAY_FONT, textX, textY, number, ink, style);

  // Significance is a secondary mark under the number, never a frame: a solid
  // rule for official days, a dotted rule for observances.
  const int markY = textY + lineH + 1;
  const int markW = std::max(textW, 12);
  const int markX = x + (w - markW) / 2;
  const auto mark = marked_dates::markOn(viewYear, viewMonth, day);
  if (mark == marked_dates::Mark::Official) {
    renderer.fillRect(markX, markY, markW, MARK_THICKNESS, ink);
  } else if (mark == marked_dates::Mark::Observance) {
    for (int dotX = markX; dotX < markX + markW; dotX += 2 * MARK_THICKNESS) {
      renderer.fillRect(dotX, markY, MARK_THICKNESS, MARK_THICKNESS, ink);
    }
  }
}

// Draws `text` wrapped to `width`; returns the height used.
int CalendarActivity::drawWrapped(const int font, const int x, const int y, const int width, const char* text,
                                  const int maxLines, const EpdFontFamily::Style style) const {
  const auto lines = renderer.wrappedText(font, text, width, maxLines, style);
  const int lineH = renderer.getLineHeight(font);
  for (size_t i = 0; i < lines.size(); ++i) {
    renderer.drawText(font, x, y + static_cast<int>(i) * lineH, lines[i].c_str(), true, style);
  }
  return static_cast<int>(lines.size()) * lineH;
}

void CalendarActivity::drawDetails(const int top) const {
  const int width = renderer.getScreenWidth();
  renderer.drawLine(MARGIN, top, width - MARGIN, top, 5, true);

  // Label row: the selected day in a black label; TODAY on the right when away.
  char label[48];
  snprintf(label, sizeof(label), "%s  %u %s %d",
           utility_dates::weekdayShortName(civil_cal::weekday(viewYear, viewMonth, selectedDay)),
           static_cast<unsigned>(selectedDay), utility_dates::monthName(viewMonth), static_cast<int>(viewYear));
  const int labelW = renderer.getTextWidth(LABEL_FONT, label, EpdFontFamily::BOLD);
  const int labelH = renderer.getLineHeight(LABEL_FONT) + 10;
  renderer.fillRect(MARGIN, top + 16, labelW + 20, labelH, true);
  renderer.drawText(LABEL_FONT, MARGIN + 10, top + 21, label, false, EpdFontFamily::BOLD);
  if (showTodayChip()) {
    int x, y, w, h;
    todayChipRect(x, y, w, h);
    renderer.drawRect(x, y, w, h, 2, true);
    renderer.drawText(LABEL_FONT, x + 10, y + 5, tr(STR_CALENDAR_TODAY), true, EpdFontFamily::BOLD);
  }

  const marked_dates::MarkedDate* found[MAX_DETAILS] = {};
  const size_t count = marked_dates::datesOn(viewYear, viewMonth, selectedDay, found, MAX_DETAILS);
  int y = top + 16 + labelH + 18;
  if (count == 0) {
    renderer.drawText(DETAIL_NOTE_FONT, MARGIN, y, tr(STR_CALENDAR_NOTHING_MARKED));
    return;
  }
  const int textX = MARGIN + 16;
  const int textW = width - MARGIN - textX;
  const int nameH = renderer.getLineHeight(DETAIL_NAME_FONT);
  for (size_t i = 0; i < count && y + nameH < contentBottom(); ++i) {
    const marked_dates::MarkedDate& date = *found[i];
    const int entryTop = y;
    renderer.drawText(LABEL_FONT, textX, y, kindLabel(date.kind), true, EpdFontFamily::BOLD);
    y += renderer.getLineHeight(LABEL_FONT) + 4;
    y += drawWrapped(DETAIL_NAME_FONT, textX, y, textW, date.name, MAX_NAME_LINES, EpdFontFamily::BOLD);
    if (date.note != nullptr) {
      y += 2 + drawWrapped(DETAIL_NOTE_FONT, textX, y + 2, textW, date.note, MAX_NOTE_LINES, EpdFontFamily::REGULAR);
    }
    // Accent rule spanning the entry: solid for official days, thin for observances.
    const bool official = marked_dates::isOfficial(date.kind);
    renderer.fillRect(MARGIN, entryTop + 2, official ? 6 : 2, y - entryTop - 2, true);
    y += 16;
  }
}

void CalendarActivity::drawMonthView() const {
  drawMasthead(nullptr);
  const int width = renderer.getScreenWidth();

  // Title: month and year, the way into the month picker.
  const char* month = utility_dates::monthName(viewMonth);
  renderer.drawText(TITLE_FONT, MARGIN, TITLE_Y, month, true, EpdFontFamily::BOLD);
  char year[8];
  snprintf(year, sizeof(year), "%d", static_cast<int>(viewYear));
  renderer.drawText(TITLE_FONT, MARGIN + renderer.getTextWidth(TITLE_FONT, month, EpdFontFamily::BOLD) + 12, TITLE_Y,
                    year);
  if (mappedInput.hasTouch()) {
    int prevX, nextX, chevronY, size;
    chevronRects(TITLE_Y - 4, prevX, nextX, chevronY, size);
    drawChevron(prevX, chevronY, size, false);
    drawChevron(nextX, chevronY, size, true);
  }

  const int w = cellWidth();
  const int left = gridLeft();
  for (uint8_t weekday = 0; weekday < 7; ++weekday) {
    const char* name = utility_dates::weekdayShortName(weekday);
    const int textW = renderer.getTextWidth(LABEL_FONT, name, EpdFontFamily::BOLD);
    renderer.drawText(LABEL_FONT, left + weekday * w + (w - textW) / 2, WEEK_ROW_Y, name, true, EpdFontFamily::BOLD);
  }
  renderer.drawLine(MARGIN, WEEK_RULE_Y, width - MARGIN, WEEK_RULE_Y, 2, true);

  const int blanks = civil_cal::leadingBlankDays(viewYear, viewMonth);
  const int days = civil_cal::daysInMonth(viewYear, viewMonth);
  for (int day = 1; day <= days; ++day) {
    const int cell = blanks + day - 1;
    drawDayCell(static_cast<uint8_t>(day), left + (cell % 7) * w, GRID_TOP + (cell / 7) * CELL_HEIGHT, w, CELL_HEIGHT);
  }

  drawDetails(DETAILS_TOP);

  const auto labels =
      mappedInput.mapLabels(tr(STR_BACK), tr(STR_CALENDAR_MONTH), tr(STR_CALENDAR_PREV_DAY), tr(STR_CALENDAR_NEXT_DAY));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}

void CalendarActivity::drawPickerView() const {
  drawMasthead(tr(STR_CALENDAR_CHOOSE_MONTH));

  char year[8];
  snprintf(year, sizeof(year), "%d", static_cast<int>(pickerYear));
  renderer.drawText(TITLE_FONT, MARGIN, TITLE_Y, year, true, EpdFontFamily::BOLD);
  if (mappedInput.hasTouch()) {
    int prevX, nextX, chevronY, size;
    chevronRects(TITLE_Y - 4, prevX, nextX, chevronY, size);
    drawChevron(prevX, chevronY, size, false);
    drawChevron(nextX, chevronY, size, true);
  }

  const int width = renderer.getScreenWidth() - 2 * MARGIN;
  const int rows = 12 / PICKER_COLS;
  const int cellW = (width - (PICKER_COLS - 1) * PICKER_GAP) / PICKER_COLS;
  const int cellH = (contentBottom() - PICKER_TOP - (rows - 1) * PICKER_GAP) / rows;
  constexpr int INSET = 10;

  // One name size for the whole grid: the largest that fits every month.
  int nameFont = PICKER_NAME_FONTS[std::size(PICKER_NAME_FONTS) - 1];
  for (const int font : PICKER_NAME_FONTS) {
    bool fits = true;
    for (uint8_t month = 1; month <= 12 && fits; ++month) {
      fits = renderer.getTextWidth(font, utility_dates::monthName(month), EpdFontFamily::BOLD) <= cellW - 2 * INSET;
    }
    if (fits) {
      nameFont = font;
      break;
    }
  }

  for (uint8_t month = 1; month <= 12; ++month) {
    const int index = month - 1;
    const int x = MARGIN + (index % PICKER_COLS) * (cellW + PICKER_GAP);
    const int y = PICKER_TOP + (index / PICKER_COLS) * (cellH + PICKER_GAP);
    const bool selected = month == pickerMonth;
    const bool viewed = hasMonthView && pickerYear == viewYear && month == viewMonth;
    if (selected) {
      renderer.fillRect(x, y, cellW, cellH, true);
    } else {
      renderer.drawRect(x, y, cellW, cellH, viewed ? 4 : 2, true);
    }
    char number[4];
    snprintf(number, sizeof(number), "%02u", static_cast<unsigned>(month));
    renderer.drawText(LABEL_FONT, x + INSET, y + 8, number, !selected, EpdFontFamily::BOLD);
    renderer.drawText(nameFont, x + INSET, y + cellH - renderer.getLineHeight(nameFont) - INSET,
                      utility_dates::monthName(month), !selected, EpdFontFamily::BOLD);
    if (hasToday && pickerYear == today.year && month == today.month) {
      renderer.fillRect(x + cellW - MARK_SIZE - INSET, y + INSET, MARK_SIZE, MARK_SIZE, !selected);
    }
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_CALENDAR_OPEN), tr(STR_CALENDAR_PREV_MONTH),
                                            tr(STR_CALENDAR_NEXT_MONTH));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}

void CalendarActivity::render(RenderLock&&) {
  renderer.clearScreen();
  if (view == View::Month) {
    drawMonthView();
  } else {
    drawPickerView();
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
