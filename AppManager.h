#pragma once

#include <M5GFX.h>

#include "App.h"

// Owns the fixed list of mini-apps, drives the active one's update()/draw(),
// and renders a persistent status bar (Home button, clock, battery) on top
// of every app except the menu itself.
//
// Also owns the alarm-ringing overlay: an alarm must be able to interrupt
// whichever app happens to be on screen, so ringAlarm() is called from
// Test.ino's loop() (which checks AlarmStore every frame regardless of the
// active app) and, once ringing, update()/draw() short-circuit to a
// full-screen alert instead of delegating to the active app.
class AppManager {
 public:
  static constexpr int kStatusBarHeight = 64;
  static constexpr int kMenuIndex = 0;

  void begin(App** apps, int count);
  void update();
  void draw();

  void switchTo(int index);
  int activeIndex() const { return _active; }

  void ringAlarm(int hour, int minute);

 private:
  void drawStatusBar();
  bool handleStatusBarTouch();

  void layoutAlarmOverlay();
  void updateAlarmOverlay();
  void drawAlarmOverlay();

  App** _apps = nullptr;
  int _count = 0;
  int _active = 0;

  int _homeX = 8, _homeY = 4, _homeW = 100, _homeH = kStatusBarHeight - 8;

  bool _alarmRinging = false;
  int _alarmHour = 0, _alarmMinute = 0;
  uint32_t _lastBeepMs = 0;
  LGFX_Button _dismissBtn;
};
