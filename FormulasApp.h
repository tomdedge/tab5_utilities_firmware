#pragma once

#include <M5GFX.h>

#include "App.h"

// Six categories of everyday reference formulas (Everyday, Finance,
// Geometry, Physics, Health, Electronic): a category grid -> a list of
// formulas in that category -> a small generic calculator screen (labeled
// input fields + a numeric keypad, built from a static FormulaDef table in
// FormulasApp.cpp). Adding a new formula only means adding a table row and
// a small compute function there, not a new screen. Fully offline.
class FormulasApp : public App {
 public:
  void onEnter() override;
  void update() override;
  void draw() override;
  const char* name() const override { return "Formulas"; }

  // Public so the formula table and keypad layout in the .cpp can size
  // arrays against them (see CLAUDE.md: private static constexpr members
  // aren't visible to anonymous-namespace code in the same .cpp file).
  static constexpr int kCategoryCount = 6;
  static constexpr int kMaxFields = 4;
  static constexpr int kKeypadKeyCount = 13;

 private:
  enum class State { Categories, List, Calc };

  static constexpr int kMaxFormulasPerCategory = 8;

  static constexpr int kFieldLabelH = 22;
  static constexpr int kFieldGap = 4;
  static constexpr int kFieldBoxH = 52;
  static constexpr int kFieldRowGap = 12;

  void layoutCategories();
  void layoutList();
  void layoutCalc();
  void layoutKeypad(int x, int y, int w, int h);

  void updateCategories();
  void updateList();
  void updateCalc();

  void drawCategories();
  void drawList();
  void drawCalc();

  void enterCategory(int category);
  void enterFormula(int formulaIndex);
  void appendChar(char c);
  void backspace();
  void calculate();

  State _state = State::Categories;
  int _category = -1;
  int _formulaIndex = -1;  // index into the file-scope formula table

  int _listIndices[kMaxFormulasPerCategory];
  int _listCount = 0;

  char _fieldText[kMaxFields][24];
  bool _fieldToggle[kMaxFields] = {false, false, false, false};
  int _focusedField = -1;

  char _resultText[200] = {0};

  LGFX_Button _categoryButtons[kCategoryCount];
  LGFX_Button _listButtons[kMaxFormulasPerCategory];
  LGFX_Button _listBackBtn;

  LGFX_Button _calcBackBtn;
  LGFX_Button _fieldButtons[kMaxFields];
  LGFX_Button _calculateBtn;
  LGFX_Button _keys[kKeypadKeyCount];

  int _fieldBoxY[kMaxFields] = {0, 0, 0, 0};
  int _fieldsLeftX = 0, _fieldsWidth = 0;

  int _resultX = 0, _resultTop = 0, _resultW = 0, _resultH = 0;
};
