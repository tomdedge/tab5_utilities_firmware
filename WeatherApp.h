#pragma once

#include <M5GFX.h>

#include "App.h"
#include "Keyboard.h"
#include "WeatherClient.h"

class AppManager;
class SettingsApp;

// Shows current conditions for a saved city (Open-Meteo, no API key). Asks
// for a city name the first time it's opened (or via "Change Location"),
// geocodes and caches lat/long via Preferences, and offers a shortcut into
// Settings' Wi-Fi section if there's no connection yet.
class WeatherApp : public App {
 public:
  void begin(AppManager* mgr, int settingsIndex, SettingsApp* settingsApp);

  void onEnter() override;
  void update() override;
  void draw() override;
  const char* name() const override { return "Weather"; }

 private:
  enum class State { NoWifi, LocationEntry, Loading, Show, Error };

  void loadLocation();
  void saveLocation();
  void layoutButtons();
  void beginLocationEntry();
  void setErrorFromLastRequest(const char* noResultsMsg);

  void updateNoWifi();
  void updateLocationEntry();
  void doFetch();
  void updateShow();
  void updateError();

  void drawNoWifi();
  void drawShow();
  void drawError();

  AppManager* _mgr = nullptr;
  int _settingsIndex = -1;
  SettingsApp* _settingsApp = nullptr;

  State _state = State::NoWifi;

  bool _hasLocation = false;
  double _lat = 0, _lon = 0;
  char _locationName[48] = {0};

  WeatherClient _client;
  WeatherResult _result;
  char _errorMsg[64] = {0};

  Keyboard _keyboard;

  LGFX_Button _openWifiBtn;
  LGFX_Button _refreshBtn;
  LGFX_Button _changeLocationBtn;
  LGFX_Button _retryBtn;
  LGFX_Button _errorChangeLocationBtn;
};
