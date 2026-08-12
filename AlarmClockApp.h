#pragma once

#include <M5GFX.h>

#include "AlarmStore.h"
#include "App.h"

// Lists existing alarms (tap a row to toggle on/off, Delete to remove) and
// an "Add Alarm" screen with hour/minute steppers, AM/PM and repeat toggles.
// Actually firing an alarm happens outside this app -- see AppManager's
// ringAlarm()/alarm overlay -- since it must interrupt whatever app is on
// screen, not just this one.
class AlarmClockApp : public App {
 public:
  void begin(AlarmStore* store);

  void onEnter() override;
  void update() override;
  void draw() override;
  const char* name() const override { return "Alarm Clock"; }
  uint32_t tileColor() const override { return Theme::kPrimary; }

 private:
  enum class State { List, Add };

  void layoutList();
  void layoutAdd();
  void updateList();
  void updateAdd();
  void drawList();
  void drawAdd();

  AlarmStore* _store = nullptr;
  State _state = State::List;

  LGFX_Button _rowButtons[AlarmStore::kMaxAlarms];
  LGFX_Button _deleteButtons[AlarmStore::kMaxAlarms];
  LGFX_Button _addBtn;

  int _editHour = 7;      // 1-12, display value while editing
  int _editMinute = 0;
  bool _editIsPM = false;
  bool _editRepeat = true;
  LGFX_Button _hourUpBtn, _hourDownBtn, _minUpBtn, _minDownBtn;
  LGFX_Button _ampmBtn, _repeatBtn, _saveBtn, _cancelBtn;
};
