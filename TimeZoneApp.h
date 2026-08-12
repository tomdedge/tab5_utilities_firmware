#pragma once

#include <M5GFX.h>

#include "App.h"

// Grid of named UTC-offset zones (fixed offsets, no automatic DST -- there's
// a manual DST toggle) that immediately re-applies the RTC via
// TimeSync.cpp's applyTimezoneOffset(), which works offline as long as the
// clock was ever correctly synced once.
class TimeZoneApp : public App {
 public:
  void onEnter() override;
  void update() override;
  void draw() override;
  const char* name() const override { return "Time Zone"; }

 private:
  static constexpr int kZoneCount = 16;
  static constexpr int kCols = 4;

  struct ZoneInfo {
    const char* label;
    long offsetSec;
  };
  static const ZoneInfo kZones[kZoneCount];

  void layout();
  void select(int index);
  void loadSelection();
  void saveSelection();

  LGFX_Button _zoneButtons[kZoneCount];
  LGFX_Button _dstBtn;

  int _selectedIndex = 0;
  bool _dst = false;
};
