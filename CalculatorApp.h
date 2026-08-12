#pragma once

#include <M5GFX.h>

#include "App.h"

// Fully offline four-function calculator with immediate (chained) evaluation,
// like a basic pocket calculator: pressing an operator immediately applies
// any previously pending operator rather than waiting for "=".
class CalculatorApp : public App {
 public:
  void onEnter() override;
  void update() override;
  void draw() override;
  const char* name() const override { return "Calculator"; }

 private:
  static constexpr int kCols = 4;
  static constexpr int kRows = 5;
  static constexpr int kKeyCount = kCols * kRows;
  static constexpr int kDisplayLen = 24;

  void layout();
  void handleKey(char action);
  void formatValue(double v);
  void applyOp(double operand);

  LGFX_Button _keys[kKeyCount];
  char _actions[kKeyCount] = {0};
  const char* _labels[kKeyCount] = {nullptr};

  char _display[kDisplayLen] = "0";
  double _accumulator = 0.0;
  char _pendingOp = 0;
  bool _startNewEntry = true;
  bool _error = false;

  int _displayAreaBottom = 0;
};
