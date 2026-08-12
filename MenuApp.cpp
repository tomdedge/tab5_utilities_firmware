#include "MenuApp.h"

#include "Theme.h"

#include <M5Unified.h>

#include <cstring>

#include "AppManager.h"

void MenuApp::begin(AppManager* mgr, App** apps, int count) {
  _mgr = mgr;
  _apps = apps;
  _count = count;
}

int MenuApp::tileCount() const {
  int n = _count - 1;  // exclude the menu itself at index 0
  return n > kMaxTiles ? kMaxTiles : n;
}

void MenuApp::onEnter() {
  layoutButtons();
  M5.Display.fillScreen(Theme::kBackground);
}

void MenuApp::layoutButtons() {
  const int n = tileCount();
  const int cols = 4;
  const int rows = (n + cols - 1) / cols;
  const int top = AppManager::kStatusBarHeight;
  const int areaW = M5.Display.width();
  const int areaH = M5.Display.height() - top;
  const int padding = 20;
  const int tileW = (areaW - padding * (cols + 1)) / cols;
  const int tileH = (areaH - padding * (rows + 1)) / rows;

  for (int i = 0; i < n; ++i) {
    int col = i % cols;
    int row = i / cols;
    int x = padding + col * (tileW + padding);
    int y = top + padding + row * (tileH + padding);

    // LGFX_Button's internal label buffer caps at 11 chars and isn't
    // null-terminated if the source is longer, so seed it with a safely
    // truncated copy; the full name is passed as drawButton()'s long_name
    // override at draw time instead.
    char shortLabel[12];
    strncpy(shortLabel, _apps[i + 1]->name(), sizeof(shortLabel) - 1);
    shortLabel[sizeof(shortLabel) - 1] = 0;

    _buttons[i].initButtonUL(&M5.Display, x, y, tileW, tileH, Theme::kTextSecondary,
                              _apps[i + 1]->tileColor(), Theme::kTextPrimary, shortLabel,
                              3.0f, 3.0f);
  }
}

void MenuApp::update() {
  const int n = tileCount();
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  for (int i = 0; i < n; ++i) {
    bool inside = pressed && _buttons[i].contains(tx, ty);
    _buttons[i].press(inside);
    if (_buttons[i].justReleased()) {
      _mgr->switchTo(i + 1);
      return;
    }
  }
}

void MenuApp::draw() {
  const int n = tileCount();
  for (int i = 0; i < n; ++i) {
    _buttons[i].drawButton(false, _apps[i + 1]->name());
  }
}
