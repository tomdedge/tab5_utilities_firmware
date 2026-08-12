#pragma once

#include <M5GFX.h>

#include "App.h"
#include "Keyboard.h"
#include "NewsClient.h"

// Feed picker (a few verified presets + custom URL via Keyboard) ->
// headline list -> tap a headline for its description via drawWrappedText().
// Selected feed persists to Preferences so it's remembered next time.
class NewsApp : public App {
 public:
  void onEnter() override;
  void update() override;
  void draw() override;
  const char* name() const override { return "News"; }

  // Public so the preset table in the .cpp can size against it.
  static constexpr int kPresetCount = 3;

 private:
  enum class State { Picker, Loading, List, Detail, Error };

  void layoutPicker();
  void layoutList();
  void updatePicker();
  void updateList();
  void updateDetail();
  void updateError();
  void drawPicker();
  void drawList();
  void drawDetail();
  void drawError();

  void loadSavedFeed();
  void saveFeed();
  void startFetch(const char* url);
  void doFetch();

  State _state = State::Picker;
  bool _enteringCustomUrl = false;
  bool _hasFeed = false;  // true once a fetch has ever succeeded this session

  char _feedUrl[160] = {0};
  char _feedTitle[64] = "News";

  NewsClient _client;
  int _selectedItem = -1;

  Keyboard _keyboard;

  LGFX_Button _presetButtons[kPresetCount];
  LGFX_Button _customBtn;
  LGFX_Button _cancelPickerBtn;  // only shown/active when _hasFeed
  LGFX_Button _headlineButtons[NewsClient::kMaxItems];
  LGFX_Button _backBtn;
  LGFX_Button _refreshBtn;

  char _errorMsg[64] = {0};
};
