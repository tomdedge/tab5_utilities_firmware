#pragma once

#include <M5GFX.h>

#include "App.h"
#include "Theme.h"
#include "TimeZoneApp.h"
#include "WifiSettingsApp.h"

// Combines Wi-Fi and Time Zone into one menu tile. Both sub-apps already
// implement the full App interface and own their entire screen below the
// status bar, so rather than rebuilding their layouts to fit under a
// shared tab bar, this is just a small 2-tile picker that hands off to
// whichever sub-app's existing, unmodified UI. Getting back to the picker
// is via the global Home button + reopening Settings -- onExit() resets
// back to the picker so that always works.
class SettingsApp : public App {
 public:
  void onEnter() override;
  void onExit() override;
  void update() override;
  void draw() override;
  const char* name() const override { return "Settings"; }
  uint32_t tileColor() const override { return Theme::kSuccess; }

  // Lets other apps (Weather) jump straight into the Wi-Fi section instead
  // of landing on the picker. Call before AppManager::switchTo() to this
  // app.
  void openWifiSection();

 private:
  enum class Section { Picker, Wifi, TimeZone };

  void layoutPicker();
  void updatePicker();
  void drawPicker();

  Section _section = Section::Picker;
  bool _forceWifiSection = false;
  // AppManager::switchTo() clears the screen *after* calling onEnter(), so
  // the picker can't paint itself eagerly there -- it would just get wiped.
  // Instead draw() paints it once, lazily, the first time it's called
  // after becoming active (see SettingsApp.cpp).
  bool _pickerDirty = false;

  WifiSettingsApp _wifi;
  TimeZoneApp _timeZone;

  LGFX_Button _wifiTileBtn, _timeZoneTileBtn;
};
