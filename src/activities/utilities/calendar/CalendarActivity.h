#pragma once

#include <EpdFontFamily.h>

#include <cstdint>

#include "activities/Activity.h"
#include "util/CivilCalendar.h"

// Calendar utility: a month grid (the dominant element) with marked dates from
// util/MarkedDates.h and a detail panel for the selected day. No events,
// reminders or sync.
//
// Boards with a trusted RTC (X4 Pro, clock synced) open on the current month
// and mark today. Other boards never claim to know today's date: they open on
// the last month viewed, or ask for a month on first launch. That choice is
// browsing state only, never the device date.
class CalendarActivity final : public Activity {
  enum class View : uint8_t { Month, Picker };

  View view = View::Month;
  int16_t viewYear = 2026;
  uint8_t viewMonth = 1;
  uint8_t selectedDay = 1;
  bool hasMonthView = false;  // false until a month has been chosen (first launch)
  bool viewChanged = false;   // persist the viewed month on exit

  bool hasToday = false;
  civil_cal::Date today;

  int16_t pickerYear = 2026;
  uint8_t pickerMonth = 1;

  bool cleanRefresh = true;
  uint8_t fastRefreshCount = 0;

  void showMonth(int16_t year, uint8_t month, uint8_t day);
  void moveSelectedDay(int delta);
  void stepMonth(int delta);
  void openPicker();
  void movePicker(int deltaMonths);
  void goToToday();

  void handleMonthInput();
  void handlePickerInput();

  // Geometry shared by drawing and touch hit-testing.
  int gridLeft() const;
  int cellWidth() const;
  int contentBottom() const;
  bool dayAtPoint(int x, int y, uint8_t& day) const;
  bool pickerMonthAtPoint(int x, int y, uint8_t& month) const;
  void chevronRects(int top, int& prevX, int& nextX, int& y, int& size) const;
  void todayChipRect(int& x, int& y, int& w, int& h) const;
  bool showTodayChip() const;

  void drawMasthead(const char* meta) const;
  void drawChevron(int x, int y, int size, bool pointRight) const;
  void drawMonthView() const;
  void drawDayCell(uint8_t day, int x, int y, int w, int h) const;
  int drawWrapped(int font, int x, int y, int width, const char* text, int maxLines, EpdFontFamily::Style style) const;
  void drawDetails(int top) const;
  void drawPickerView() const;

 public:
  CalendarActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Calendar", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
};
