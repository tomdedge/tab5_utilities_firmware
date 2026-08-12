#include "SettingsApp.h"

#include <M5Unified.h>

#include "AppManager.h"
#include "Theme.h"

namespace {
const int kTop = AppManager::kStatusBarHeight;
}  // namespace

void SettingsApp::layoutPicker() {
  const int screenW = M5.Display.width();
  const int screenH = M5.Display.height();
  const int padding = 20;
  const int top = kTop + 70;
  const int tileW = (screenW - padding * 3) / 2;
  const int tileH = screenH - top - padding;

  _wifiTileBtn.initButtonUL(&M5.Display, padding, top, tileW, tileH,
                             Theme::kOutline, Theme::kPrimary,
                             Theme::kTextPrimary, "", 3.0f, 3.0f);
  _timeZoneTileBtn.initButtonUL(&M5.Display, padding * 2 + tileW, top, tileW,
                                 tileH, Theme::kOutline, Theme::kPrimary,
                                 Theme::kTextPrimary, "", 3.0f, 3.0f);
}

void SettingsApp::onEnter() {
  if (_forceWifiSection) {
    _forceWifiSection = false;
    _section = Section::Wifi;
    _wifi.onEnter();
    return;
  }
  _section = Section::Picker;
  layoutPicker();
  // Don't draw here: AppManager::switchTo() calls onEnter() and then
  // clears the whole screen afterward, so anything painted here would
  // just get wiped. draw() paints the picker once instead, the first time
  // it runs after this flag is set.
  _pickerDirty = true;
}

void SettingsApp::onExit() {
  if (_section == Section::Wifi) {
    _wifi.onExit();
  } else if (_section == Section::TimeZone) {
    _timeZone.onExit();
  }
  _section = Section::Picker;
}

void SettingsApp::openWifiSection() { _forceWifiSection = true; }

void SettingsApp::updatePicker() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  bool insideWifi = pressed && _wifiTileBtn.contains(tx, ty);
  _wifiTileBtn.press(insideWifi);
  if (_wifiTileBtn.justReleased()) {
    _section = Section::Wifi;
    _wifi.onEnter();
    return;
  }

  bool insideTz = pressed && _timeZoneTileBtn.contains(tx, ty);
  _timeZoneTileBtn.press(insideTz);
  if (_timeZoneTileBtn.justReleased()) {
    _section = Section::TimeZone;
    _timeZone.onEnter();
    return;
  }
}

void SettingsApp::update() {
  switch (_section) {
    case Section::Picker:
      updatePicker();
      break;
    case Section::Wifi:
      _wifi.update();
      break;
    case Section::TimeZone:
      _timeZone.update();
      break;
  }
}

void SettingsApp::drawPicker() {
  M5.Display.setTextDatum(top_left);
  M5.Display.setTextColor(Theme::kTextSecondary, Theme::kBackground);
  M5.Display.setTextSize(2);
  M5.Display.drawString("Settings", 10, kTop + 12);

  _wifiTileBtn.drawButton(false, "Wi-Fi");
  _timeZoneTileBtn.drawButton(false, "Time Zone");
}

void SettingsApp::draw() {
  switch (_section) {
    case Section::Picker:
      // Paint exactly once: the first draw() after AppManager's
      // post-onEnter() screen clear. The tiles never change appearance
      // (no press-highlight, no dynamic label), so nothing needs to
      // refresh on later frames -- redrawing them every frame at 100+ fps
      // was visibly flickering on real hardware.
      if (_pickerDirty) {
        drawPicker();
        _pickerDirty = false;
      }
      break;
    case Section::Wifi:
      _wifi.draw();
      break;
    case Section::TimeZone:
      _timeZone.draw();
      break;
  }
}
