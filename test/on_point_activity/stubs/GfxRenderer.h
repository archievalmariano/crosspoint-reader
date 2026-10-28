#pragma once
// Host-test stand-in: records drawn text and rectangles.
#include <cstring>
#include <string>
#include <vector>

namespace EpdFontFamily {
enum Style { REGULAR = 0, BOLD = 1, ITALIC = 2, BOLD_ITALIC = 3 };
}

struct DrawnText {
  int font;
  int x;
  int y;
  std::string text;
};

struct DrawnRect {
  int x;
  int y;
  int width;
  int height;
};

class GfxRenderer {
 public:
  int width = 480;
  std::vector<DrawnText> texts;
  std::vector<DrawnRect> rects;
  int displayCount = 0;
  int lastMode = -1;

  int getScreenWidth() const { return width; }
  int getTextWidth(int, const char* text, int = EpdFontFamily::REGULAR) const {
    return 10 * static_cast<int>(strlen(text));
  }
  void drawText(int font, int x, int y, const char* text, bool = true, int = EpdFontFamily::REGULAR) {
    texts.push_back({font, x, y, text});
  }
  template <typename... Args>
  void drawLine(Args...) {}
  void fillRect(int x, int y, int w, int h, bool = true) { rects.push_back({x, y, w, h}); }
  template <typename... Args>
  void fillPolygon(Args...) {}
  void clearScreen() {
    texts.clear();
    rects.clear();
  }
  template <typename Mode>
  void displayBuffer(Mode mode) {
    ++displayCount;
    lastMode = static_cast<int>(mode);
  }
};
