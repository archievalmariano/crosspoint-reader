#pragma once

#include <cstddef>
#include <cstdint>

// Basic desk-calculator logic (no device deps, host-testable). Operators run
// immediately, left to right: 2 + 3 x 4 = 20. No precedence, memory, history
// or scientific functions. Numbers show at most MAX_DIGITS digits; results
// that need more integer digits, and division by zero, show Error.
class CalculatorEngine {
 public:
  enum class Op : uint8_t { None, Add, Subtract, Multiply, Divide };

  static constexpr size_t MAX_DIGITS = 12;
  // Display text: optional '-', MAX_DIGITS digits, optional '.', terminator.
  static constexpr size_t DISPLAY_SIZE = MAX_DIGITS + 3;

  void digit(uint8_t value);
  void decimalPoint();
  void op(Op next);
  void equals();
  void clear();
  void backspace();

  // Text for the main display: the number being typed, or the last result.
  const char* display() const { return text; }
  bool hasError() const { return error; }
  // The operator waiting for its second operand (Op::None when there is none).
  Op pendingOp() const { return pending; }

 private:
  char text[DISPLAY_SIZE] = "0";
  double accumulator = 0;
  Op pending = Op::None;
  bool typing = false;  // text holds a number being typed, not a result
  bool error = false;

  void startTyping();
  void showValue(double value);
  bool applyPending();  // false on error (shown)
  size_t digitCount() const;
};
