#include "WifiStore.h"

#include <Preferences.h>
#include <WiFiMulti.h>

#include <cstdio>
#include <cstring>

namespace {
const char* kNamespace = "wifi";
}

void WifiStore::load() {
  Preferences prefs;
  prefs.begin(kNamespace, /*readOnly=*/true);
  _count = prefs.getInt("count", 0);
  if (_count < 0) _count = 0;
  if (_count > kMaxNetworks) _count = kMaxNetworks;

  char key[16];
  for (int i = 0; i < _count; ++i) {
    snprintf(key, sizeof(key), "ssid%d", i);
    prefs.getString(key, _networks[i].ssid, kSsidLen);
    snprintf(key, sizeof(key), "pass%d", i);
    prefs.getString(key, _networks[i].pass, kPassLen);
  }
  prefs.end();
}

void WifiStore::persist() {
  Preferences prefs;
  prefs.begin(kNamespace, /*readOnly=*/false);
  prefs.putInt("count", _count);

  char key[16];
  for (int i = 0; i < _count; ++i) {
    snprintf(key, sizeof(key), "ssid%d", i);
    prefs.putString(key, _networks[i].ssid);
    snprintf(key, sizeof(key), "pass%d", i);
    prefs.putString(key, _networks[i].pass);
  }
  prefs.end();
}

bool WifiStore::addOrUpdate(const char* ssid, const char* pass) {
  for (int i = 0; i < _count; ++i) {
    if (strcmp(_networks[i].ssid, ssid) == 0) {
      strncpy(_networks[i].pass, pass, kPassLen - 1);
      _networks[i].pass[kPassLen - 1] = 0;
      persist();
      return true;
    }
  }
  if (_count >= kMaxNetworks) return false;

  strncpy(_networks[_count].ssid, ssid, kSsidLen - 1);
  _networks[_count].ssid[kSsidLen - 1] = 0;
  strncpy(_networks[_count].pass, pass, kPassLen - 1);
  _networks[_count].pass[kPassLen - 1] = 0;
  _count++;
  persist();
  return true;
}

bool WifiStore::remove(int index) {
  if (index < 0 || index >= _count) return false;
  for (int i = index; i < _count - 1; ++i) {
    _networks[i] = _networks[i + 1];
  }
  _count--;
  persist();
  return true;
}

void WifiStore::applyToWiFiMulti(WiFiMulti& multi) const {
  for (int i = 0; i < _count; ++i) {
    multi.addAP(_networks[i].ssid, _networks[i].pass);
  }
}
