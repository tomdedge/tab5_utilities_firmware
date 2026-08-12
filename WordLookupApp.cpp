#include "WordLookupApp.h"

#include "Theme.h"

#include <M5Unified.h>

#include <cstdio>
#include <cstring>

#include "AppManager.h"
#include "TextBox.h"

namespace {
const int kTop = AppManager::kStatusBarHeight;
}  // namespace

void WordLookupApp::layout() {
  const int screenW = M5.Display.width();
  const int padding = 10;
  const int tabY = kTop + 10;
  const int tabH = 64;
  const int tabW = 220;

  _defTabBtn.initButtonUL(
      &M5.Display, padding, tabY, tabW, tabH, Theme::kOutline,
      _mode == Mode::Definitions ? Theme::kSuccess : Theme::kBackground, Theme::kTextPrimary, "",
      2.2f, 2.2f);
  _synTabBtn.initButtonUL(
      &M5.Display, padding * 2 + tabW, tabY, tabW, tabH, Theme::kOutline,
      _mode == Mode::Synonyms ? Theme::kSuccess : Theme::kBackground, Theme::kTextPrimary, "",
      2.2f, 2.2f);

  _newSearchBtn.initButtonUL(&M5.Display, screenW - 220 - padding, tabY, 220,
                              tabH, Theme::kOutline, Theme::kPrimary, Theme::kTextPrimary, "",
                              2.2f, 2.2f);
}

void WordLookupApp::beginEntry() {
  _keyboard.begin(&M5.Display, "Look up a word", _word, sizeof(_word) - 1,
                   /*maskInput=*/false);
  _state = State::Entry;
  M5.Display.fillScreen(Theme::kBackground);
}

void WordLookupApp::onEnter() {
  layout();
  beginEntry();
}

void WordLookupApp::doLookup() {
  bool ok = (_mode == Mode::Definitions)
                ? _client.lookupDefinitions(_word, _resultText,
                                             sizeof(_resultText))
                : _client.lookupSynonyms(_word, _resultText,
                                          sizeof(_resultText));

  if (ok) {
    _state = State::Result;
  } else {
    int code = _client.lastHttpCode();
    if (code == WordClient::kNoResults) {
      strcpy(_errorMsg, "No results found");
    } else {
      snprintf(_errorMsg, sizeof(_errorMsg), "Network error (code %d)", code);
    }
    _state = State::Error;
  }
  M5.Display.fillScreen(Theme::kBackground);
}

void WordLookupApp::update() {
  switch (_state) {
    case State::Entry: {
      _keyboard.update();
      if (_keyboard.isDone()) {
        const char* text = _keyboard.text();
        if (strlen(text) == 0) return;
        strncpy(_word, text, sizeof(_word) - 1);
        _word[sizeof(_word) - 1] = 0;
        _state = State::Loading;
        M5.Display.fillScreen(Theme::kBackground);
      } else if (_keyboard.isCancelled()) {
        if (strlen(_word) > 0) {
          _state = State::Result;
          M5.Display.fillScreen(Theme::kBackground);
        } else {
          beginEntry();  // re-arms Keyboard's cancelled flag too
        }
      }
      break;
    }

    case State::Loading:
      doLookup();
      break;

    case State::Result:
    case State::Error: {
      bool touched = M5.Touch.getCount() > 0;
      int tx = 0, ty = 0;
      bool pressed = false;
      if (touched) {
        auto t = M5.Touch.getDetail(0);
        tx = t.x;
        ty = t.y;
        pressed = t.isPressed();
      }

      bool insideDef = pressed && _defTabBtn.contains(tx, ty);
      _defTabBtn.press(insideDef);
      if (_defTabBtn.justReleased() && _mode != Mode::Definitions) {
        _mode = Mode::Definitions;
        _state = State::Loading;
        M5.Display.fillScreen(Theme::kBackground);
        return;
      }

      bool insideSyn = pressed && _synTabBtn.contains(tx, ty);
      _synTabBtn.press(insideSyn);
      if (_synTabBtn.justReleased() && _mode != Mode::Synonyms) {
        _mode = Mode::Synonyms;
        _state = State::Loading;
        M5.Display.fillScreen(Theme::kBackground);
        return;
      }

      bool insideNew = pressed && _newSearchBtn.contains(tx, ty);
      _newSearchBtn.press(insideNew);
      if (_newSearchBtn.justReleased()) {
        beginEntry();
      }
      break;
    }
  }
}

void WordLookupApp::draw() {
  switch (_state) {
    case State::Entry:
      _keyboard.draw();
      break;

    case State::Loading:
      M5.Display.setTextDatum(middle_center);
      M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
      M5.Display.setTextSize(3);
      M5.Display.drawString("Looking up...", M5.Display.width() / 2,
                             M5.Display.height() / 2);
      break;

    case State::Result: {
      _defTabBtn.drawButton(false, "Definitions");
      _synTabBtn.drawButton(false, "Synonyms");
      _newSearchBtn.drawButton(false, "New Search");

      const int contentTop = kTop + 90;
      M5.Display.fillRect(0, contentTop, M5.Display.width(), 40, Theme::kBackground);
      M5.Display.setTextDatum(top_left);
      M5.Display.setTextColor(Theme::kTextSecondary, Theme::kBackground);
      M5.Display.setTextSize(2);
      M5.Display.drawString(_word, 10, contentTop);

      drawWrappedText(&M5.Display, 10, contentTop + 44,
                       M5.Display.width() - 20,
                       M5.Display.height() - (contentTop + 44) - 10,
                       _resultText, 2);
      break;
    }

    case State::Error:
      _defTabBtn.drawButton(false, "Definitions");
      _synTabBtn.drawButton(false, "Synonyms");
      _newSearchBtn.drawButton(false, "New Search");
      M5.Display.setTextDatum(middle_center);
      M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
      M5.Display.setTextSize(3);
      M5.Display.drawString(_errorMsg, M5.Display.width() / 2,
                             M5.Display.height() / 2);
      break;
  }
}
