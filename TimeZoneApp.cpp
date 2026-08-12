#include "TimeZoneApp.h"

#include "Theme.h"

#include <M5Unified.h>
#include <Preferences.h>

#include <cstdio>

#include "AppManager.h"
#include "TimeSync.h"

namespace {
const int kTop = AppManager::kStatusBarHeight;
const char* kPrefsNamespace = "tz";
}  // namespace

const TimeZoneApp::ZoneInfo TimeZoneApp::kZones[TimeZoneApp::kZoneCount] = {
    {"UTC", 0},
    {"US Eastern", -5 * 3600L},
    {"US Central", -6 * 3600L},
    {"US Mountain", -7 * 3600L},
    {"US Pacific", -8 * 3600L},
    {"US Alaska", -9 * 3600L},
    {"US Hawaii", -10 * 3600L},
    {"UK/W Europe", 0},
    {"Central Europe", 1 * 3600L},
    {"Eastern Europe", 2 * 3600L},
    {"Moscow", 3 * 3600L},
    {"India", 5 * 3600L + 1800L},
    {"China/Sing.", 8 * 3600L},
    {"Japan/Korea", 9 * 3600L},
    {"Australia E.", 10 * 3600L},
    {"New Zealand", 12 * 3600L},
};

void TimeZoneApp::loadSelection() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, /*readOnly=*/true);
  _selectedIndex = prefs.getInt("zoneidx", 0);
  _dst = prefs.getBool("dst", false);
  prefs.end();
  if (_selectedIndex < 0 || _selectedIndex >= kZoneCount) _selectedIndex = 0;
}

void TimeZoneApp::saveSelection() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, /*readOnly=*/false);
  prefs.putInt("zoneidx", _selectedIndex);
  prefs.putBool("dst", _dst);
  prefs.end();
}

void TimeZoneApp::layout() {
  const int screenW = M5.Display.width();
  const int screenH = M5.Display.height();
  const int gridTop = kTop + 60;
  const int dstH = 70;
  const int padding = 8;
  const int rows = (kZoneCount + kCols - 1) / kCols;
  const int gridH = screenH - gridTop - dstH - padding * 2;
  const int colW = (screenW - padding * (kCols + 1)) / kCols;
  const int rowH = (gridH - padding * rows) / rows;

  for (int i = 0; i < kZoneCount; ++i) {
    int col = i % kCols;
    int row = i / kCols;
    int x = padding + col * (colW + padding);
    int y = gridTop + row * (rowH + padding);
    uint32_t fill = (i == _selectedIndex) ? Theme::kSuccess : Theme::kPrimary;
    _zoneButtons[i].initButtonUL(&M5.Display, x, y, colW, rowH, Theme::kTextSecondary,
                                  fill, Theme::kTextPrimary, "", 2.4f, 2.4f);
  }

  int dstY = screenH - dstH - padding;
  _dstBtn.initButtonUL(&M5.Display, padding, dstY, screenW - padding * 2,
                        dstH, Theme::kOutline, _dst ? Theme::kSuccess : Theme::kBackground,
                        Theme::kTextPrimary, "", 2.2f, 2.2f);
}

void TimeZoneApp::select(int index) {
  _selectedIndex = index;
  saveSelection();
  long offset = kZones[_selectedIndex].offsetSec + (_dst ? 3600L : 0L);
  applyTimezoneOffset(offset);
  layout();
  M5.Display.fillScreen(Theme::kBackground);
}

void TimeZoneApp::onEnter() {
  loadSelection();
  layout();
  M5.Display.fillScreen(Theme::kBackground);
}

void TimeZoneApp::update() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  for (int i = 0; i < kZoneCount; ++i) {
    bool inside = pressed && _zoneButtons[i].contains(tx, ty);
    _zoneButtons[i].press(inside);
    if (_zoneButtons[i].justReleased()) {
      select(i);
      return;
    }
  }

  bool insideDst = pressed && _dstBtn.contains(tx, ty);
  _dstBtn.press(insideDst);
  if (_dstBtn.justReleased()) {
    _dst = !_dst;
    saveSelection();
    long offset = kZones[_selectedIndex].offsetSec + (_dst ? 3600L : 0L);
    applyTimezoneOffset(offset);
    layout();
    M5.Display.fillScreen(Theme::kBackground);
  }
}

void TimeZoneApp::draw() {
  M5.Display.setTextDatum(top_left);
  M5.Display.setTextColor(Theme::kTextSecondary, Theme::kBackground);
  M5.Display.setTextSize(2);
  char summary[64];
  snprintf(summary, sizeof(summary), "Current: %s%s",
           kZones[_selectedIndex].label, _dst ? " (DST)" : "");
  M5.Display.drawString(summary, 10, kTop + 12);

  for (int i = 0; i < kZoneCount; ++i) {
    _zoneButtons[i].drawButton(false, kZones[i].label);
  }
  _dstBtn.drawButton(false, _dst ? "DST: ON" : "DST: OFF");
}
