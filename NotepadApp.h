#pragma once

#include <M5GFX.h>

#include "App.h"
#include "Keyboard.h"
#include "NoteStore.h"

// Lists notes stored on the SD card, and opens the shared Keyboard (in
// multiline mode) to create or edit one. Notes are just timestamp-named
// text files under /notes.
class NotepadApp : public App {
 public:
  void onEnter() override;
  void update() override;
  void draw() override;
  const char* name() const override { return "Notepad"; }

 private:
  enum class State { NoSd, List, Editing };

  static constexpr int kMaxRows = 8;

  void layoutList();
  void updateList();
  void drawList();
  void beginEdit(int index);  // -1 = new note

  NoteStore _store;
  State _state = State::List;

  LGFX_Button _rowButtons[kMaxRows];
  LGFX_Button _deleteButtons[kMaxRows];
  LGFX_Button _newBtn;
  int _rowCount = 0;

  Keyboard _keyboard;
  int _editingIndex = -1;
  char _editBuffer[NoteStore::kMaxNoteBytes] = {0};
};
