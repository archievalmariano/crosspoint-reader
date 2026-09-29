#pragma once

class GfxRenderer;

// Display-sized numerals drawn from solid bars (seven segments), for the Clock
// time and the Calculator display. Supports 0-9, '-', '.' and ':'; other
// characters are skipped. No font asset: every bar is a filled rectangle.
namespace segment_digits {

int textWidth(const char* text, int height);
// Draws `text` with its top-left at (x, y), `height` pixels tall.
void draw(const GfxRenderer& renderer, int x, int y, int height, const char* text);

}  // namespace segment_digits
