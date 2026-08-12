#include "NotepadApp.h"

#include "Theme.h"

#include <M5Unified.h>

#include "AppManager.h"

namespace {
const int kTop = AppManager::kStatusBarHeight;
}

void NotepadApp::onEnter() {
  if (!_store.sdReady()) {
    _state = State::NoSd;
    M5.Display.fillScreen(Theme::kBackground);
    return;
  }
  _store.refresh();
  layoutList();
  _state = State::List;
  M5.Display.fillScreen(Theme::kBackground);
}

void NotepadApp::layoutList() {
  const int screenW = M5.Display.width();
  const int top = kTop + 10;
  const int rowH = 56;
  const int padding = 8;
  const int delW = 100;

  _rowCount = _store.count();
  if (_rowCount > kMaxRows) _rowCount = kMaxRows;

  for (int i = 0; i < _rowCount; ++i) {
    int y = top + i * (rowH + padding);
    _rowButtons[i].initButtonUL(&M5.Display, padding, y,
                                 screenW - delW - padding * 3, rowH,
                                 Theme::kOutline, Theme::kPrimary, Theme::kTextPrimary, "", 2.0f,
                                 2.0f);
    _deleteButtons[i].initButtonUL(&M5.Display, screenW - delW - padding, y,
                                    delW, rowH, Theme::kOutline, Theme::kDanger,
                                    Theme::kTextPrimary, "Delete", 1.8f, 1.8f);
  }

  // When the list is empty, reserve one row's worth of height for the
  // "No notes yet" message so the New button doesn't land on top of it.
  int effectiveRows = _rowCount > 0 ? _rowCount : 1;
  int newY = top + effectiveRows * (rowH + padding) + 10;
  _newBtn.initButtonUL(&M5.Display, padding, newY, screenW - padding * 2,
                        rowH, Theme::kOutline, Theme::kSuccess, Theme::kTextPrimary, "", 2.2f,
                        2.2f);
}

void NotepadApp::beginEdit(int index) {
  _editingIndex = index;
  const char* title;
  if (index >= 0) {
    _store.readNote(index, _editBuffer, sizeof(_editBuffer));
    title = "Edit Note";
  } else {
    _editBuffer[0] = 0;
    title = "New Note";
  }
  _keyboard.begin(&M5.Display, title, _editBuffer, NoteStore::kMaxNoteBytes - 1,
                   /*maskInput=*/false, /*multiline=*/true);
  _state = State::Editing;
  M5.Display.fillScreen(Theme::kBackground);
}

void NotepadApp::updateList() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  for (int i = 0; i < _rowCount; ++i) {
    bool insideRow = pressed && _rowButtons[i].contains(tx, ty);
    _rowButtons[i].press(insideRow);
    if (_rowButtons[i].justReleased()) {
      beginEdit(i);
      return;
    }

    bool insideDel = pressed && _deleteButtons[i].contains(tx, ty);
    _deleteButtons[i].press(insideDel);
    if (_deleteButtons[i].justReleased()) {
      _store.deleteNote(i);
      _store.refresh();
      layoutList();
      M5.Display.fillScreen(Theme::kBackground);
      return;
    }
  }

  bool insideNew = pressed && _newBtn.contains(tx, ty);
  _newBtn.press(insideNew);
  if (_newBtn.justReleased()) {
    beginEdit(-1);
    return;
  }
}

void NotepadApp::update() {
  switch (_state) {
    case State::NoSd:
      break;

    case State::List:
      updateList();
      break;

    case State::Editing:
      _keyboard.update();
      if (_keyboard.isDone()) {
        if (_editingIndex >= 0) {
          _store.updateNote(_editingIndex, _keyboard.text());
        } else {
          _store.createNote(_keyboard.text(), nullptr, 0);
        }
        _store.refresh();
        layoutList();
        _state = State::List;
        M5.Display.fillScreen(Theme::kBackground);
      } else if (_keyboard.isCancelled()) {
        _state = State::List;
        M5.Display.fillScreen(Theme::kBackground);
      }
      break;
  }
}

void NotepadApp::drawList() {
  if (_rowCount == 0) {
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextColor(Theme::kTextSecondary, Theme::kBackground);
    M5.Display.setTextSize(2);
    M5.Display.drawString("No notes yet", 10, kTop + 10);
  }

  for (int i = 0; i < _rowCount; ++i) {
    _rowButtons[i].drawButton(false, _store.at(i).filename);
    _deleteButtons[i].drawButton();
  }
  _newBtn.drawButton(false, "New Note");
}

void NotepadApp::draw() {
  switch (_state) {
    case State::NoSd:
      M5.Display.setTextDatum(middle_center);
      M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
      M5.Display.setTextSize(3);
      M5.Display.drawString("No SD card detected", M5.Display.width() / 2,
                             M5.Display.height() / 2);
      break;

    case State::List:
      drawList();
      break;

    case State::Editing:
      _keyboard.draw();
      break;
  }
}
