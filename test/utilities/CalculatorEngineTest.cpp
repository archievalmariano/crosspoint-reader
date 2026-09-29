#include <gtest/gtest.h>

#include <cstring>

#include "activities/utilities/calculator/CalculatorEngine.h"

namespace {

using Op = CalculatorEngine::Op;

// Feeds keys: digits, '.', + - * /, '=', 'C' (clear), '<' (backspace).
const char* run(CalculatorEngine& calc, const char* keys) {
  for (const char* key = keys; *key != '\0'; ++key) {
    switch (*key) {
      case '+':
        calc.op(Op::Add);
        break;
      case '-':
        calc.op(Op::Subtract);
        break;
      case '*':
        calc.op(Op::Multiply);
        break;
      case '/':
        calc.op(Op::Divide);
        break;
      case '=':
        calc.equals();
        break;
      case '.':
        calc.decimalPoint();
        break;
      case 'C':
        calc.clear();
        break;
      case '<':
        calc.backspace();
        break;
      default:
        calc.digit(static_cast<uint8_t>(*key - '0'));
    }
  }
  return calc.display();
}

const char* eval(const char* keys) {
  static CalculatorEngine calc;
  calc.clear();
  return run(calc, keys);
}

TEST(CalculatorEngine, StartsAtZero) {
  CalculatorEngine calc;
  EXPECT_STREQ(calc.display(), "0");
  EXPECT_EQ(calc.pendingOp(), Op::None);
}

TEST(CalculatorEngine, RunsOperatorsLeftToRight) {
  EXPECT_STREQ(eval("2+3*4="), "20");  // desk calculator, not 14
  EXPECT_STREQ(eval("10-2/4="), "2");
}

TEST(CalculatorEngine, ShowsRunningResultWhenAnOperatorIsPressed) {
  CalculatorEngine calc;
  EXPECT_STREQ(run(calc, "2+3*"), "5");
  EXPECT_EQ(calc.pendingOp(), Op::Multiply);
}

TEST(CalculatorEngine, LastOperatorPressedWins) {
  EXPECT_STREQ(eval("8+*2="), "16");
  EXPECT_STREQ(eval("8*-2="), "6");
}

TEST(CalculatorEngine, StartingWithAnOperatorUsesZero) { EXPECT_STREQ(eval("-5="), "-5"); }

TEST(CalculatorEngine, EqualsWithoutSecondOperandKeepsTheValue) { EXPECT_STREQ(eval("7*="), "7"); }

TEST(CalculatorEngine, ResultCanBeContinued) {
  EXPECT_STREQ(eval("2+3=*10="), "50");
  EXPECT_STREQ(eval("2+3=9"), "9");  // a digit after a result starts a new number
}

TEST(CalculatorEngine, Decimals) {
  EXPECT_STREQ(eval("1.5+2.25="), "3.75");
  EXPECT_STREQ(eval(".5"), "0.5");
  EXPECT_STREQ(eval("1..2"), "1.2");  // a second point is ignored
  EXPECT_STREQ(eval("0.1+0.2="), "0.3");
  EXPECT_STREQ(eval("1/3="), "0.33333333333");
  EXPECT_STREQ(eval("2/3="), "0.66666666667");
  EXPECT_STREQ(eval("1/3=*3="), "1");  // full precision carries between steps
}

TEST(CalculatorEngine, TrailingPointAndZerosAreDroppedFromResults) {
  EXPECT_STREQ(eval("5.="), "5");
  EXPECT_STREQ(eval("2.50*2="), "5");
}

TEST(CalculatorEngine, LeadingZerosCollapse) { EXPECT_STREQ(eval("0007"), "7"); }

TEST(CalculatorEngine, DivideByZeroIsError) {
  CalculatorEngine calc;
  EXPECT_STREQ(run(calc, "5/0="), "Error");
  EXPECT_TRUE(calc.hasError());
  EXPECT_STREQ(run(calc, "+="), "Error");  // operators do nothing while in error
  EXPECT_STREQ(run(calc, "4"), "4");       // a digit starts over
  EXPECT_FALSE(calc.hasError());
}

TEST(CalculatorEngine, ChainedDivideByZeroIsError) { EXPECT_STREQ(eval("6/0+"), "Error"); }

TEST(CalculatorEngine, EntryIsLimitedToTwelveDigits) {
  EXPECT_STREQ(eval("1234567890123"), "123456789012");
  EXPECT_STREQ(eval("12345678901.23"), "12345678901.2");
  EXPECT_STREQ(eval("123456789012."), "123456789012");  // no room for a decimal digit
}

TEST(CalculatorEngine, TooLargeResultIsError) {
  EXPECT_STREQ(eval("999999999999+1="), "Error");
  EXPECT_STREQ(eval("999999999999*999999999999="), "Error");
  EXPECT_STREQ(eval("999999999999+0="), "999999999999");
}

TEST(CalculatorEngine, TinyResultsRoundToTwelveDigits) {
  EXPECT_STREQ(eval("1/100000000000="), "0.00000000001");
  EXPECT_STREQ(eval("0.00000000001/10="), "0");
}

TEST(CalculatorEngine, NegativeResults) {
  EXPECT_STREQ(eval("3-10="), "-7");
  EXPECT_STREQ(eval("0-0.5="), "-0.5");
}

TEST(CalculatorEngine, BackspaceEditsOnlyTypedNumbers) {
  EXPECT_STREQ(eval("123<"), "12");
  EXPECT_STREQ(eval("1<"), "0");
  EXPECT_STREQ(eval("1.5<"), "1.");
  EXPECT_STREQ(eval("2+3=<"), "5");  // results are not editable
}

TEST(CalculatorEngine, BackspaceClearsAnError) {
  CalculatorEngine calc;
  run(calc, "1/0=");
  calc.backspace();
  EXPECT_FALSE(calc.hasError());
  EXPECT_STREQ(calc.display(), "0");
}

TEST(CalculatorEngine, ClearResetsEverything) {
  CalculatorEngine calc;
  run(calc, "5+3");
  calc.clear();
  EXPECT_STREQ(calc.display(), "0");
  EXPECT_EQ(calc.pendingOp(), Op::None);
  EXPECT_STREQ(run(calc, "4="), "4");
}

}  // namespace
