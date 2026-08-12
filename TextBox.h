#pragma once

#include <M5GFX.h>

#include "Theme.h"

// Draws word-wrapped, read-only text inside (x,y,w,h), clipped to that box
// so overflow is simply cut off rather than bleeding into whatever's drawn
// next to it. Same clip-rect + wrap + cursor technique as
// Keyboard::drawPreview()'s multiline mode, factored out for screens that
// just need to *show* text rather than edit it (Word Lookup, News).
//
// color/bg must stay uint32_t (RGB888) -- LGFXBase's color setters are
// templated on color type and a plain `int`-typed TFT_* macro here would
// be reinterpreted as RGB565 instead of RGB888 by convert_to_rgb888's
// overload resolution (this genuinely happened with the old TFT_WHITE/
// TFT_BLACK defaults: they rendered as near-black/dark-blue, not white).
void drawWrappedText(LovyanGFX* gfx, int x, int y, int w, int h,
                      const char* text, int textSize = 2,
                      uint32_t color = Theme::kTextPrimary,
                      uint32_t bg = Theme::kBackground);
