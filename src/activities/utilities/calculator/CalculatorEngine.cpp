#include "CalculatorEngine.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
constexpr char ERROR_TEXT[] = "Error";
}  // namespace

size_t CalculatorEngine::digitCount() const {
  size_t count = 0;
  for (const char* c = text; *c != '\0'; ++c) count += (*c >= '0' && *c <= '9') ? 1 : 0;
  return count;
}

void CalculatorEngine::clear() {
  strcpy(text, "0");
  accumulator = 0;
  pending = Op::None;
  typing = false;
  error = false;
}

void CalculatorEngine::startTyping() {
  if (error) clear();
  if (!typing) {
    strcpy(text, "0");
    typing = true;
  }
}

void CalculatorEngine::digit(const uint8_t value) {
  if (value > 9) return;
  startTyping();
  const size_t length = strlen(text);
  if (length == 1 && text[0] == '0') {
    text[0] = static_cast<char>('0' + value);
    return;
  }
  if (digitCount() >= MAX_DIGITS) return;
  text[length] = static_cast<char>('0' + value);
  text[length + 1] = '\0';
}

void CalculatorEngine::decimalPoint() {
  startTyping();
  if (strchr(text, '.') != nullptr || digitCount() >= MAX_DIGITS) return;
  const size_t length = strlen(text);
  text[length] = '.';
  text[length + 1] = '\0';
}

void CalculatorEngine::backspace() {
  if (error) {
    clear();
    return;
  }
  if (!typing) return;  // results are not editable
  const size_t length = strlen(text);
  if (length <= 1) {
    strcpy(text, "0");
    return;
  }
  text[length - 1] = '\0';
}

// Rounds to at most MAX_DIGITS significant digits without exponent notation.
// Error when the integer part alone needs more digits.
void CalculatorEngine::showValue(const double value) {
  const double magnitude = std::fabs(value);
  size_t integerDigits = 1;
  for (double limit = 10; magnitude >= limit && integerDigits <= MAX_DIGITS; limit *= 10) ++integerDigits;
  if (integerDigits > MAX_DIGITS || !std::isfinite(value)) {
    strcpy(text, ERROR_TEXT);
    error = true;
    return;
  }
  char formatted[40];
  snprintf(formatted, sizeof(formatted), "%.*f", static_cast<int>(MAX_DIGITS - integerDigits), value);
  if (strchr(formatted, '.') != nullptr) {
    size_t end = strlen(formatted);
    while (end > 0 && formatted[end - 1] == '0') formatted[--end] = '\0';
    if (end > 0 && formatted[end - 1] == '.') formatted[--end] = '\0';
  }
  if (strcmp(formatted, "-0") == 0) strcpy(formatted, "0");
  size_t digits = 0;
  for (const char* c = formatted; *c != '\0'; ++c) digits += (*c >= '0' && *c <= '9') ? 1 : 0;
  if (digits > MAX_DIGITS) {  // rounding carried into a new integer digit
    strcpy(text, ERROR_TEXT);
    error = true;
    return;
  }
  strcpy(text, formatted);
}

bool CalculatorEngine::applyPending() {
  const double operand = strtod(text, nullptr);
  switch (pending) {
    case Op::None:
      accumulator = operand;
      break;
    case Op::Add:
      accumulator += operand;
      break;
    case Op::Subtract:
      accumulator -= operand;
      break;
    case Op::Multiply:
      accumulator *= operand;
      break;
    case Op::Divide:
      if (operand == 0) {
        strcpy(text, ERROR_TEXT);
        error = true;
        return false;
      }
      accumulator /= operand;
      break;
  }
  showValue(accumulator);
  return !error;
}

void CalculatorEngine::op(const Op next) {
  if (error || next == Op::None) return;
  // A shown result is already the accumulator; only a typed number applies.
  if (typing && !applyPending()) return;
  typing = false;
  pending = next;
}

void CalculatorEngine::equals() {
  if (error) return;
  if (typing && !applyPending()) return;
  typing = false;
  pending = Op::None;
}
