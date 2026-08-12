#include "AlarmStore.h"

#include <M5Unified.h>
#include <Preferences.h>

#include <cstdio>

namespace {
const char* kPrefsNamespace = "alarms";
}  // namespace

void AlarmStore::load() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, /*readOnly=*/true);
  _count = prefs.getInt("count", 0);
  if (_count < 0) _count = 0;
  if (_count > kMaxAlarms) _count = kMaxAlarms;

  char key[16];
  for (int i = 0; i < _count; ++i) {
    snprintf(key, sizeof(key), "hour%d", i);
    _alarms[i].hour = prefs.getInt(key, 7);
    snprintf(key, sizeof(key), "min%d", i);
    _alarms[i].minute = prefs.getInt(key, 0);
    snprintf(key, sizeof(key), "en%d", i);
    _alarms[i].enabled = prefs.getBool(key, true);
    snprintf(key, sizeof(key), "rep%d", i);
    _alarms[i].repeat = prefs.getBool(key, true);
  }
  prefs.end();
}

void AlarmStore::save() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, /*readOnly=*/false);
  prefs.putInt("count", _count);

  char key[16];
  for (int i = 0; i < _count; ++i) {
    snprintf(key, sizeof(key), "hour%d", i);
    prefs.putInt(key, _alarms[i].hour);
    snprintf(key, sizeof(key), "min%d", i);
    prefs.putInt(key, _alarms[i].minute);
    snprintf(key, sizeof(key), "en%d", i);
    prefs.putBool(key, _alarms[i].enabled);
    snprintf(key, sizeof(key), "rep%d", i);
    prefs.putBool(key, _alarms[i].repeat);
  }
  prefs.end();
}

bool AlarmStore::add(int hour, int minute, bool repeat) {
  if (_count >= kMaxAlarms) return false;
  _alarms[_count].hour = hour;
  _alarms[_count].minute = minute;
  _alarms[_count].enabled = true;
  _alarms[_count].repeat = repeat;
  _count++;
  save();
  return true;
}

bool AlarmStore::remove(int index) {
  if (index < 0 || index >= _count) return false;
  for (int i = index; i < _count - 1; ++i) _alarms[i] = _alarms[i + 1];
  _count--;
  save();
  return true;
}

int AlarmStore::checkDue() {
  if (!M5.Rtc.isEnabled()) return -1;
  auto dt = M5.Rtc.getDateTime();
  int minuteOfDay = dt.time.hours * 60 + dt.time.minutes;
  if (minuteOfDay == _lastCheckedMinuteOfDay) return -1;
  _lastCheckedMinuteOfDay = minuteOfDay;

  for (int i = 0; i < _count; ++i) {
    if (_alarms[i].enabled && _alarms[i].hour == dt.time.hours &&
        _alarms[i].minute == dt.time.minutes) {
      if (!_alarms[i].repeat) {
        _alarms[i].enabled = false;
        save();
      }
      return i;
    }
  }
  return -1;
}
