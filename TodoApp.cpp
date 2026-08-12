#include "TodoApp.h"

#include "Theme.h"

#include <M5Unified.h>
#include <Preferences.h>

#include <cstdio>
#include <cstring>

#include "AppManager.h"

namespace {
const int kTop = AppManager::kStatusBarHeight;
const char* kPrefsNamespace = "todo";
}  // namespace

void TodoApp::load() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, /*readOnly=*/true);
  _count = prefs.getInt("count", 0);
  if (_count < 0) _count = 0;
  if (_count > kMaxItems) _count = kMaxItems;

  char key[16];
  for (int i = 0; i < _count; ++i) {
    snprintf(key, sizeof(key), "item%d", i);
    prefs.getString(key, _items[i].text, kItemLen);
    snprintf(key, sizeof(key), "done%d", i);
    _items[i].done = prefs.getBool(key, false);
  }
  prefs.end();
}

void TodoApp::save() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, /*readOnly=*/false);
  prefs.putInt("count", _count);

  char key[16];
  for (int i = 0; i < _count; ++i) {
    snprintf(key, sizeof(key), "item%d", i);
    prefs.putString(key, _items[i].text);
    snprintf(key, sizeof(key), "done%d", i);
    prefs.putBool(key, _items[i].done);
  }
  prefs.end();
}

void TodoApp::layout() {
  const int screenW = M5.Display.width();
  const int top = kTop + 10;
  const int rowH = 56;
  const int padding = 8;
  const int delW = 100;

  for (int i = 0; i < _count; ++i) {
    int y = top + i * (rowH + padding);
    _rowButtons[i].initButtonUL(&M5.Display, padding, y,
                                 screenW - delW - padding * 3, rowH,
                                 Theme::kOutline,
                                 _items[i].done ? Theme::kSuccess : Theme::kPrimary,
                                 Theme::kTextPrimary, "", 2.0f, 2.0f);
    _deleteButtons[i].initButtonUL(&M5.Display, screenW - delW - padding, y,
                                    delW, rowH, Theme::kOutline, Theme::kDanger,
                                    Theme::kTextPrimary, "Delete", 1.8f, 1.8f);
  }

  // When the list is empty, reserve one row's worth of height for the
  // "No to-do items yet" message so the Add button doesn't land on top of
  // it.
  int effectiveRows = _count > 0 ? _count : 1;
  int addY = top + effectiveRows * (rowH + padding) + 10;
  _addBtn.initButtonUL(&M5.Display, padding, addY, screenW - padding * 2,
                        rowH, Theme::kOutline, Theme::kSuccess, Theme::kTextPrimary, "",
                        2.2f, 2.2f);
}

void TodoApp::onEnter() {
  load();
  layout();
  _adding = false;
  M5.Display.fillScreen(Theme::kBackground);
}

void TodoApp::updateList() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  for (int i = 0; i < _count; ++i) {
    bool insideRow = pressed && _rowButtons[i].contains(tx, ty);
    _rowButtons[i].press(insideRow);
    if (_rowButtons[i].justReleased()) {
      _items[i].done = !_items[i].done;
      save();
      layout();
      M5.Display.fillScreen(Theme::kBackground);
      return;
    }

    bool insideDel = pressed && _deleteButtons[i].contains(tx, ty);
    _deleteButtons[i].press(insideDel);
    if (_deleteButtons[i].justReleased()) {
      for (int j = i; j < _count - 1; ++j) _items[j] = _items[j + 1];
      _count--;
      save();
      layout();
      M5.Display.fillScreen(Theme::kBackground);
      return;
    }
  }

  bool insideAdd = pressed && _addBtn.contains(tx, ty);
  _addBtn.press(insideAdd);
  if (_addBtn.justReleased() && _count < kMaxItems) {
    _keyboard.begin(&M5.Display, "New To-Do Item", "", kItemLen - 1,
                     /*maskInput=*/false);
    _adding = true;
    M5.Display.fillScreen(Theme::kBackground);
  }
}

void TodoApp::update() {
  if (_adding) {
    _keyboard.update();
    if (_keyboard.isDone()) {
      const char* text = _keyboard.text();
      if (strlen(text) > 0 && _count < kMaxItems) {
        strncpy(_items[_count].text, text, kItemLen - 1);
        _items[_count].text[kItemLen - 1] = 0;
        _items[_count].done = false;
        _count++;
        save();
      }
      _adding = false;
      layout();
      M5.Display.fillScreen(Theme::kBackground);
    } else if (_keyboard.isCancelled()) {
      _adding = false;
      layout();
      M5.Display.fillScreen(Theme::kBackground);
    }
    return;
  }
  updateList();
}

void TodoApp::drawList() {
  if (_count == 0) {
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextColor(Theme::kTextSecondary, Theme::kBackground);
    M5.Display.setTextSize(2);
    M5.Display.drawString("No to-do items yet", 10, kTop + 10);
  }

  char label[kItemLen + 4];
  for (int i = 0; i < _count; ++i) {
    snprintf(label, sizeof(label), "%s %s", _items[i].done ? "[x]" : "[ ]",
             _items[i].text);
    _rowButtons[i].drawButton(false, label);
    _deleteButtons[i].drawButton();
  }
  _addBtn.drawButton(false, "Add Item");
}

void TodoApp::draw() {
  if (_adding) {
    _keyboard.draw();
    return;
  }
  drawList();
}
