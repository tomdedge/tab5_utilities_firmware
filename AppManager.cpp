#include "AppManager.h"

#include <M5Unified.h>

#include "Theme.h"

void AppManager::begin(App** apps, int count) {
  _apps = apps;
  _count = count;
  _active = kMenuIndex;
  _apps[_active]->onEnter();
}

void AppManager::switchTo(int index) {
  if (index < 0 || index >= _count || index == _active) return;
  _apps[_active]->onExit();
  _active = index;
  _apps[_active]->onEnter();
  M5.Display.fillScreen(Theme::kBackground);
}

bool AppManager::handleStatusBarTouch() {
  if (_active == kMenuIndex) return false;
  if (M5.Touch.getCount() == 0) return false;

  auto t = M5.Touch.getDetail(0);
  if (t.wasReleased() && t.x >= _homeX && t.x <= _homeX + _homeW &&
      t.y >= _homeY && t.y <= _homeY + _homeH) {
    switchTo(kMenuIndex);
    return true;
  }
  return false;
}

void AppManager::update() {
  if (_alarmRinging) {
    updateAlarmOverlay();
    return;
  }
  if (handleStatusBarTouch()) return;
  _apps[_active]->update();
}

void AppManager::ringAlarm(int hour, int minute) {
  _alarmRinging = true;
  _alarmHour = hour;
  _alarmMinute = minute;
  _lastBeepMs = 0;
  layoutAlarmOverlay();
}

void AppManager::layoutAlarmOverlay() {
  const int screenW = M5.Display.width();
  const int screenH = M5.Display.height();
  const int btnW = 320, btnH = 90;
  _dismissBtn.initButtonUL(&M5.Display, screenW / 2 - btnW / 2, screenH - btnH - 60,
                            btnW, btnH, Theme::kOutline, Theme::kSuccess,
                            Theme::kTextPrimary, "", 2.5f, 2.5f);
}

void AppManager::updateAlarmOverlay() {
  uint32_t now = millis();
  if (now - _lastBeepMs > 2000) {
    _lastBeepMs = now;
    M5.Speaker.begin();
    M5.Speaker.setVolume(255);
    M5.Speaker.tone(1000, 400);
  }

  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  bool inside = pressed && _dismissBtn.contains(tx, ty);
  _dismissBtn.press(inside);
  if (_dismissBtn.justReleased()) {
    _alarmRinging = false;
    M5.Speaker.stop();
    M5.Display.fillScreen(Theme::kBackground);
  }
}

void AppManager::drawAlarmOverlay() {
  auto& d = M5.Display;
  bool flash = (millis() / 400) % 2;
  uint32_t bg = flash ? Theme::kDanger : Theme::kBackground;
  d.fillScreen(bg);

  int hour12 = _alarmHour % 12;
  if (hour12 == 0) hour12 = 12;
  const char* ampm = _alarmHour < 12 ? "AM" : "PM";
  char buf[16];
  snprintf(buf, sizeof(buf), "%d:%02d %s", hour12, _alarmMinute, ampm);

  d.setTextDatum(middle_center);
  d.setTextColor(Theme::kTextPrimary, bg);
  d.setTextSize(4);
  d.drawString("ALARM", d.width() / 2, d.height() / 2 - 150);
  d.setTextSize(7);
  d.drawString(buf, d.width() / 2, d.height() / 2 - 40);

  _dismissBtn.drawButton(false, "Dismiss");
}

void AppManager::drawStatusBar() {
  auto& d = M5.Display;
  d.fillRect(0, 0, d.width(), kStatusBarHeight, Theme::kSurface);

  // Text size/datum/color are mutable state shared on the display object --
  // every app's own draw() sets these explicitly before it draws, but this
  // runs *before* that each frame, so it must never rely on whatever the
  // previous frame happened to leave behind (that was the bug that made the
  // bar flicker: it would inherit e.g. a huge leftover size from the last
  // app drawn and briefly render oversized before the app's next redraw
  // painted over it).
  d.setTextSize(2);

  if (_active != kMenuIndex) {
    d.fillRoundRect(_homeX, _homeY, _homeW, _homeH, 8, Theme::kPrimary);
    d.setTextDatum(middle_center);
    d.setTextColor(Theme::kTextPrimary, Theme::kPrimary);
    d.drawString("Home", _homeX + _homeW / 2, _homeY + _homeH / 2);
  }

  if (M5.Rtc.isEnabled()) {
    auto dt = M5.Rtc.getDateTime();
    int hour12 = dt.time.hours % 12;
    if (hour12 == 0) hour12 = 12;
    const char* ampm = dt.time.hours < 12 ? "AM" : "PM";
    char buf[12];
    snprintf(buf, sizeof(buf), "%d:%02d %s", hour12, dt.time.minutes, ampm);
    d.setTextDatum(middle_center);
    d.setTextColor(Theme::kTextPrimary, Theme::kSurface);
    d.drawString(buf, d.width() / 2, kStatusBarHeight / 2);
  }

  int batt = M5.Power.getBatteryLevel();
  char bbuf[8];
  if (batt >= 0) {
    snprintf(bbuf, sizeof(bbuf), "%d%%", batt);
  } else {
    snprintf(bbuf, sizeof(bbuf), "--");
  }
  d.setTextDatum(middle_right);
  d.setTextColor(Theme::kTextSecondary, Theme::kSurface);
  d.drawString(bbuf, d.width() - 16, kStatusBarHeight / 2);
}

void AppManager::draw() {
  if (_alarmRinging) {
    drawAlarmOverlay();
    return;
  }
  drawStatusBar();
  _apps[_active]->draw();
}
