#pragma once

#include <cstdint>

#include "CalculatorEngine.h"
#include "activities/Activity.h"

// Calculator utility: a basic desk calculator (CalculatorEngine) with a keypad
// as the dominant element. Touch boards tap keys; button boards move a focus
// frame with the direction buttons and press with Confirm. The screen only
// redraws in response to input.
class CalculatorActivity final : public Activity {
 public:
  enum class KeyType : uint8_t { Digit, Point, Operator, Equals, Clear, Backspace };
  struct Key {
    uint8_t col, row, colSpan, rowSpan;
    KeyType type;
    uint8_t value;  // digit, or CalculatorEngine::Op for operators
  };
  static constexpr uint8_t COLS = 4;
  static constexpr uint8_t ROWS = 5;

  CalculatorActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Calculator", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  CalculatorEngine engine;
  // Left operand and operator shown above the display while an operator waits.
  char pendingText[CalculatorEngine::DISPLAY_SIZE] = "";
  uint8_t focusCol = 0;
  uint8_t focusRow = 1;    // the 7 key
  bool showFocus = false;  // button boards always; touch boards after a button press
  bool cleanRefresh = true;
  uint8_t fastRefreshCount = 0;

  int keypadTop() const;
  int keypadBottom() const;
  void keyRect(const Key& key, int& x, int& y, int& w, int& h) const;
  int keyAt(uint8_t col, uint8_t row) const;
  int keyAtPoint(int x, int y) const;
  void moveFocus(int dCol, int dRow);
  void press(const Key& key);
  void drawDisplay() const;
  void drawKey(const Key& key, bool focused) const;
  void drawOperatorGlyph(int cx, int cy, int size, uint8_t op, bool black) const;
  void drawBackspaceGlyph(int cx, int cy, int size, bool black) const;
};
