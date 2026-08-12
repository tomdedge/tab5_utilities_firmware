#pragma once

#include <M5GFX.h>

#include "App.h"

// Category picker (Length/Weight/Temperature/Volume) + From/To unit
// selectors (tap to cycle) + a numeric keypad, converting live as digits
// are entered. Linear categories use a scale-to-base-unit factor;
// Temperature is special-cased (affine, not a simple ratio). Fully offline.
class UnitConverterApp : public App {
 public:
  void onEnter() override;
  void update() override;
  void draw() override;
  const char* name() const override { return "Unit Converter"; }

  // Public so file-scope data tables in the .cpp can size arrays against
  // them; not otherwise part of the App interface.
  static constexpr int kCategoryCount = 4;
  static constexpr int kKeyCount = 13;

 private:
  void layout();
  void layoutKeypad();
  double convert(double value) const;
  void appendChar(char c);
  void backspace();

  int _category = 0;
  int _fromUnit = 0;
  int _toUnit = 1;
  char _inputText[24] = "0";

  LGFX_Button _categoryButtons[kCategoryCount];
  LGFX_Button _fromBtn, _toBtn, _swapBtn;
  LGFX_Button _keys[kKeyCount];
};
