#include "CalculatorApp.h"

#include "Theme.h"

#include <M5Unified.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "AppManager.h"

void CalculatorApp::onEnter() {
  _accumulator = 0.0;
  _pendingOp = 0;
  _startNewEntry = true;
  _error = false;
  strcpy(_display, "0");
  layout();
  M5.Display.fillScreen(Theme::kBackground);
}

void CalculatorApp::layout() {
  static const char* labels[kKeyCount] = {
      "C", "DEL", "%",   "/", "7", "8", "9",  "*", "4", "5",
      "6", "-",   "1",   "2", "3", "+", "+/-", "0", ".", "=",
  };
  static const char actions[kKeyCount] = {
      'C', 'D', '%', '/', '7', '8', '9', '*', '4', '5',
      '6', '-', '1', '2', '3', '+', 'N', '0', '.', '=',
  };

  for (int i = 0; i < kKeyCount; ++i) {
    _labels[i] = labels[i];
    _actions[i] = actions[i];
  }

  const int top = AppManager::kStatusBarHeight;
  const int screenW = M5.Display.width();
  const int screenH = M5.Display.height();
  const int padding = 6;

  _displayAreaBottom = top + 140;

  const int gridTop = _displayAreaBottom + padding;
  const int gridH = screenH - gridTop - padding;
  const int keyW = (screenW - padding * (kCols + 1)) / kCols;
  const int keyH = (gridH - padding * kRows) / kRows;

  for (int r = 0; r < kRows; ++r) {
    for (int c = 0; c < kCols; ++c) {
      int i = r * kCols + c;
      int x = padding + c * (keyW + padding);
      int y = gridTop + r * (keyH + padding);

      uint32_t fill = Theme::kOutline;
      char a = _actions[i];
      if (a == 'C' || a == 'D') {
        fill = Theme::kDanger;
      } else if (a == '=') {
        fill = Theme::kSuccess;
      } else if (a == '+' || a == '-' || a == '*' || a == '/') {
        fill = Theme::kPrimary;
      }

      _keys[i].initButtonUL(&M5.Display, x, y, keyW, keyH, Theme::kTextSecondary,
                             fill, Theme::kTextPrimary, _labels[i], 4.0f, 4.0f);
    }
  }
}

void CalculatorApp::formatValue(double v) {
  snprintf(_display, kDisplayLen, "%.10g", v);
}

void CalculatorApp::applyOp(double operand) {
  double a = _accumulator;
  double result = operand;
  switch (_pendingOp) {
    case '+':
      result = a + operand;
      break;
    case '-':
      result = a - operand;
      break;
    case '*':
      result = a * operand;
      break;
    case '/':
      if (operand == 0.0) {
        _error = true;
        strcpy(_display, "Error");
        return;
      }
      result = a / operand;
      break;
    default:
      break;
  }
  _accumulator = result;
}

void CalculatorApp::handleKey(char action) {
  if (_error && action != 'C') return;

  if (action >= '0' && action <= '9') {
    if (_startNewEntry) {
      strcpy(_display, "0");
      _startNewEntry = false;
    }
    int len = (int)strlen(_display);
    bool isZero = (len == 1 && _display[0] == '0');
    if (isZero) {
      _display[0] = action;
      _display[1] = 0;
    } else if (len < kDisplayLen - 1) {
      _display[len] = action;
      _display[len + 1] = 0;
    }
    return;
  }

  switch (action) {
    case 'C':
      _accumulator = 0.0;
      _pendingOp = 0;
      _error = false;
      strcpy(_display, "0");
      _startNewEntry = true;
      return;

    case 'D': {
      if (_startNewEntry) return;
      int len = (int)strlen(_display);
      if (len <= 1) {
        strcpy(_display, "0");
        _startNewEntry = true;
      } else {
        _display[len - 1] = 0;
      }
      return;
    }

    case '.':
      if (_startNewEntry) {
        strcpy(_display, "0.");
        _startNewEntry = false;
        return;
      }
      if (!strchr(_display, '.')) {
        int len = (int)strlen(_display);
        if (len < kDisplayLen - 1) {
          _display[len] = '.';
          _display[len + 1] = 0;
        }
      }
      return;

    case 'N': {
      if (_display[0] == '-') {
        memmove(_display, _display + 1, strlen(_display));
      } else if (strcmp(_display, "0") != 0) {
        int len = (int)strlen(_display);
        if (len < kDisplayLen - 1) {
          memmove(_display + 1, _display, len + 1);
          _display[0] = '-';
        }
      }
      return;
    }

    case '%': {
      double v = atof(_display) / 100.0;
      formatValue(v);
      _startNewEntry = false;
      return;
    }

    case '+':
    case '-':
    case '*':
    case '/': {
      double operand = atof(_display);
      if (_pendingOp) {
        applyOp(operand);
      } else {
        _accumulator = operand;
      }
      if (!_error) {
        _pendingOp = action;
        formatValue(_accumulator);
      }
      _startNewEntry = true;
      return;
    }

    case '=': {
      double operand = atof(_display);
      if (_pendingOp) {
        applyOp(operand);
      } else {
        _accumulator = operand;
      }
      _pendingOp = 0;
      if (!_error) formatValue(_accumulator);
      _startNewEntry = true;
      return;
    }

    default:
      return;
  }
}

void CalculatorApp::update() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  for (int i = 0; i < kKeyCount; ++i) {
    auto& btn = _keys[i];
    bool inside = pressed && btn.contains(tx, ty);
    btn.press(inside);
    if (btn.justReleased()) {
      handleKey(_actions[i]);
      return;
    }
  }
}

void CalculatorApp::draw() {
  const int top = AppManager::kStatusBarHeight;
  const int margin = 12;
  M5.Display.fillRect(0, top, M5.Display.width(), _displayAreaBottom - top,
                       Theme::kBackground);
  M5.Display.fillRoundRect(margin, top + margin, M5.Display.width() - margin * 2,
                            _displayAreaBottom - top - margin * 2, 16,
                            Theme::kSurface);

  int len = (int)strlen(_display);
  int textSize = len > 12 ? 3 : (len > 8 ? 5 : 7);

  M5.Display.setTextDatum(middle_right);
  M5.Display.setTextColor(Theme::kTextPrimary, Theme::kSurface);
  M5.Display.setTextSize(textSize);
  M5.Display.drawString(_display, M5.Display.width() - 20 - margin,
                         (top + _displayAreaBottom) / 2);

  for (int i = 0; i < kKeyCount; ++i) _keys[i].drawButton();
}
