#include "TextBox.h"

void drawWrappedText(LovyanGFX* gfx, int x, int y, int w, int h,
                      const char* text, int textSize, uint32_t color,
                      uint32_t bg) {
  gfx->fillRect(x, y, w, h, bg);
  gfx->setTextDatum(top_left);
  gfx->setTextColor(color, bg);
  gfx->setTextSize(textSize);
  gfx->setClipRect(x, y, w, h);
  gfx->setTextWrap(true, true);
  gfx->setCursor(x, y);
  gfx->print(text);
  gfx->clearClipRect();
}
