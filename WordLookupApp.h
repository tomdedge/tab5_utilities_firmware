#pragma once

#include <M5GFX.h>

#include "App.h"
#include "Keyboard.h"
#include "WordClient.h"

// Keyboard word entry, then a Definitions/Synonyms mode tab, results shown
// via the shared drawWrappedText() helper.
class WordLookupApp : public App {
 public:
  void onEnter() override;
  void update() override;
  void draw() override;
  const char* name() const override { return "Dictionary"; }

 private:
  enum class State { Entry, Loading, Result, Error };
  enum class Mode { Definitions, Synonyms };

  void layout();
  void beginEntry();
  void doLookup();

  Mode _mode = Mode::Definitions;
  State _state = State::Entry;

  char _word[48] = {0};
  char _resultText[1200] = {0};
  char _errorMsg[64] = {0};

  WordClient _client;
  Keyboard _keyboard;

  LGFX_Button _defTabBtn, _synTabBtn;
  LGFX_Button _newSearchBtn;
};
