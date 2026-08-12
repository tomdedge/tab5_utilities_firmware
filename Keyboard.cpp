#include "Keyboard.h"

#include "Theme.h"

#include <M5Unified.h>

#include <cctype>
#include <cstring>

#include "AppManager.h"

void Keyboard::begin(LovyanGFX* gfx, const char* title,
                      const char* initialText, int maxLen, bool maskInput,
                      bool multiline) {
  _gfx = gfx;
  _title = title;
  _maxLen = (maxLen > 0 && maxLen < kMaxLen) ? maxLen : kMaxLen;
  _mask = maskInput;
  _multiline = multiline;
  _done = false;
  _cancelled = false;
  _caps = false;
  _symbolPage = false;

  _len = 0;
  _buffer[0] = 0;
  if (initialText) {
    strncpy(_buffer, initialText, _maxLen);
    _buffer[_maxLen] = 0;
    _len = strlen(_buffer);
  }

  layout();
  _gfx->fillScreen(Theme::kBackground);
}

void Keyboard::appendChar(char c) {
  if (_len >= _maxLen) return;
  _buffer[_len++] = c;
  _buffer[_len] = 0;
}

void Keyboard::backspace() {
  if (_len == 0) return;
  _buffer[--_len] = 0;
}

void Keyboard::layoutRow(int rowIndex, const char* chars, int y, int rowH) {
  int n = (int)strlen(chars);
  if (n > kMaxRowKeys) n = kMaxRowKeys;
  _keyCounts[rowIndex] = n;

  const int padding = 4;
  const int screenW = _gfx->width();
  const int keyW = (screenW - padding * (n + 1)) / n;

  for (int i = 0; i < n; ++i) {
    char c = chars[i];
    _keyChars[rowIndex][i] = c;
    bool upper = _caps && !_symbolPage && isalpha((unsigned char)c);
    char label[2] = {(char)(upper ? toupper(c) : c), 0};
    int x = padding + i * (keyW + padding);
    _keyButtons[rowIndex][i].initButtonUL(_gfx, x, y, keyW, rowH,
                                           Theme::kOutline, Theme::kBackground, Theme::kTextPrimary,
                                           label, 3.2f, 3.2f);
  }
}

void Keyboard::layout() {
  static const char* lettersRows[kRows] = {
      "1234567890",
      "qwertyuiop",
      "asdfghjkl",
      "zxcvbnm",
  };
  static const char* symbolRows[kRows] = {
      "1234567890",
      "!@#$%^&*()",
      "-_=+[]{}",
      ".,;:'\"?/",
  };
  const char** rows = _symbolPage ? symbolRows : lettersRows;

  const int screenW = _gfx->width();
  const int screenH = _gfx->height();
  const int padding = 4;

  // Content starts below AppManager's status bar -- this widget used to
  // draw from y=0, which meant it painted over (and got painted over by)
  // the status bar every frame, a second source of the flashing bug.
  _gridTop = AppManager::kStatusBarHeight + (_multiline ? 220 : 100);
  int gridBottom = screenH - padding;
  int totalRows = kRows + 1;  // +1 for the bottom function row
  _rowH = (gridBottom - _gridTop - padding * totalRows) / totalRows;
  if (_rowH < 30) _rowH = 30;

  for (int r = 0; r < kRows; ++r) {
    int y = _gridTop + r * (_rowH + padding);
    layoutRow(r, rows[r], y, _rowH);
  }

  int bottomY = _gridTop + kRows * (_rowH + padding);
  int x = padding;

  const int capsW = 140;
  _capsBtn.initButtonUL(_gfx, x, bottomY, capsW, _rowH, Theme::kOutline,
                         _caps ? Theme::kPrimary : Theme::kBackground, Theme::kTextPrimary, "CAPS", 2.0f,
                         2.0f);
  x += capsW + padding;

  const int pageW = 110;
  _pageToggleBtn.initButtonUL(_gfx, x, bottomY, pageW, _rowH, Theme::kOutline,
                               Theme::kBackground, Theme::kTextPrimary,
                               _symbolPage ? "ABC" : "123", 2.0f, 2.0f);
  x += pageW + padding;

  const int backW = 140;
  const int doneW = 180;
  int spaceW = screenW - x - padding - backW - padding - doneW - padding;
  if (spaceW < 40) spaceW = 40;

  _spaceBtn.initButtonUL(_gfx, x, bottomY, spaceW, _rowH, Theme::kOutline,
                          Theme::kBackground, Theme::kTextPrimary, "SPACE", 2.0f, 2.0f);
  x += spaceW + padding;

  _backspaceBtn.initButtonUL(_gfx, x, bottomY, backW, _rowH, Theme::kOutline,
                              Theme::kBackground, Theme::kTextPrimary, "DEL", 2.0f, 2.0f);
  x += backW + padding;

  _doneBtn.initButtonUL(_gfx, x, bottomY, doneW, _rowH, Theme::kOutline,
                         Theme::kSuccess, Theme::kTextPrimary, "Done", 2.2f, 2.2f);

  _cancelBtn.initButtonUL(_gfx, screenW - 130 - padding,
                           AppManager::kStatusBarHeight + 8, 130, 54,
                           Theme::kOutline, Theme::kDanger, Theme::kTextPrimary, "Cancel",
                           2.0f, 2.0f);
}

void Keyboard::update() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  for (int r = 0; r < kRows; ++r) {
    for (int i = 0; i < _keyCounts[r]; ++i) {
      auto& btn = _keyButtons[r][i];
      bool inside = pressed && btn.contains(tx, ty);
      btn.press(inside);
      if (btn.justReleased()) {
        char c = _keyChars[r][i];
        if (_caps && !_symbolPage && isalpha((unsigned char)c)) {
          c = (char)toupper(c);
        }
        appendChar(c);
        return;
      }
    }
  }

  bool insideCaps = pressed && _capsBtn.contains(tx, ty);
  _capsBtn.press(insideCaps);
  if (_capsBtn.justReleased()) {
    _caps = !_caps;
    layout();
    return;
  }

  bool insidePage = pressed && _pageToggleBtn.contains(tx, ty);
  _pageToggleBtn.press(insidePage);
  if (_pageToggleBtn.justReleased()) {
    _symbolPage = !_symbolPage;
    layout();
    return;
  }

  bool insideSpace = pressed && _spaceBtn.contains(tx, ty);
  _spaceBtn.press(insideSpace);
  if (_spaceBtn.justReleased()) {
    appendChar(' ');
    return;
  }

  bool insideBack = pressed && _backspaceBtn.contains(tx, ty);
  _backspaceBtn.press(insideBack);
  if (_backspaceBtn.justReleased()) {
    backspace();
    return;
  }

  bool insideDone = pressed && _doneBtn.contains(tx, ty);
  _doneBtn.press(insideDone);
  if (_doneBtn.justReleased()) {
    _done = true;
    return;
  }

  bool insideCancel = pressed && _cancelBtn.contains(tx, ty);
  _cancelBtn.press(insideCancel);
  if (_cancelBtn.justReleased()) {
    _cancelled = true;
    return;
  }
}

void Keyboard::drawPreview() {
  const int top = AppManager::kStatusBarHeight;
  const int previewH = _gridTop - top - 10;
  // Leave the top-right corner (where _cancelBtn lives, 130px wide) alone.
  _gfx->fillRect(0, top, _gfx->width() - 140, previewH, Theme::kBackground);
  _gfx->setTextDatum(top_left);
  _gfx->setTextColor(Theme::kTextSecondary, Theme::kBackground);
  _gfx->setTextSize(2);
  _gfx->drawString(_title, 10, top + 8);

  const char* shown = _buffer;
  if (_mask) {
    for (int i = 0; i < _len; ++i) _shownBuf[i] = '*';
    _shownBuf[_len] = 0;
    shown = _shownBuf;
  }

  _gfx->setTextColor(Theme::kTextPrimary, Theme::kBackground);
  _gfx->setTextSize(3);

  if (_multiline) {
    // Once typed text overflows the visible preview, show only the tail so
    // the cursor position stays visible (word-wrap is left to the library).
    const int kTailChars = 500;
    if (!_mask && _len > kTailChars) shown = _buffer + (_len - kTailChars);

    _gfx->setClipRect(0, top + 44, _gfx->width(), previewH - 44);
    _gfx->setTextWrap(true, true);
    _gfx->setCursor(10, top + 46);
    _gfx->print(shown);
    _gfx->clearClipRect();
  } else {
    _gfx->drawString(shown, 10, top + 52);
  }
}

void Keyboard::draw() {
  drawPreview();

  for (int r = 0; r < kRows; ++r) {
    for (int i = 0; i < _keyCounts[r]; ++i) {
      _keyButtons[r][i].drawButton();
    }
  }
  _capsBtn.drawButton();
  _pageToggleBtn.drawButton();
  _spaceBtn.drawButton();
  _backspaceBtn.drawButton();
  _doneBtn.drawButton();
  _cancelBtn.drawButton();
}
