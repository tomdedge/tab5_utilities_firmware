#pragma once

#include <M5GFX.h>

#include "App.h"

class AppManager;

// Grid of tappable tiles, one per registered app (excluding itself). Tapping
// a tile tells the AppManager to switch to that app.
class MenuApp : public App {
 public:
  void begin(AppManager* mgr, App** apps, int count);

  void onEnter() override;
  void update() override;
  void draw() override;
  const char* name() const override { return "Menu"; }

 private:
  static constexpr int kMaxTiles = 12;

  void layoutButtons();
  int tileCount() const;

  AppManager* _mgr = nullptr;
  App** _apps = nullptr;
  int _count = 0;
  LGFX_Button _buttons[kMaxTiles];
};
