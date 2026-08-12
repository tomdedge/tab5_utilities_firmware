#pragma once

#include <M5GFX.h>

// Reusable modal on-screen QWERTY keyboard. Not an App itself -- any App that
// needs text input owns a Keyboard instance, calls begin() to open it, then
// routes its own update()/draw() to the keyboard's while it's active, and
// checks isDone()/isCancelled() to know when to read text() and close it.
//
// Usage:
//   _keyboard.begin(&M5.Display, "Wi-Fi Password", "", 32, /*mask=*/true);
//   // each frame while active:
//   _keyboard.update();
//   if (_keyboard.isDone()) { save(_keyboard.text()); closeKeyboard(); }
//   else if (_keyboard.isCancelled()) { closeKeyboard(); }
//   _keyboard.draw();
//
// Pass multiline=true (e.g. for note-taking) to get a taller, word-wrapped
// preview area; once typed text overflows the visible preview, only the
// tail end is shown (so the cursor position stays visible), though the
// full buffer is always what's returned by text().
class Keyboard {
 public:
  static constexpr int kMaxLen = 4095;

  void begin(LovyanGFX* gfx, const char* title, const char* initialText,
             int maxLen, bool maskInput, bool multiline = false);

  void update();
  void draw();

  bool isDone() const { return _done; }
  bool isCancelled() const { return _cancelled; }
  const char* text() const { return _buffer; }

 private:
  static constexpr int kRows = 4;
  static constexpr int kMaxRowKeys = 10;

  void layout();
  void layoutRow(int rowIndex, const char* chars, int y, int rowH);
  void appendChar(char c);
  void backspace();
  void drawPreview();

  LovyanGFX* _gfx = nullptr;
  const char* _title = "";
  char _buffer[kMaxLen + 1] = {0};
  char _shownBuf[kMaxLen + 1] = {0};
  int _len = 0;
  int _maxLen = kMaxLen;
  bool _mask = false;
  bool _multiline = false;
  bool _done = false;
  bool _cancelled = false;
  bool _caps = false;
  bool _symbolPage = false;

  int _gridTop = 0;
  int _rowH = 0;

  LGFX_Button _keyButtons[kRows][kMaxRowKeys];
  char _keyChars[kRows][kMaxRowKeys];
  int _keyCounts[kRows] = {0};

  LGFX_Button _capsBtn, _pageToggleBtn, _spaceBtn, _backspaceBtn, _doneBtn,
      _cancelBtn;
};
