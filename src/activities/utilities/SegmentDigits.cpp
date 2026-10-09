#include "SegmentDigits.h"

#include <GfxRenderer.h>

#include <algorithm>
#include <cstdint>

namespace segment_digits {
namespace {

// Bits: a (top), b (top right), c (bottom right), d (bottom), e (bottom left),
// f (top left), g (middle).
constexpr uint8_t A = 1, B = 2, C = 4, D = 8, E = 16, F = 32, G = 64;
constexpr uint8_t DIGITS[10] = {
    A | B | C | D | E | F, B | C,         A | B | G | E | D,
    A | B | G | C | D,     F | G | B | C, A | F | G | C | D,
    A | F | G | E | C | D, A | B | C,     A | B | C | D | E | F | G,
    A | B | C | D | F | G,
};

struct Metrics {
  int digitWidth;
  int stroke;
  int gap;  // between characters
};

Metrics metricsFor(const int height) {
  const int stroke = std::max(3, height / 8);
  return {height * 11 / 20, stroke, std::max(3, stroke * 2 / 3)};
}

int charWidth(const char c, const Metrics& m) {
  if (c >= '0' && c <= '9') return m.digitWidth;
  if (c == '-') return m.digitWidth;
  if (c == '.' || c == ':') return m.stroke;
  return -1;  // unsupported: skipped
}

void drawDigitBars(const GfxRenderer& renderer, const int x, const int y, const int height, const uint8_t bars,
                   const Metrics& m) {
  const int w = m.digitWidth;
  const int t = m.stroke;
  const int half = height / 2;
  const int notch = std::max(1, t / 3);  // small breaks keep the bars legible as separate strokes
  if (bars & A) renderer.fillRect(x + notch, y, w - 2 * notch, t, true);
  if (bars & G) renderer.fillRect(x + notch, y + half - t / 2, w - 2 * notch, t, true);
  if (bars & D) renderer.fillRect(x + notch, y + height - t, w - 2 * notch, t, true);
  if (bars & F) renderer.fillRect(x, y + notch, t, half - 2 * notch, true);
  if (bars & B) renderer.fillRect(x + w - t, y + notch, t, half - 2 * notch, true);
  if (bars & E) renderer.fillRect(x, y + half + notch, t, height - half - 2 * notch, true);
  if (bars & C) renderer.fillRect(x + w - t, y + half + notch, t, height - half - 2 * notch, true);
}

}  // namespace

int textWidth(const char* text, const int height) {
  const Metrics m = metricsFor(height);
  int width = 0;
  int count = 0;
  for (const char* c = text; *c != '\0'; ++c) {
    const int cw = charWidth(*c, m);
    if (cw < 0) continue;
    width += cw;
    ++count;
  }
  return count > 0 ? width + (count - 1) * m.gap : 0;
}

void draw(const GfxRenderer& renderer, int x, const int y, const int height, const char* text) {
  const Metrics m = metricsFor(height);
  for (const char* c = text; *c != '\0'; ++c) {
    const int cw = charWidth(*c, m);
    if (cw < 0) continue;
    if (*c >= '0' && *c <= '9') {
      drawDigitBars(renderer, x, y, height, DIGITS[*c - '0'], m);
    } else if (*c == '-') {
      drawDigitBars(renderer, x, y, height, G, m);
    } else if (*c == '.') {
      renderer.fillRect(x, y + height - m.stroke, m.stroke, m.stroke, true);
    } else if (*c == ':') {
      renderer.fillRect(x, y + height / 4, m.stroke, m.stroke, true);
      renderer.fillRect(x, y + height * 3 / 4 - m.stroke, m.stroke, m.stroke, true);
    }
    x += cw + m.gap;
  }
}

}  // namespace segment_digits
