#pragma once

#include <M5GFX.h>

#include "App.h"
#include "Keyboard.h"

// List of up to 8 short to-do items. Tap a row to toggle done/not-done,
// Delete removes it, Add opens the shared Keyboard for new item text.
// Persisted to Preferences, same indexed-key pattern as WifiStore.
class TodoApp : public App {
 public:
  void onEnter() override;
  void update() override;
  void draw() override;
  const char* name() const override { return "To-Do List"; }

 private:
  static constexpr int kMaxItems = 8;
  static constexpr int kItemLen = 48;

  struct Item {
    char text[kItemLen];
    bool done;
  };

  void load();
  void save();
  void layout();
  void updateList();
  void drawList();

  Item _items[kMaxItems];
  int _count = 0;

  LGFX_Button _rowButtons[kMaxItems];
  LGFX_Button _deleteButtons[kMaxItems];
  LGFX_Button _addBtn;

  bool _adding = false;
  Keyboard _keyboard;
};
