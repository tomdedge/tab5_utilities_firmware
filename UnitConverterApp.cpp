#include "UnitConverterApp.h"

#include "Theme.h"

#include <M5Unified.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "AppManager.h"

namespace {
const int kTop = AppManager::kStatusBarHeight;

const char* kCategoryNames[UnitConverterApp::kCategoryCount] = {
    "Length", "Weight", "Temp", "Volume"};

const char* kLengthUnits[] = {"Meters",     "Feet",       "Inches",
                               "Miles",      "Kilometers", "Centimeters"};
const double kLengthToBase[] = {1.0, 0.3048, 0.0254, 1609.344, 1000.0, 0.01};

const char* kWeightUnits[] = {"Kilograms", "Pounds", "Ounces", "Grams"};
const double kWeightToBase[] = {1.0, 0.453592, 0.0283495, 0.001};

const char* kTempUnits[] = {"Celsius", "Fahrenheit", "Kelvin"};

const char* kVolumeUnits[] = {"Liters", "Gallons", "Milliliters", "Cups"};
const double kVolumeToBase[] = {1.0, 3.78541, 0.001, 0.236588};

const char* const* kCategoryUnits[UnitConverterApp::kCategoryCount] = {
    kLengthUnits, kWeightUnits, kTempUnits, kVolumeUnits};
const int kCategoryUnitCounts[UnitConverterApp::kCategoryCount] = {6, 4, 3,
                                                                     4};
const double* kCategoryToBase[UnitConverterApp::kCategoryCount] = {
    kLengthToBase, kWeightToBase, nullptr, kVolumeToBase};

double celsiusFromTemp(int unitIdx, double v) {
  switch (unitIdx) {
    case 1:
      return (v - 32.0) * 5.0 / 9.0;  // Fahrenheit
    case 2:
      return v - 273.15;  // Kelvin
    default:
      return v;  // Celsius
  }
}

double tempFromCelsius(int unitIdx, double c) {
  switch (unitIdx) {
    case 1:
      return c * 9.0 / 5.0 + 32.0;
    case 2:
      return c + 273.15;
    default:
      return c;
  }
}

struct KeyDef {
  char action;
  const char* label;
  int col;
  int row;
};
const KeyDef kKeys[UnitConverterApp::kKeyCount] = {
    {'7', "7", 0, 0}, {'8', "8", 1, 0}, {'9', "9", 2, 0}, {'D', "DEL", 3, 0},
    {'4', "4", 0, 1}, {'5', "5", 1, 1}, {'6', "6", 2, 1}, {'C', "C", 3, 1},
    {'1', "1", 0, 2}, {'2', "2", 1, 2}, {'3', "3", 2, 2}, {'.', ".", 3, 2},
    {'0', "0", 0, 3},
};
const int kKeyCols = 4;
const int kKeyRows = 4;
}  // namespace

double UnitConverterApp::convert(double value) const {
  if (_category == 2) {  // Temperature
    double c = celsiusFromTemp(_fromUnit, value);
    return tempFromCelsius(_toUnit, c);
  }
  const double* toBase = kCategoryToBase[_category];
  double base = value * toBase[_fromUnit];
  return base / toBase[_toUnit];
}

void UnitConverterApp::layoutKeypad() {
  const int screenW = M5.Display.width();
  const int screenH = M5.Display.height();
  const int padding = 8;
  const int keypadTop = kTop + 10 + 56 + 12 + 64 + 90;
  const int colW = (screenW - padding * (kKeyCols + 1)) / kKeyCols;
  const int rowH = (screenH - keypadTop - padding * kKeyRows) / kKeyRows;

  for (int i = 0; i < kKeyCount; ++i) {
    int x = padding + kKeys[i].col * (colW + padding);
    int y = keypadTop + kKeys[i].row * (rowH + padding);
    uint32_t fill = (kKeys[i].action == 'C' || kKeys[i].action == 'D')
                        ? Theme::kDanger
                        : Theme::kOutline;
    _keys[i].initButtonUL(&M5.Display, x, y, colW, rowH, Theme::kTextSecondary, fill,
                           Theme::kTextPrimary, kKeys[i].label, 3.0f, 3.0f);
  }
}

void UnitConverterApp::layout() {
  const int screenW = M5.Display.width();
  const int padding = 8;
  const int catH = 56;
  const int catY = kTop + 10;
  const int catW =
      (screenW - padding * (kCategoryCount + 1)) / kCategoryCount;

  for (int i = 0; i < kCategoryCount; ++i) {
    int x = padding + i * (catW + padding);
    uint32_t fill = (i == _category) ? Theme::kSuccess : Theme::kPrimary;
    _categoryButtons[i].initButtonUL(&M5.Display, x, catY, catW, catH,
                                      Theme::kOutline, fill, Theme::kTextPrimary, "", 2.2f,
                                      2.2f);
  }

  const int rowY = catY + catH + 12;
  const int rowH = 64;
  const int swapW = 90;
  const int unitW = (screenW - padding * 3 - swapW) / 2;
  _fromBtn.initButtonUL(&M5.Display, padding, rowY, unitW, rowH, Theme::kOutline,
                         Theme::kPrimary, Theme::kTextPrimary, "", 2.4f, 2.4f);
  _swapBtn.initButtonUL(&M5.Display, padding * 2 + unitW, rowY, swapW, rowH,
                         Theme::kOutline, Theme::kBackground, Theme::kTextPrimary, "<->", 2.4f,
                         2.4f);
  _toBtn.initButtonUL(&M5.Display, padding * 3 + unitW + swapW, rowY, unitW,
                       rowH, Theme::kOutline, Theme::kPrimary, Theme::kTextPrimary, "", 2.4f,
                       2.4f);

  layoutKeypad();
}

void UnitConverterApp::onEnter() {
  layout();
  M5.Display.fillScreen(Theme::kBackground);
}

void UnitConverterApp::appendChar(char c) {
  int len = (int)strlen(_inputText);
  if (strcmp(_inputText, "0") == 0 && c != '.') {
    _inputText[0] = c;
    _inputText[1] = 0;
    return;
  }
  if (c == '.' && strchr(_inputText, '.')) return;
  if (len < (int)sizeof(_inputText) - 1) {
    _inputText[len] = c;
    _inputText[len + 1] = 0;
  }
}

void UnitConverterApp::backspace() {
  int len = (int)strlen(_inputText);
  if (len <= 1) {
    strcpy(_inputText, "0");
    return;
  }
  _inputText[len - 1] = 0;
}

void UnitConverterApp::update() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  for (int i = 0; i < kCategoryCount; ++i) {
    bool inside = pressed && _categoryButtons[i].contains(tx, ty);
    _categoryButtons[i].press(inside);
    if (_categoryButtons[i].justReleased()) {
      _category = i;
      _fromUnit = 0;
      _toUnit = (kCategoryUnitCounts[i] > 1) ? 1 : 0;
      strcpy(_inputText, "0");
      layout();
      M5.Display.fillScreen(Theme::kBackground);
      return;
    }
  }

  bool insideFrom = pressed && _fromBtn.contains(tx, ty);
  _fromBtn.press(insideFrom);
  if (_fromBtn.justReleased()) {
    _fromUnit = (_fromUnit + 1) % kCategoryUnitCounts[_category];
    return;
  }

  bool insideTo = pressed && _toBtn.contains(tx, ty);
  _toBtn.press(insideTo);
  if (_toBtn.justReleased()) {
    _toUnit = (_toUnit + 1) % kCategoryUnitCounts[_category];
    return;
  }

  bool insideSwap = pressed && _swapBtn.contains(tx, ty);
  _swapBtn.press(insideSwap);
  if (_swapBtn.justReleased()) {
    int tmp = _fromUnit;
    _fromUnit = _toUnit;
    _toUnit = tmp;
    return;
  }

  for (int i = 0; i < kKeyCount; ++i) {
    bool inside = pressed && _keys[i].contains(tx, ty);
    _keys[i].press(inside);
    if (_keys[i].justReleased()) {
      char a = kKeys[i].action;
      if (a == 'C') {
        strcpy(_inputText, "0");
      } else if (a == 'D') {
        backspace();
      } else {
        appendChar(a);
      }
      return;
    }
  }
}

void UnitConverterApp::draw() {
  const int screenW = M5.Display.width();

  for (int i = 0; i < kCategoryCount; ++i) {
    _categoryButtons[i].drawButton(false, kCategoryNames[i]);
  }
  _fromBtn.drawButton(false, kCategoryUnits[_category][_fromUnit]);
  _swapBtn.drawButton();
  _toBtn.drawButton(false, kCategoryUnits[_category][_toUnit]);

  double inputValue = atof(_inputText);
  double result = convert(inputValue);

  char resultStr[32];
  snprintf(resultStr, sizeof(resultStr), "%.4g", result);

  char line[80];
  snprintf(line, sizeof(line), "%s %s = %s %s", _inputText,
           kCategoryUnits[_category][_fromUnit], resultStr,
           kCategoryUnits[_category][_toUnit]);

  const int readoutY = kTop + 10 + 56 + 12 + 64 + 20;
  M5.Display.fillRect(0, readoutY, screenW, 60, Theme::kBackground);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
  M5.Display.setTextSize(2);
  M5.Display.drawString(line, screenW / 2, readoutY + 30);

  for (int i = 0; i < kKeyCount; ++i) {
    _keys[i].drawButton();
  }
}
