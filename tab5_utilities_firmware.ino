#include <M5Unified.h>
#include <SD_MMC.h>
#include <WiFi.h>
#include <WiFiMulti.h>

#include "AlarmClockApp.h"
#include "AlarmStore.h"
#include "App.h"
#include "AppManager.h"
#include "CalculatorApp.h"
#include "FormulasApp.h"
#include "MenuApp.h"
#include "NewsApp.h"
#include "NotepadApp.h"
#include "SettingsApp.h"
#include "StopwatchApp.h"
#include "TimeSync.h"
#include "TodoApp.h"
#include "UnitConverterApp.h"
#include "VoiceRecorderApp.h"
#include "WeatherApp.h"
#include "WifiStore.h"
#include "WordLookupApp.h"

// Registry order: index 0 must be the menu. Settings combines Wi-Fi and
// Time Zone into one tile (see SettingsApp.h).
enum AppIndex {
  kAppMenu = 0,
  kAppSettings = 1,
  kAppCalculator = 2,
  kAppWeather = 3,
  kAppNotepad = 4,
  kAppVoiceRecorder = 5,
  kAppStopwatch = 6,
  kAppTodo = 7,
  kAppUnitConverter = 8,
  kAppDictionary = 9,
  kAppNews = 10,
  kAppAlarmClock = 11,
  kAppFormulas = 12,
  kAppCount = 13,
};

MenuApp menuApp;
CalculatorApp calculatorApp;
SettingsApp settingsApp;
WeatherApp weatherApp;
NotepadApp notepadApp;
VoiceRecorderApp voiceRecorderApp;
StopwatchApp stopwatchApp;
TodoApp todoApp;
UnitConverterApp unitConverterApp;
WordLookupApp dictionaryApp;
NewsApp newsApp;
AlarmClockApp alarmClockApp;
FormulasApp formulasApp;

App* apps[kAppCount] = {
    &menuApp,          &settingsApp,   &calculatorApp,     &weatherApp,
    &notepadApp,       &voiceRecorderApp, &stopwatchApp,   &todoApp,
    &unitConverterApp, &dictionaryApp, &newsApp,           &alarmClockApp,
    &formulasApp,
};

AppManager appManager;
AlarmStore alarmStore;

void setup() {
  Serial.begin(115200);

  auto cfg = M5.config();
  cfg.internal_mic = true;
  cfg.internal_spk = true;
  cfg.internal_rtc = true;
  M5.begin(cfg);

  // Tab5's panel is natively portrait (720 wide x 1280 tall); rotate to
  // landscape for a tablet-style UI. If this comes up mirrored/upside down
  // on the real hardware, try setRotation(3) instead.
  M5.Display.setRotation(1);

  // The Tab5's ESP32-P4 has no native Wi-Fi; it talks to the onboard
  // ESP32-C6 co-processor over SDIO, which needs this pin bring-up before
  // any WiFi.* call. (Pin set confirmed via M5Unified's Rtc.ino example.)
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  if (M5.getBoard() == m5::board_t::board_M5Tab5) {
    WiFi.setPins(GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_11, GPIO_NUM_10,
                 GPIO_NUM_9, GPIO_NUM_8, GPIO_NUM_15);
  }
#endif

  // Tab5's SD slot is 4-bit SDIO, not SPI -- mount via SD_MMC using the
  // board's own pin table rather than hardcoding GPIO numbers here.
  SD_MMC.setPins(M5.getPin(m5::pin_name_t::sd_mmc_clk),
                 M5.getPin(m5::pin_name_t::sd_mmc_cmd),
                 M5.getPin(m5::pin_name_t::sd_mmc_d0),
                 M5.getPin(m5::pin_name_t::sd_mmc_d1),
                 M5.getPin(m5::pin_name_t::sd_mmc_d2),
                 M5.getPin(m5::pin_name_t::sd_mmc_d3));
  SD_MMC.begin("/sdcard", /*mode1bit=*/false);

  // Quick best-effort auto-reconnect to a remembered network at boot. If
  // this doesn't succeed within 5s (none saved, out of range, etc.) we just
  // continue on -- Settings' Wi-Fi section can always scan/connect later.
  WifiStore bootStore;
  bootStore.load();
  Serial.printf("[Boot] %d saved Wi-Fi network(s)\n", bootStore.count());
  if (bootStore.count() > 0) {
    WiFiMulti wifiMulti;
    bootStore.applyToWiFiMulti(wifiMulti);
    wifiMulti.run(5000);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[Boot] Wi-Fi connected: %s, IP %s\n",
                   WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  } else {
    Serial.println("[Boot] Wi-Fi not connected at boot");
  }

  // The RTC otherwise never gets set to anything real -- sync it from NTP
  // now if we made it onto Wi-Fi above (Settings' Wi-Fi section also syncs
  // it after a manual connect, for when boot didn't have a network yet).
  Serial.printf("[Boot] NTP sync %s\n",
                syncRtcFromNtp() ? "succeeded" : "skipped/failed");

  alarmStore.load();

  menuApp.begin(&appManager, apps, kAppCount);
  weatherApp.begin(&appManager, kAppSettings, &settingsApp);
  alarmClockApp.begin(&alarmStore);
  appManager.begin(apps, kAppCount);
}

void loop() {
  M5.update();

  // Checked every frame regardless of which app is active -- an alarm has
  // to be able to interrupt whatever's on screen, not just fire while the
  // Alarm Clock app itself happens to be open.
  int firedIdx = alarmStore.checkDue();
  if (firedIdx >= 0) {
    appManager.ringAlarm(alarmStore.at(firedIdx).hour, alarmStore.at(firedIdx).minute);
  }

  appManager.update();
  appManager.draw();
  delay(5);
}
