#pragma once

#include <M5GFX.h>
#include <cstdint>

#include "App.h"
#include "Keyboard.h"
#include "WifiStore.h"

// Scan for nearby networks, tap one to enter its password (via the shared
// Keyboard), and remember it (via WifiStore) so future boots auto-reconnect.
// Saved networks can also be forgotten from the main list.
class WifiSettingsApp : public App {
 public:
  void onEnter() override;
  void update() override;
  void draw() override;
  const char* name() const override { return "Wi-Fi Settings"; }

 private:
  enum class State {
    Main,
    Scanning,
    ScanResults,
    PasswordEntry,
    Connecting,
    Message,
  };

  static constexpr int kMaxRows = 8;
  static constexpr uint32_t kConnectTimeoutMs = 15000;

  struct ScanEntry {
    char ssid[WifiStore::kSsidLen];
    int32_t rssi;
    bool secure;
  };

  void layoutMain();
  void layoutScanResults();

  void updateMain();
  void updateScanResults();
  void updateConnecting();
  void updateMessage();

  void drawMain();
  void drawScanning();
  void drawScanResults();
  void drawConnecting();
  void drawMessage();

  void startScan();
  void startConnect(const char* ssid, const char* pass, bool saveOnSuccess);
  void showMessage(const char* msg);

  WifiStore _store;
  State _state = State::Main;

  LGFX_Button _rowButtons[kMaxRows];
  LGFX_Button _forgetButtons[kMaxRows];
  int _rowCount = 0;
  LGFX_Button _scanBtn;
  LGFX_Button _backBtn;
  LGFX_Button _okBtn;

  ScanEntry _scanResults[kMaxRows];
  int _scanCount = 0;

  char _pendingSsid[WifiStore::kSsidLen] = {0};
  char _pendingPass[WifiStore::kPassLen] = {0};
  bool _saveOnConnectSuccess = false;

  Keyboard _keyboard;

  char _message[64] = {0};

  uint32_t _connectStartMs = 0;
};
