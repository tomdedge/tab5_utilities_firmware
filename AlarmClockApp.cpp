#include "AlarmClockApp.h"

#include <M5Unified.h>

#include <cstdio>

#include "AppManager.h"
#include "Theme.h"

namespace {
const int kTop = AppManager::kStatusBarHeight;
}  // namespace

void AlarmClockApp::begin(AlarmStore* store) { _store = store; }

void AlarmClockApp::onEnter() {
  layoutList();
  _state = State::List;
  M5.Display.fillScreen(Theme::kBackground);
}

void AlarmClockApp::layoutList() {
  const int screenW = M5.Display.width();
  const int top = kTop + 10;
  const int rowH = 56;
  const int padding = 8;
  const int delW = 100;

  int n = _store->count();
  for (int i = 0; i < n; ++i) {
    int y = top + i * (rowH + padding);
    _rowButtons[i].initButtonUL(&M5.Display, padding, y,
                                 screenW - delW - padding * 3, rowH,
                                 Theme::kOutline,
                                 _store->at(i).enabled ? Theme::kSuccess : Theme::kOutline,
                                 Theme::kTextPrimary, "", 2.0f, 2.0f);
    _deleteButtons[i].initButtonUL(&M5.Display, screenW - delW - padding, y,
                                    delW, rowH, Theme::kOutline, Theme::kDanger,
                                    Theme::kTextPrimary, "Delete", 1.8f, 1.8f);
  }

  // When the list is empty, reserve one row's worth of height for the
  // "No alarms set" message so the Add button doesn't land on top of it.
  int effectiveRows = n > 0 ? n : 1;
  int addY = top + effectiveRows * (rowH + padding) + 10;
  _addBtn.initButtonUL(&M5.Display, padding, addY, screenW - padding * 2, rowH,
                        Theme::kOutline, Theme::kSuccess, Theme::kTextPrimary,
                        "", 2.2f, 2.2f);
}

void AlarmClockApp::updateList() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  int n = _store->count();
  for (int i = 0; i < n; ++i) {
    bool insideRow = pressed && _rowButtons[i].contains(tx, ty);
    _rowButtons[i].press(insideRow);
    if (_rowButtons[i].justReleased()) {
      _store->at(i).enabled = !_store->at(i).enabled;
      _store->save();
      layoutList();
      M5.Display.fillScreen(Theme::kBackground);
      return;
    }

    bool insideDel = pressed && _deleteButtons[i].contains(tx, ty);
    _deleteButtons[i].press(insideDel);
    if (_deleteButtons[i].justReleased()) {
      _store->remove(i);
      layoutList();
      M5.Display.fillScreen(Theme::kBackground);
      return;
    }
  }

  bool insideAdd = pressed && _addBtn.contains(tx, ty);
  _addBtn.press(insideAdd);
  if (_addBtn.justReleased() && n < AlarmStore::kMaxAlarms) {
    _editHour = 7;
    _editMinute = 0;
    _editIsPM = false;
    _editRepeat = true;
    layoutAdd();
    _state = State::Add;
    M5.Display.fillScreen(Theme::kBackground);
  }
}

void AlarmClockApp::layoutAdd() {
  const int screenW = M5.Display.width();
  const int screenH = M5.Display.height();
  const int stepBtnSize = 90;
  const int centerY = kTop + 200;

  int hourX = screenW / 2 - 320;
  _hourDownBtn.initButtonUL(&M5.Display, hourX, centerY, stepBtnSize, stepBtnSize,
                             Theme::kOutline, Theme::kPrimary, Theme::kTextPrimary,
                             "-", 3.0f, 3.0f);
  _hourUpBtn.initButtonUL(&M5.Display, hourX + 180, centerY, stepBtnSize,
                           stepBtnSize, Theme::kOutline, Theme::kPrimary,
                           Theme::kTextPrimary, "+", 3.0f, 3.0f);

  int minX = screenW / 2 + 60;
  _minDownBtn.initButtonUL(&M5.Display, minX, centerY, stepBtnSize, stepBtnSize,
                            Theme::kOutline, Theme::kPrimary, Theme::kTextPrimary,
                            "-", 3.0f, 3.0f);
  _minUpBtn.initButtonUL(&M5.Display, minX + 180, centerY, stepBtnSize,
                          stepBtnSize, Theme::kOutline, Theme::kPrimary,
                          Theme::kTextPrimary, "+", 3.0f, 3.0f);

  _ampmBtn.initButtonUL(&M5.Display, screenW / 2 - 100, centerY + 130, 200, 70,
                         Theme::kOutline, Theme::kPrimary, Theme::kTextPrimary,
                         "", 2.2f, 2.2f);

  _repeatBtn.initButtonUL(&M5.Display, screenW / 2 - 150, centerY + 220, 300, 70,
                           Theme::kOutline,
                           _editRepeat ? Theme::kSuccess : Theme::kOutline,
                           Theme::kTextPrimary, "", 2.0f, 2.0f);

  _saveBtn.initButtonUL(&M5.Display, screenW / 2 - 320, screenH - 110, 280, 80,
                         Theme::kOutline, Theme::kSuccess, Theme::kTextPrimary,
                         "Save", 2.4f, 2.4f);
  _cancelBtn.initButtonUL(&M5.Display, screenW / 2 + 40, screenH - 110, 280, 80,
                           Theme::kOutline, Theme::kDanger, Theme::kTextPrimary,
                           "Cancel", 2.4f, 2.4f);
}

void AlarmClockApp::updateAdd() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  bool insideHourUp = pressed && _hourUpBtn.contains(tx, ty);
  _hourUpBtn.press(insideHourUp);
  if (_hourUpBtn.justReleased()) {
    _editHour = (_editHour % 12) + 1;
    return;
  }

  bool insideHourDown = pressed && _hourDownBtn.contains(tx, ty);
  _hourDownBtn.press(insideHourDown);
  if (_hourDownBtn.justReleased()) {
    _editHour = (_editHour == 1) ? 12 : _editHour - 1;
    return;
  }

  bool insideMinUp = pressed && _minUpBtn.contains(tx, ty);
  _minUpBtn.press(insideMinUp);
  if (_minUpBtn.justReleased()) {
    _editMinute = (_editMinute + 5) % 60;
    return;
  }

  bool insideMinDown = pressed && _minDownBtn.contains(tx, ty);
  _minDownBtn.press(insideMinDown);
  if (_minDownBtn.justReleased()) {
    _editMinute = (_editMinute + 55) % 60;
    return;
  }

  bool insideAmpm = pressed && _ampmBtn.contains(tx, ty);
  _ampmBtn.press(insideAmpm);
  if (_ampmBtn.justReleased()) {
    _editIsPM = !_editIsPM;
    return;
  }

  bool insideRepeat = pressed && _repeatBtn.contains(tx, ty);
  _repeatBtn.press(insideRepeat);
  if (_repeatBtn.justReleased()) {
    _editRepeat = !_editRepeat;
    layoutAdd();
    return;
  }

  bool insideSave = pressed && _saveBtn.contains(tx, ty);
  _saveBtn.press(insideSave);
  if (_saveBtn.justReleased()) {
    int hour24 = _editIsPM ? (_editHour == 12 ? 12 : _editHour + 12)
                            : (_editHour == 12 ? 0 : _editHour);
    _store->add(hour24, _editMinute, _editRepeat);
    layoutList();
    _state = State::List;
    M5.Display.fillScreen(Theme::kBackground);
    return;
  }

  bool insideCancel = pressed && _cancelBtn.contains(tx, ty);
  _cancelBtn.press(insideCancel);
  if (_cancelBtn.justReleased()) {
    layoutList();
    _state = State::List;
    M5.Display.fillScreen(Theme::kBackground);
  }
}

void AlarmClockApp::update() {
  switch (_state) {
    case State::List:
      updateList();
      break;
    case State::Add:
      updateAdd();
      break;
  }
}

void AlarmClockApp::drawList() {
  int n = _store->count();
  if (n == 0) {
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextColor(Theme::kTextSecondary, Theme::kBackground);
    M5.Display.setTextSize(2);
    M5.Display.drawString("No alarms set", 10, kTop + 10);
  }

  char label[32];
  for (int i = 0; i < n; ++i) {
    const Alarm& a = _store->at(i);
    int hour12 = a.hour % 12;
    if (hour12 == 0) hour12 = 12;
    const char* ampm = a.hour < 12 ? "AM" : "PM";
    snprintf(label, sizeof(label), "%d:%02d %s  %s  %s", hour12, a.minute, ampm,
             a.repeat ? "Daily" : "Once", a.enabled ? "ON" : "OFF");
    _rowButtons[i].drawButton(false, label);
    _deleteButtons[i].drawButton();
  }
  _addBtn.drawButton(false, "Add Alarm");
}

void AlarmClockApp::drawAdd() {
  const int screenW = M5.Display.width();
  M5.Display.setTextDatum(top_left);
  M5.Display.setTextColor(Theme::kTextSecondary, Theme::kBackground);
  M5.Display.setTextSize(2);
  M5.Display.drawString("New Alarm", 10, kTop + 12);

  _hourDownBtn.drawButton();
  _hourUpBtn.drawButton();
  _minDownBtn.drawButton();
  _minUpBtn.drawButton();

  char hourStr[4], minStr[4];
  snprintf(hourStr, sizeof(hourStr), "%d", _editHour);
  snprintf(minStr, sizeof(minStr), "%02d", _editMinute);

  int centerY = kTop + 200 + 45;
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
  M5.Display.setTextSize(4);
  M5.Display.drawString(hourStr, screenW / 2 - 320 + 90 + 45, centerY);
  M5.Display.drawString(minStr, screenW / 2 + 60 + 90 + 45, centerY);

  _ampmBtn.drawButton(false, _editIsPM ? "PM" : "AM");
  _repeatBtn.drawButton(false, _editRepeat ? "Repeat: Daily" : "Repeat: Once");
  _saveBtn.drawButton();
  _cancelBtn.drawButton();
}

void AlarmClockApp::draw() {
  switch (_state) {
    case State::List:
      drawList();
      break;
    case State::Add:
      drawAdd();
      break;
  }
}
