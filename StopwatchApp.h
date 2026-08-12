#pragma once

#include <M5GFX.h>

#include "App.h"

// Combined stopwatch + countdown timer, switched via a tab toggle at the
// top. Elapsed/remaining time is tracked as a millis() delta rather than a
// per-frame counter, so it stays correct even if the user leaves this
// screen and comes back later -- onEnter() must not reset it.
class StopwatchApp : public App {
 public:
  void onEnter() override;
  void update() override;
  void draw() override;
  const char* name() const override { return "Stopwatch"; }

 private:
  enum class Mode { Stopwatch, Timer };

  static constexpr int kPresetCount = 5;
  static const int kPresetMinutes[kPresetCount];

  void layout();
  void updateStopwatch();
  void updateTimer();
  void drawStopwatch();
  void drawTimer();
  static void formatMs(unsigned long ms, char* out, size_t outLen);

  Mode _mode = Mode::Stopwatch;

  bool _swRunning = false;
  unsigned long _swStartMs = 0;
  unsigned long _swElapsedMs = 0;

  bool _timerRunning = false;
  bool _timerFinished = false;
  unsigned long _timerStartMs = 0;
  unsigned long _timerDurationMs = 5 * 60000UL;
  unsigned long _timerRemainingMs = 5 * 60000UL;

  LGFX_Button _stopwatchTabBtn, _timerTabBtn;
  LGFX_Button _startPauseBtn, _resetBtn;
  LGFX_Button _presetButtons[kPresetCount];
};
