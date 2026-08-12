#include "WeatherApp.h"

#include "Theme.h"

#include <M5Unified.h>
#include <Preferences.h>
#include <WiFi.h>

#include <cstdio>
#include <cstring>

#include "AppManager.h"
#include "SettingsApp.h"

namespace {
const int kTop = AppManager::kStatusBarHeight;
const char* kPrefsNamespace = "weather";
}  // namespace

void WeatherApp::begin(AppManager* mgr, int settingsIndex,
                        SettingsApp* settingsApp) {
  _mgr = mgr;
  _settingsIndex = settingsIndex;
  _settingsApp = settingsApp;
}

void WeatherApp::loadLocation() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, /*readOnly=*/true);
  _hasLocation = prefs.isKey("lat");
  if (_hasLocation) {
    _lat = prefs.getDouble("lat", 0.0);
    _lon = prefs.getDouble("lon", 0.0);
    prefs.getString("name", _locationName, sizeof(_locationName));
  }
  prefs.end();
}

void WeatherApp::saveLocation() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, /*readOnly=*/false);
  prefs.putDouble("lat", _lat);
  prefs.putDouble("lon", _lon);
  prefs.putString("name", _locationName);
  prefs.end();
  _hasLocation = true;
}

void WeatherApp::layoutButtons() {
  const int screenW = M5.Display.width();
  const int screenH = M5.Display.height();
  const int btnW = 300, btnH = 64;

  _openWifiBtn.initButtonUL(&M5.Display, screenW / 2 - btnW / 2,
                             screenH / 2 + 40, btnW, btnH, Theme::kOutline,
                             Theme::kPrimary, Theme::kTextPrimary, "", 2.0f, 2.0f);

  _refreshBtn.initButtonUL(&M5.Display, screenW - btnW - 16,
                            screenH - btnH - 16, btnW, btnH, Theme::kOutline,
                            Theme::kSuccess, Theme::kTextPrimary, "Refresh", 2.0f, 2.0f);

  _changeLocationBtn.initButtonUL(&M5.Display, 16, screenH - btnH - 16, btnW,
                                   btnH, Theme::kOutline, Theme::kPrimary, Theme::kTextPrimary, "",
                                   2.0f, 2.0f);

  _retryBtn.initButtonUL(&M5.Display, screenW / 2 - btnW / 2,
                          screenH / 2 + 20, btnW, btnH, Theme::kOutline,
                          Theme::kSuccess, Theme::kTextPrimary, "Retry", 2.0f, 2.0f);

  _errorChangeLocationBtn.initButtonUL(
      &M5.Display, screenW / 2 - btnW / 2, screenH / 2 + 20 + btnH + 16, btnW,
      btnH, Theme::kOutline, Theme::kPrimary, Theme::kTextPrimary, "", 2.0f, 2.0f);
}

void WeatherApp::beginLocationEntry() {
  _keyboard.begin(&M5.Display, "Weather Location (city name)", "", 47,
                   /*maskInput=*/false);
  _state = State::LocationEntry;
  M5.Display.fillScreen(Theme::kBackground);
}

void WeatherApp::onEnter() {
  loadLocation();
  layoutButtons();

  if (WiFi.status() != WL_CONNECTED) {
    _state = State::NoWifi;
    M5.Display.fillScreen(Theme::kBackground);
  } else if (!_hasLocation) {
    beginLocationEntry();
  } else {
    _state = State::Loading;
    M5.Display.fillScreen(Theme::kBackground);
  }
}

void WeatherApp::updateNoWifi() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  bool inside = pressed && _openWifiBtn.contains(tx, ty);
  _openWifiBtn.press(inside);
  if (_openWifiBtn.justReleased() && _mgr && _settingsIndex >= 0) {
    if (_settingsApp) _settingsApp->openWifiSection();
    _mgr->switchTo(_settingsIndex);
  }
}

void WeatherApp::updateLocationEntry() {
  _keyboard.update();
  if (_keyboard.isDone()) {
    const char* city = _keyboard.text();
    if (strlen(city) == 0) return;  // ignore empty submit

    double lat, lon;
    char resolved[48];
    if (_client.geocode(city, lat, lon, resolved, sizeof(resolved))) {
      _lat = lat;
      _lon = lon;
      strncpy(_locationName, resolved, sizeof(_locationName) - 1);
      _locationName[sizeof(_locationName) - 1] = 0;
      saveLocation();
      _state = State::Loading;
    } else {
      setErrorFromLastRequest("City not found");
      _state = State::Error;
    }
    M5.Display.fillScreen(Theme::kBackground);
  } else if (_keyboard.isCancelled()) {
    if (_hasLocation) {
      _state = State::Loading;
    } else {
      beginLocationEntry();
      return;
    }
    M5.Display.fillScreen(Theme::kBackground);
  }
}

void WeatherApp::doFetch() {
  if (_client.fetchCurrent(_lat, _lon, _result)) {
    _state = State::Show;
  } else {
    setErrorFromLastRequest("No weather data for this location");
    _state = State::Error;
  }
  M5.Display.fillScreen(Theme::kBackground);
}

void WeatherApp::setErrorFromLastRequest(const char* noResultsMsg) {
  int code = _client.lastHttpCode();
  if (code == WeatherClient::kNoResults) {
    strncpy(_errorMsg, noResultsMsg, sizeof(_errorMsg) - 1);
    _errorMsg[sizeof(_errorMsg) - 1] = 0;
  } else if (code == WeatherClient::kParseError) {
    strcpy(_errorMsg, "Bad response from server");
  } else {
    snprintf(_errorMsg, sizeof(_errorMsg), "Network error (code %d)", code);
  }
}

void WeatherApp::updateShow() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  bool insideRefresh = pressed && _refreshBtn.contains(tx, ty);
  _refreshBtn.press(insideRefresh);
  if (_refreshBtn.justReleased()) {
    _state = State::Loading;
    M5.Display.fillScreen(Theme::kBackground);
    return;
  }

  bool insideChange = pressed && _changeLocationBtn.contains(tx, ty);
  _changeLocationBtn.press(insideChange);
  if (_changeLocationBtn.justReleased()) {
    beginLocationEntry();
  }
}

void WeatherApp::updateError() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  bool inside = pressed && _retryBtn.contains(tx, ty);
  _retryBtn.press(inside);
  if (_retryBtn.justReleased()) {
    if (WiFi.status() != WL_CONNECTED) {
      _state = State::NoWifi;
      M5.Display.fillScreen(Theme::kBackground);
    } else if (!_hasLocation) {
      beginLocationEntry();
    } else {
      _state = State::Loading;
      M5.Display.fillScreen(Theme::kBackground);
    }
    return;
  }

  bool insideChange = pressed && _errorChangeLocationBtn.contains(tx, ty);
  _errorChangeLocationBtn.press(insideChange);
  if (_errorChangeLocationBtn.justReleased()) {
    beginLocationEntry();
  }
}

void WeatherApp::update() {
  switch (_state) {
    case State::NoWifi:
      updateNoWifi();
      break;
    case State::LocationEntry:
      updateLocationEntry();
      break;
    case State::Loading:
      doFetch();
      break;
    case State::Show:
      updateShow();
      break;
    case State::Error:
      updateError();
      break;
  }
}

void WeatherApp::drawNoWifi() {
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
  M5.Display.setTextSize(3);
  M5.Display.drawString("Not connected to Wi-Fi", M5.Display.width() / 2,
                         M5.Display.height() / 2 - 20);
  _openWifiBtn.drawButton(false, "Open Settings");
}

void WeatherApp::drawShow() {
  const int screenW = M5.Display.width();
  const int screenH = M5.Display.height();

  M5.Display.setTextDatum(top_center);
  M5.Display.setTextColor(Theme::kTextSecondary, Theme::kBackground);
  M5.Display.setTextSize(3);
  M5.Display.drawString(_locationName, screenW / 2, kTop + 20);

  char tempStr[16];
  snprintf(tempStr, sizeof(tempStr), "%.0f F", _result.temperature);
  M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
  M5.Display.setTextSize(8);
  M5.Display.drawString(tempStr, screenW / 2, kTop + 100);

  M5.Display.setTextSize(3);
  M5.Display.drawString(_result.description, screenW / 2, kTop + 230);

  char windStr[32];
  snprintf(windStr, sizeof(windStr), "Wind: %.0f mph", _result.windSpeed);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(Theme::kTextSecondary, Theme::kBackground);
  M5.Display.drawString(windStr, screenW / 2, kTop + 280);

  _refreshBtn.drawButton();
  _changeLocationBtn.drawButton(false, "Change Location");

  // Open-Meteo's free tier is CC BY 4.0 and requires attribution -- see
  // https://open-meteo.com/en/terms.
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(Theme::kTextSecondary, Theme::kBackground);
  M5.Display.setTextSize(1);
  M5.Display.drawString("Weather data by Open-Meteo.com (CC BY 4.0)",
                         screenW / 2, screenH - 48);
}

void WeatherApp::drawError() {
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
  M5.Display.setTextSize(3);
  M5.Display.drawString(_errorMsg, M5.Display.width() / 2,
                         M5.Display.height() / 2 - 40);
  _retryBtn.drawButton();
  _errorChangeLocationBtn.drawButton(false, "Change Location");
}

void WeatherApp::draw() {
  switch (_state) {
    case State::NoWifi:
      drawNoWifi();
      break;
    case State::LocationEntry:
      _keyboard.draw();
      break;
    case State::Loading:
      M5.Display.setTextDatum(middle_center);
      M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
      M5.Display.setTextSize(3);
      M5.Display.drawString("Loading...", M5.Display.width() / 2,
                             M5.Display.height() / 2);
      break;
    case State::Show:
      drawShow();
      break;
    case State::Error:
      drawError();
      break;
  }
}
