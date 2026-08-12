#include "WifiSettingsApp.h"

#include "Theme.h"

#include <M5Unified.h>
#include <WiFi.h>

#include <cstdio>
#include <cstring>

#include "AppManager.h"
#include "TimeSync.h"

namespace {
const int kTop = AppManager::kStatusBarHeight;
}

void WifiSettingsApp::onEnter() {
  _store.load();
  _state = State::Main;
  layoutMain();
  M5.Display.fillScreen(Theme::kBackground);
}

void WifiSettingsApp::layoutMain() {
  const int screenW = M5.Display.width();
  const int top = kTop + 40;
  const int rowH = 56;
  const int padding = 8;
  const int forgetW = 100;

  _rowCount = _store.count();
  if (_rowCount > kMaxRows) _rowCount = kMaxRows;

  for (int i = 0; i < _rowCount; ++i) {
    int y = top + i * (rowH + padding);
    _rowButtons[i].initButtonUL(&M5.Display, padding, y,
                                 screenW - forgetW - padding * 3, rowH,
                                 Theme::kOutline, Theme::kPrimary, Theme::kTextPrimary, "", 2.0f,
                                 2.0f);
    _forgetButtons[i].initButtonUL(&M5.Display, screenW - forgetW - padding,
                                    y, forgetW, rowH, Theme::kOutline,
                                    Theme::kDanger, Theme::kTextPrimary, "Forget", 1.8f,
                                    1.8f);
  }

  int scanY = top + _rowCount * (rowH + padding) + 10;
  _scanBtn.initButtonUL(&M5.Display, padding, scanY, screenW - padding * 2,
                         rowH, Theme::kOutline, Theme::kSuccess, Theme::kTextPrimary, "",
                         2.2f, 2.2f);
}

void WifiSettingsApp::layoutScanResults() {
  const int screenW = M5.Display.width();
  const int top = kTop + 40;
  const int rowH = 56;
  const int padding = 8;

  for (int i = 0; i < _scanCount; ++i) {
    int y = top + i * (rowH + padding);
    _rowButtons[i].initButtonUL(&M5.Display, padding, y, screenW - padding * 2,
                                 rowH, Theme::kOutline, Theme::kPrimary, Theme::kTextPrimary, "",
                                 2.0f, 2.0f);
  }

  int backY = top + _scanCount * (rowH + padding) + 10;
  _backBtn.initButtonUL(&M5.Display, padding, backY, screenW - padding * 2,
                         rowH, Theme::kOutline, Theme::kDanger, Theme::kTextPrimary, "Back",
                         2.2f, 2.2f);
}

void WifiSettingsApp::startScan() {
  WiFi.scanNetworks(/*async=*/true);
  _state = State::Scanning;
  M5.Display.fillScreen(Theme::kBackground);
}

void WifiSettingsApp::startConnect(const char* ssid, const char* pass,
                                    bool saveOnSuccess) {
  strncpy(_pendingSsid, ssid, sizeof(_pendingSsid) - 1);
  _pendingSsid[sizeof(_pendingSsid) - 1] = 0;
  strncpy(_pendingPass, pass, sizeof(_pendingPass) - 1);
  _pendingPass[sizeof(_pendingPass) - 1] = 0;
  _saveOnConnectSuccess = saveOnSuccess;

  WiFi.begin(ssid, (pass && pass[0]) ? pass : nullptr);
  _connectStartMs = millis();
  _state = State::Connecting;
  M5.Display.fillScreen(Theme::kBackground);
}

void WifiSettingsApp::showMessage(const char* msg) {
  strncpy(_message, msg, sizeof(_message) - 1);
  _message[sizeof(_message) - 1] = 0;
  _state = State::Message;

  const int screenW = M5.Display.width();
  _okBtn.initButtonUL(&M5.Display, screenW / 2 - 90, kTop + 200, 180, 64,
                       Theme::kOutline, Theme::kSuccess, Theme::kTextPrimary, "OK", 2.0f,
                       2.0f);
  M5.Display.fillScreen(Theme::kBackground);
}

void WifiSettingsApp::updateMain() {
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
      const auto& net = _store.at(i);
      startConnect(net.ssid, net.pass, false);
      return;
    }

    bool insideForget = pressed && _forgetButtons[i].contains(tx, ty);
    _forgetButtons[i].press(insideForget);
    if (_forgetButtons[i].justReleased()) {
      _store.remove(i);
      layoutMain();
      M5.Display.fillScreen(Theme::kBackground);
      return;
    }
  }

  bool insideScan = pressed && _scanBtn.contains(tx, ty);
  _scanBtn.press(insideScan);
  if (_scanBtn.justReleased()) {
    startScan();
    return;
  }
}

void WifiSettingsApp::updateScanResults() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  for (int i = 0; i < _scanCount; ++i) {
    bool inside = pressed && _rowButtons[i].contains(tx, ty);
    _rowButtons[i].press(inside);
    if (_rowButtons[i].justReleased()) {
      const char* ssid = _scanResults[i].ssid;
      if (_scanResults[i].secure) {
        char title[64];
        snprintf(title, sizeof(title), "Password for %s", ssid);
        strncpy(_pendingSsid, ssid, sizeof(_pendingSsid) - 1);
        _pendingSsid[sizeof(_pendingSsid) - 1] = 0;
        _keyboard.begin(&M5.Display, title, "", WifiStore::kPassLen - 1,
                         /*maskInput=*/true);
        _state = State::PasswordEntry;
        M5.Display.fillScreen(Theme::kBackground);
      } else {
        startConnect(ssid, "", true);
      }
      return;
    }
  }

  bool insideBack = pressed && _backBtn.contains(tx, ty);
  _backBtn.press(insideBack);
  if (_backBtn.justReleased()) {
    _state = State::Main;
    layoutMain();
    M5.Display.fillScreen(Theme::kBackground);
    return;
  }
}

void WifiSettingsApp::updateConnecting() {
  wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    if (_saveOnConnectSuccess) {
      _store.addOrUpdate(_pendingSsid, _pendingPass);
    }
    syncRtcFromNtp();
    char msg[64];
    snprintf(msg, sizeof(msg), "Connected to %s", _pendingSsid);
    showMessage(msg);
    return;
  }
  if (millis() - _connectStartMs > kConnectTimeoutMs) {
    WiFi.disconnect();
    showMessage("Connection failed");
    return;
  }
}

void WifiSettingsApp::updateMessage() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  bool inside = pressed && _okBtn.contains(tx, ty);
  _okBtn.press(inside);
  if (_okBtn.justReleased()) {
    _state = State::Main;
    layoutMain();
    M5.Display.fillScreen(Theme::kBackground);
  }
}

void WifiSettingsApp::update() {
  switch (_state) {
    case State::Main:
      updateMain();
      break;

    case State::Scanning: {
      int16_t n = WiFi.scanComplete();
      if (n >= 0) {
        _scanCount = n > kMaxRows ? kMaxRows : n;
        for (int i = 0; i < _scanCount; ++i) {
          strncpy(_scanResults[i].ssid, WiFi.SSID(i).c_str(),
                  sizeof(_scanResults[i].ssid) - 1);
          _scanResults[i].ssid[sizeof(_scanResults[i].ssid) - 1] = 0;
          _scanResults[i].rssi = WiFi.RSSI(i);
          _scanResults[i].secure = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
        }
        WiFi.scanDelete();
        layoutScanResults();
        _state = State::ScanResults;
        M5.Display.fillScreen(Theme::kBackground);
      } else if (n == WIFI_SCAN_FAILED) {
        showMessage("Scan failed");
      }
      break;
    }

    case State::ScanResults:
      updateScanResults();
      break;

    case State::PasswordEntry:
      _keyboard.update();
      if (_keyboard.isDone()) {
        startConnect(_pendingSsid, _keyboard.text(), true);
      } else if (_keyboard.isCancelled()) {
        _state = State::ScanResults;
        layoutScanResults();
        M5.Display.fillScreen(Theme::kBackground);
      }
      break;

    case State::Connecting:
      updateConnecting();
      break;

    case State::Message:
      updateMessage();
      break;
  }
}

void WifiSettingsApp::drawMain() {
  M5.Display.setTextDatum(top_left);
  M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
  M5.Display.setTextSize(2);
  if (WiFi.status() == WL_CONNECTED) {
    char status[64];
    snprintf(status, sizeof(status), "Connected to %s",
             WiFi.SSID().c_str());
    M5.Display.drawString(status, 10, kTop + 10);
  } else {
    M5.Display.drawString("Not connected", 10, kTop + 10);
  }

  for (int i = 0; i < _rowCount; ++i) {
    _rowButtons[i].drawButton(false, _store.at(i).ssid);
    _forgetButtons[i].drawButton();
  }
  _scanBtn.drawButton(false, "Scan for Networks");
}

void WifiSettingsApp::drawScanning() {
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
  M5.Display.setTextSize(3);
  M5.Display.drawString("Scanning...", M5.Display.width() / 2,
                         M5.Display.height() / 2);
}

void WifiSettingsApp::drawScanResults() {
  for (int i = 0; i < _scanCount; ++i) {
    char label[48];
    snprintf(label, sizeof(label), "%s%s", _scanResults[i].ssid,
             _scanResults[i].secure ? "" : "  (open)");
    _rowButtons[i].drawButton(false, label);
  }
  _backBtn.drawButton();
}

void WifiSettingsApp::drawConnecting() {
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
  M5.Display.setTextSize(3);
  char msg[64];
  snprintf(msg, sizeof(msg), "Connecting to %s...", _pendingSsid);
  M5.Display.drawString(msg, M5.Display.width() / 2, M5.Display.height() / 2);
}

void WifiSettingsApp::drawMessage() {
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
  M5.Display.setTextSize(3);
  M5.Display.drawString(_message, M5.Display.width() / 2, kTop + 100);
  _okBtn.drawButton();
}

void WifiSettingsApp::draw() {
  switch (_state) {
    case State::Main:
      drawMain();
      break;
    case State::Scanning:
      drawScanning();
      break;
    case State::ScanResults:
      drawScanResults();
      break;
    case State::PasswordEntry:
      _keyboard.draw();
      break;
    case State::Connecting:
      drawConnecting();
      break;
    case State::Message:
      drawMessage();
      break;
  }
}
