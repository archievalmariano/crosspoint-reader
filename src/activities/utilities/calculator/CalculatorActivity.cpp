#include "CalculatorActivity.h"

#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <I18n.h>

#include <cstdio>
#include <cstring>

#include "MappedInputManager.h"
#include "activities/utilities/SegmentDigits.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
using Op = CalculatorEngine::Op;
using Key = CalculatorActivity::Key;
using KeyType = CalculatorActivity::KeyType;

constexpr int MARGIN = 24;
constexpr int HEADER_TITLE_Y = 8;
constexpr int HEADER_RULE_Y = 50;
constexpr int HEADER_FONT = NOTOSANS_16_FONT_ID;
constexpr int LABEL_FONT = UI_10_FONT_ID;
constexpr int PENDING_Y = 74;
constexpr int NUMBER_TOP = 118;
constexpr int NUMBER_MAX_HEIGHT = 110;
constexpr int DISPLAY_RULE_Y = 272;
constexpr int KEYPAD_GAP_BELOW_RULE = 24;
constexpr int KEY_GAP = 8;
constexpr int KEY_FONT = NOTOSANS_18_FONT_ID;
// Key presses use fast refreshes. A clean refresh clears their ghosting only
// on entry, on C, or after a long run of presses, so typing never flashes.
constexpr uint8_t HALF_REFRESH_AFTER_FAST = 30;

constexpr uint8_t op(const Op value) { return static_cast<uint8_t>(value); }

// 4 x 5 keypad; 0 spans two columns and = spans two rows.
constexpr Key KEYS[] = {
    {0, 0, 1, 1, KeyType::Clear, 0},
    {1, 0, 1, 1, KeyType::Backspace, 0},
    {2, 0, 1, 1, KeyType::Operator, op(Op::Divide)},
    {3, 0, 1, 1, KeyType::Operator, op(Op::Multiply)},
    {0, 1, 1, 1, KeyType::Digit, 7},
    {1, 1, 1, 1, KeyType::Digit, 8},
    {2, 1, 1, 1, KeyType::Digit, 9},
    {3, 1, 1, 1, KeyType::Operator, op(Op::Subtract)},
    {0, 2, 1, 1, KeyType::Digit, 4},
    {1, 2, 1, 1, KeyType::Digit, 5},
    {2, 2, 1, 1, KeyType::Digit, 6},
    {3, 2, 1, 1, KeyType::Operator, op(Op::Add)},
    {0, 3, 1, 1, KeyType::Digit, 1},
    {1, 3, 1, 1, KeyType::Digit, 2},
    {2, 3, 1, 1, KeyType::Digit, 3},
    {3, 3, 1, 2, KeyType::Equals, 0},
    {0, 4, 2, 1, KeyType::Digit, 0},
    {2, 4, 1, 1, KeyType::Point, 0},
};
constexpr int KEY_COUNT = sizeof(KEYS) / sizeof(KEYS[0]);
}  // namespace

void CalculatorActivity::onEnter() {
  Activity::onEnter();
  showFocus = !mappedInput.hasTouch();
  cleanRefresh = true;
  requestUpdate();
}

int CalculatorActivity::keypadTop() const { return DISPLAY_RULE_Y + KEYPAD_GAP_BELOW_RULE; }

int CalculatorActivity::keypadBottom() const {
  // Button boards reserve the hint bar; touch boards use the full height.
  const int hints = mappedInput.hasTouch() ? 0 : UITheme::getInstance().getMetrics().buttonHintsHeight;
  return renderer.getScreenHeight() - hints - 16;
}

void CalculatorActivity::keyRect(const Key& key, int& x, int& y, int& w, int& h) const {
  const int width = renderer.getScreenWidth() - 2 * MARGIN;
  const int height = keypadBottom() - keypadTop();
  const int cellW = (width - (COLS - 1) * KEY_GAP) / COLS;
  const int cellH = (height - (ROWS - 1) * KEY_GAP) / ROWS;
  x = MARGIN + key.col * (cellW + KEY_GAP);
  y = keypadTop() + key.row * (cellH + KEY_GAP);
  w = key.colSpan * cellW + (key.colSpan - 1) * KEY_GAP;
  h = key.rowSpan * cellH + (key.rowSpan - 1) * KEY_GAP;
}

int CalculatorActivity::keyAt(const uint8_t col, const uint8_t row) const {
  for (int i = 0; i < KEY_COUNT; ++i) {
    const Key& key = KEYS[i];
    if (col >= key.col && col < key.col + key.colSpan && row >= key.row && row < key.row + key.rowSpan) return i;
  }
  return -1;
}

// Hit areas fill the gaps between keys, and keys on the keypad's outer edge
// extend to the screen edge, so a tap near a key always lands on it.
int CalculatorActivity::keyAtPoint(const int px, const int py) const {
  for (int i = 0; i < KEY_COUNT; ++i) {
    const Key& key = KEYS[i];
    int x, y, w, h;
    keyRect(key, x, y, w, h);
    const int left = key.col == 0 ? 0 : x - KEY_GAP / 2;
    const int right = key.col + key.colSpan == COLS ? renderer.getScreenWidth() : x + w + KEY_GAP / 2;
    const int top = y - KEY_GAP / 2;
    const int bottom = key.row + key.rowSpan == ROWS ? renderer.getScreenHeight() : y + h + KEY_GAP / 2;
    if (px >= left && px < right && py >= top && py < bottom) return i;
  }
  return -1;
}

// Steps cell by cell, skipping the rest of a key that spans several cells, and
// wraps around the keypad edges.
void CalculatorActivity::moveFocus(const int dCol, const int dRow) {
  const int start = keyAt(focusCol, focusRow);
  int col = focusCol;
  int row = focusRow;
  for (int step = 0; step < COLS * ROWS; ++step) {
    col = (col + dCol + COLS) % COLS;
    row = (row + dRow + ROWS) % ROWS;
    if (keyAt(col, row) != start) break;
  }
  const Key& landed = KEYS[keyAt(col, row)];
  focusCol = landed.col;
  focusRow = landed.row;
}

void CalculatorActivity::press(const Key& key) {
  switch (key.type) {
    case KeyType::Digit:
      engine.digit(key.value);
      break;
    case KeyType::Point:
      engine.decimalPoint();
      break;
    case KeyType::Operator:
      engine.op(static_cast<Op>(key.value));
      break;
    case KeyType::Equals:
      engine.equals();
      break;
    case KeyType::Clear:
      engine.clear();
      cleanRefresh = true;
      break;
    case KeyType::Backspace:
      engine.backspace();
      break;
  }
  // The left operand shown above the display is the result the operator
  // produced; it goes once the operator has been applied or cleared.
  if (key.type == KeyType::Operator && engine.pendingOp() != Op::None && !engine.hasError()) {
    strncpy(pendingText, engine.display(), sizeof(pendingText) - 1);
    pendingText[sizeof(pendingText) - 1] = '\0';
  } else if (engine.pendingOp() == Op::None || engine.hasError()) {
    pendingText[0] = '\0';
  }
  requestUpdate();
}

void CalculatorActivity::loop() {
  using Button = MappedInputManager::Button;
  if (mappedInput.wasHomeGesture() || mappedInput.wasHomeKeyHold()) {
    activityManager.goHome();
    return;
  }
  if (mappedInput.wasReleased(Button::Back)) {
    activityManager.goToApps(AppId::Calculator);
    return;
  }
  // Every key counts: a press during a refresh waits for the render lock, then
  // applies, rather than being dropped (quick digit-then-operator input).

  int tx = 0;
  int ty = 0;
  if (mappedInput.wasScreenTapped(tx, ty)) {
    const int index = keyAtPoint(tx, ty);
    if (index >= 0) {
      showFocus = false;
      focusCol = KEYS[index].col;
      focusRow = KEYS[index].row;
      RenderLock lock(*this);
      press(KEYS[index]);
    }
    return;
  }
  if (mappedInput.wasReleased(Button::Confirm)) {
    showFocus = true;
    RenderLock lock(*this);
    press(KEYS[keyAt(focusCol, focusRow)]);
    return;
  }
  int dCol = 0;
  int dRow = 0;
  if (mappedInput.wasPressed(Button::Left)) dCol = -1;
  if (mappedInput.wasPressed(Button::Right)) dCol = 1;
  if (mappedInput.wasPressed(Button::Up)) dRow = -1;
  if (mappedInput.wasPressed(Button::Down)) dRow = 1;
  if (dCol != 0 || dRow != 0) {
    RenderLock lock(*this);
    if (showFocus) moveFocus(dCol, dRow);
    showFocus = true;
    requestUpdate();
  }
}

// Operators are drawn from bars rather than font glyphs: they stay bold at any
// size and do not depend on the font carrying × ÷ −.
void CalculatorActivity::drawOperatorGlyph(const int cx, const int cy, const int size, const uint8_t value,
                                           const bool black) const {
  const int t = size / 6 > 3 ? size / 6 : 3;
  const int half = size / 2;
  switch (static_cast<Op>(value)) {
    case Op::Add:
      renderer.fillRect(cx - half, cy - t / 2, size, t, black);
      renderer.fillRect(cx - t / 2, cy - half, t, size, black);
      break;
    case Op::Subtract:
      renderer.fillRect(cx - half, cy - t / 2, size, t, black);
      break;
    case Op::Multiply: {
      const int arm = half * 4 / 5;
      renderer.drawLine(cx - arm, cy - arm, cx + arm, cy + arm, t, black);
      renderer.drawLine(cx - arm, cy + arm, cx + arm, cy - arm, t, black);
      break;
    }
    case Op::Divide:
      renderer.fillRect(cx - half, cy - t / 2, size, t, black);
      renderer.fillRect(cx - t / 2 - 1, cy - half, t + 2, t + 2, black);
      renderer.fillRect(cx - t / 2 - 1, cy + half - t - 2, t + 2, t + 2, black);
      break;
    case Op::None:
      break;
  }
}

void CalculatorActivity::drawBackspaceGlyph(const int cx, const int cy, const int size, const bool black) const {
  const int t = size / 7 > 3 ? size / 7 : 3;
  const int half = size / 2;
  renderer.drawLine(cx - half, cy, cx + half, cy, t, black);
  renderer.drawLine(cx - half, cy, cx - half + size / 3, cy - size / 3, t, black);
  renderer.drawLine(cx - half, cy, cx - half + size / 3, cy + size / 3, t, black);
}

void CalculatorActivity::drawKey(const Key& key, const bool focused) const {
  int x, y, w, h;
  keyRect(key, x, y, w, h);
  const bool active = key.type == KeyType::Operator && engine.pendingOp() == static_cast<Op>(key.value);
  const bool solid = (key.type == KeyType::Operator || key.type == KeyType::Equals) && !active;
  if (solid) {
    renderer.fillRect(x, y, w, h, true);
  } else {
    renderer.drawRect(x, y, w, h, key.type == KeyType::Digit || key.type == KeyType::Point ? 2 : 4, true);
  }
  // The operator waiting for its second number reads as a hollow, heavy key.
  if (active) renderer.drawRect(x + 6, y + 6, w - 12, h - 12, 2, true);
  if (focused) renderer.drawRect(x - 5, y - 5, w + 10, h + 10, 3, true);

  const int cx = x + w / 2;
  const int cy = y + h / 2;
  const int glyph = h < w ? h / 3 : w / 3;
  switch (key.type) {
    case KeyType::Operator:
      drawOperatorGlyph(cx, cy, glyph, key.value, !solid);
      break;
    case KeyType::Equals: {
      const int t = glyph / 6 > 3 ? glyph / 6 : 3;
      renderer.fillRect(cx - glyph / 2, cy - t - t / 2 - 2, glyph, t, false);
      renderer.fillRect(cx - glyph / 2, cy + t / 2 + 2, glyph, t, false);
      break;
    }
    case KeyType::Backspace:
      drawBackspaceGlyph(cx, cy, glyph, true);
      break;
    default: {
      char label[2] = {key.type == KeyType::Digit   ? static_cast<char>('0' + key.value)
                       : key.type == KeyType::Point ? '.'
                                                    : 'C',
                       '\0'};
      const int textW = renderer.getTextWidth(KEY_FONT, label, EpdFontFamily::BOLD);
      renderer.drawText(KEY_FONT, cx - textW / 2, cy - renderer.getLineHeight(KEY_FONT) / 2, label, true,
                        EpdFontFamily::BOLD);
      break;
    }
  }
}

void CalculatorActivity::drawDisplay() const {
  const int width = renderer.getScreenWidth();
  const int right = width - MARGIN;
  if (pendingText[0] != '\0') {
    // Annotation: the left operand and the operator waiting for the next number.
    const int textW = renderer.getTextWidth(LABEL_FONT, pendingText, EpdFontFamily::BOLD);
    constexpr int GLYPH = 14;
    renderer.drawText(LABEL_FONT, right - GLYPH - 10 - textW, PENDING_Y, pendingText, true, EpdFontFamily::BOLD);
    drawOperatorGlyph(right - GLYPH / 2, PENDING_Y + renderer.getLineHeight(LABEL_FONT) / 2, GLYPH,
                      static_cast<uint8_t>(engine.pendingOp()), true);
  }
  if (engine.hasError()) {
    const char* error = tr(STR_CALCULATOR_ERROR);
    renderer.drawText(NOTOSANS_18_FONT_ID,
                      right - renderer.getTextWidth(NOTOSANS_18_FONT_ID, error, EpdFontFamily::BOLD),
                      NUMBER_TOP + NUMBER_MAX_HEIGHT / 3, error, true, EpdFontFamily::BOLD);
    return;
  }
  const char* number = engine.display();
  int height = NUMBER_MAX_HEIGHT;
  while (height > 30 && segment_digits::textWidth(number, height) > width - 2 * MARGIN) height -= 4;
  segment_digits::draw(renderer, right - segment_digits::textWidth(number, height),
                       NUMBER_TOP + NUMBER_MAX_HEIGHT - height, height, number);
}

void CalculatorActivity::render(RenderLock&&) {
  renderer.clearScreen();
  const int width = renderer.getScreenWidth();
  renderer.drawText(HEADER_FONT, MARGIN, HEADER_TITLE_Y, tr(STR_CALCULATOR), true, EpdFontFamily::BOLD);
  renderer.drawLine(MARGIN, HEADER_RULE_Y, width - MARGIN, HEADER_RULE_Y, 5, true);

  drawDisplay();
  renderer.drawLine(MARGIN, DISPLAY_RULE_Y, width - MARGIN, DISPLAY_RULE_Y, 5, true);

  const int focused = showFocus ? keyAt(focusCol, focusRow) : -1;
  for (int i = 0; i < KEY_COUNT; ++i) drawKey(KEYS[i], i == focused);

  const auto labels =
      mappedInput.mapLabels(tr(STR_BACK), tr(STR_CALCULATOR_PRESS), tr(STR_DIR_LEFT), tr(STR_DIR_RIGHT));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

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
