#include "StopwatchApp.h"

#include "Theme.h"

#include <M5Unified.h>

#include <cstdio>

#include "AppManager.h"

namespace {
const int kTop = AppManager::kStatusBarHeight;
const int kTabY = kTop + 10;
const int kTabH = 64;
const int kContentTop = kTop + 90;
const int kFlashRectH = 200;
const int kPresetY = kContentTop + 220;
const int kPresetH = 64;
}  // namespace

const int StopwatchApp::kPresetMinutes[StopwatchApp::kPresetCount] = {1, 5,
                                                                        10, 30,
                                                                        60};

void StopwatchApp::formatMs(unsigned long ms, char* out, size_t outLen) {
  unsigned long totalSec = ms / 1000;
  unsigned long hh = totalSec / 3600;
  unsigned long mm = (totalSec % 3600) / 60;
  unsigned long ss = totalSec % 60;
  if (hh > 0) {
    snprintf(out, outLen, "%lu:%02lu:%02lu", hh, mm, ss);
  } else {
    snprintf(out, outLen, "%02lu:%02lu", mm, ss);
  }
}

void StopwatchApp::layout() {
  const int screenW = M5.Display.width();
  const int screenH = M5.Display.height();
  const int padding = 10;
  const int tabW = 220;

  _stopwatchTabBtn.initButtonUL(
      &M5.Display, padding, kTabY, tabW, kTabH, Theme::kOutline,
      _mode == Mode::Stopwatch ? Theme::kSuccess : Theme::kBackground, Theme::kTextPrimary, "",
      2.2f, 2.2f);
  _timerTabBtn.initButtonUL(
      &M5.Display, padding * 2 + tabW, kTabY, tabW, kTabH, Theme::kOutline,
      _mode == Mode::Timer ? Theme::kSuccess : Theme::kBackground, Theme::kTextPrimary, "", 2.2f,
      2.2f);

  const int btnW = 220, btnH = 70;
  const int btnY = screenH - btnH - 20;
  _startPauseBtn.initButtonUL(&M5.Display, screenW / 2 - btnW - 10, btnY,
                               btnW, btnH, Theme::kOutline, Theme::kSuccess,
                               Theme::kTextPrimary, "", 2.0f, 2.0f);
  _resetBtn.initButtonUL(&M5.Display, screenW / 2 + 10, btnY, btnW, btnH,
                          Theme::kOutline, Theme::kDanger, Theme::kTextPrimary, "Reset", 2.0f,
                          2.0f);

  const int presetW = 180;
  const int totalPresetW =
      kPresetCount * presetW + (kPresetCount - 1) * padding;
  const int presetX0 = (screenW - totalPresetW) / 2;
  for (int i = 0; i < kPresetCount; ++i) {
    int x = presetX0 + i * (presetW + padding);
    _presetButtons[i].initButtonUL(&M5.Display, x, kPresetY, presetW,
                                    kPresetH, Theme::kOutline, Theme::kPrimary,
                                    Theme::kTextPrimary, "", 2.2f, 2.2f);
  }
}

void StopwatchApp::onEnter() {
  layout();
  M5.Display.fillScreen(Theme::kBackground);
}

void StopwatchApp::updateStopwatch() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  bool insideStart = pressed && _startPauseBtn.contains(tx, ty);
  _startPauseBtn.press(insideStart);
  if (_startPauseBtn.justReleased()) {
    if (_swRunning) {
      _swElapsedMs += millis() - _swStartMs;
      _swRunning = false;
    } else {
      _swStartMs = millis();
      _swRunning = true;
    }
    return;
  }

  bool insideReset = pressed && _resetBtn.contains(tx, ty);
  _resetBtn.press(insideReset);
  if (_resetBtn.justReleased()) {
    _swRunning = false;
    _swElapsedMs = 0;
  }
}

void StopwatchApp::updateTimer() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  if (!_timerRunning) {
    for (int i = 0; i < kPresetCount; ++i) {
      bool inside = pressed && _presetButtons[i].contains(tx, ty);
      _presetButtons[i].press(inside);
      if (_presetButtons[i].justReleased()) {
        _timerDurationMs = (unsigned long)kPresetMinutes[i] * 60000UL;
        _timerRemainingMs = _timerDurationMs;
        _timerFinished = false;
        return;
      }
    }
  }

  bool insideStart = pressed && _startPauseBtn.contains(tx, ty);
  _startPauseBtn.press(insideStart);
  if (_startPauseBtn.justReleased()) {
    if (_timerFinished) {
      _timerFinished = false;
      _timerRemainingMs = _timerDurationMs;
      return;
    }
    if (_timerRunning) {
      _timerRemainingMs -= millis() - _timerStartMs;
      _timerRunning = false;
    } else {
      _timerStartMs = millis();
      _timerRunning = true;
    }
    return;
  }

  bool insideReset = pressed && _resetBtn.contains(tx, ty);
  _resetBtn.press(insideReset);
  if (_resetBtn.justReleased()) {
    _timerRunning = false;
    _timerFinished = false;
    _timerRemainingMs = _timerDurationMs;
    return;
  }

  if (_timerRunning) {
    unsigned long elapsed = millis() - _timerStartMs;
    if (elapsed >= _timerRemainingMs) {
      _timerRunning = false;
      _timerFinished = true;
      _timerRemainingMs = 0;
      M5.Speaker.begin();
      M5.Speaker.setVolume(255);
      M5.Speaker.tone(1000, 600);
    }
  }
}

void StopwatchApp::update() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  bool insideSwTab = pressed && _stopwatchTabBtn.contains(tx, ty);
  _stopwatchTabBtn.press(insideSwTab);
  if (_stopwatchTabBtn.justReleased() && _mode != Mode::Stopwatch) {
    _mode = Mode::Stopwatch;
    layout();
    M5.Display.fillScreen(Theme::kBackground);
    return;
  }

  bool insideTimerTab = pressed && _timerTabBtn.contains(tx, ty);
  _timerTabBtn.press(insideTimerTab);
  if (_timerTabBtn.justReleased() && _mode != Mode::Timer) {
    _mode = Mode::Timer;
    layout();
    M5.Display.fillScreen(Theme::kBackground);
    return;
  }

  if (_mode == Mode::Stopwatch) {
    updateStopwatch();
  } else {
    updateTimer();
  }
}

void StopwatchApp::drawStopwatch() {
  const int screenW = M5.Display.width();
  const int margin = 20;
  M5.Display.fillRect(0, kContentTop, screenW, kFlashRectH, Theme::kBackground);
  M5.Display.fillRoundRect(margin, kContentTop, screenW - margin * 2,
                            kFlashRectH - 20, 20, Theme::kSurface);

  unsigned long elapsed =
      _swElapsedMs + (_swRunning ? (millis() - _swStartMs) : 0);
  char buf[16];
  formatMs(elapsed, buf, sizeof(buf));

  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(Theme::kTextPrimary, Theme::kSurface);
  M5.Display.setTextSize(9);
  M5.Display.drawString(buf, screenW / 2, kContentTop + 90);

  _startPauseBtn.drawButton(false, _swRunning ? "Pause" : "Start");
  _resetBtn.drawButton();
}

void StopwatchApp::drawTimer() {
  const int screenW = M5.Display.width();
  const int margin = 20;

  unsigned long remaining = _timerRemainingMs;
  if (_timerRunning) {
    unsigned long elapsed = millis() - _timerStartMs;
    remaining = (elapsed < _timerRemainingMs) ? _timerRemainingMs - elapsed : 0;
  }

  uint32_t bg = Theme::kSurface;
  if (_timerFinished && ((millis() / 400) % 2)) bg = Theme::kDanger;
  M5.Display.fillRect(0, kContentTop, screenW, kFlashRectH, Theme::kBackground);
  M5.Display.fillRoundRect(margin, kContentTop, screenW - margin * 2,
                            kFlashRectH - 20, 20, bg);

  char buf[16];
  formatMs(remaining, buf, sizeof(buf));
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(Theme::kTextPrimary, bg);
  M5.Display.setTextSize(9);
  M5.Display.drawString(buf, screenW / 2, kContentTop + 70);

  if (_timerFinished) {
    M5.Display.setTextSize(3);
    M5.Display.drawString("Time's Up!", screenW / 2, kContentTop + 160);
  }

  for (int i = 0; i < kPresetCount; ++i) {
    char label[8];
    snprintf(label, sizeof(label), "%d min", kPresetMinutes[i]);
    _presetButtons[i].drawButton(false, label);
  }

  _startPauseBtn.drawButton(
      false, _timerFinished ? "Dismiss" : (_timerRunning ? "Pause" : "Start"));
  _resetBtn.drawButton();
}

void StopwatchApp::draw() {
  _stopwatchTabBtn.drawButton(false, "Stopwatch");
  _timerTabBtn.drawButton(false, "Timer");

  if (_mode == Mode::Stopwatch) {
    drawStopwatch();
  } else {
    drawTimer();
  }
}
