#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

// DOGEAR cover text (pure, host-testable). A cover title over its line limit is
// cut at a word boundary with the ellipsis right after the last kept word: never
// mid-word ("Titl…") and never after a space ("Volume …"). The same rule as
// ellipsize() in dogear's mock/render_dogear.py, the approved reference. Only a
// single word wider than the whole line is cut by characters (UTF-8 safe).
namespace dogear_text {

inline constexpr char kEllipsis[] = "\xE2\x80\xA6";  // U+2026

// `lines` is the title fully wrapped (no line limit); `fits(s)` says whether `s`
// fits the line width in the title's font. Returns at most `maxLines` lines.
template <class Fits>
std::vector<std::string> ellipsizeAtWord(std::vector<std::string> lines, size_t maxLines, Fits fits) {
  if (lines.size() <= maxLines || maxLines == 0) {
    if (maxLines == 0) lines.clear();
    return lines;
  }
  lines.resize(maxLines);
  std::string last = std::move(lines.back());
  const auto trimRight = [](std::string& s) {
    while (!s.empty() && s.back() == ' ') s.pop_back();
  };
  trimRight(last);
  while (!last.empty() && !fits(last + kEllipsis)) {
    const size_t space = last.rfind(' ');
    if (space != std::string::npos) {
      last.erase(space);
      trimRight(last);
    } else {  // one word wider than the line: drop whole UTF-8 characters
      size_t cut = last.size() - 1;
      while (cut > 0 && (static_cast<unsigned char>(last[cut]) & 0xC0) == 0x80) --cut;
      last.erase(cut);
    }
  }
  lines.back() = last + kEllipsis;
  return lines;
}

}  // namespace dogear_text
