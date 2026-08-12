#pragma once

// Persisted alarm list plus due-checking. Owned by Test.ino as a single
// instance shared between the background check in loop() (fires regardless
// of which app is active, via AppManager::ringAlarm()) and AlarmClockApp
// (the management UI for adding/toggling/deleting alarms).
struct Alarm {
  int hour = 7;    // 0-23
  int minute = 0;
  bool enabled = true;
  bool repeat = true;  // true = fires daily, false = auto-disables after firing once
};

class AlarmStore {
 public:
  static constexpr int kMaxAlarms = 8;

  void load();
  void save();

  int count() const { return _count; }
  const Alarm& at(int i) const { return _alarms[i]; }
  Alarm& at(int i) { return _alarms[i]; }

  bool add(int hour, int minute, bool repeat);
  bool remove(int index);

  // Call once per frame. Cheap -- only evaluates alarms when the clock's
  // minute has actually changed, so it won't re-fire repeatedly within the
  // same minute. Returns the index of an alarm that just became due, or -1.
  int checkDue();

 private:
  Alarm _alarms[kMaxAlarms];
  int _count = 0;
  int _lastCheckedMinuteOfDay = -1;
};
