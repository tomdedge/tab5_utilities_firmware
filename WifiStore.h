#pragma once

class WiFiMulti;

// Persists a small list of remembered Wi-Fi networks (SSID + password) in
// NVS via Preferences, and can feed them into a WiFiMulti instance so the
// device auto-connects to whichever remembered network is strongest.
class WifiStore {
 public:
  static constexpr int kMaxNetworks = 8;
  static constexpr int kSsidLen = 33;  // 32 chars + null
  static constexpr int kPassLen = 65;  // 64 chars + null

  struct Network {
    char ssid[kSsidLen];
    char pass[kPassLen];
  };

  void load();
  int count() const { return _count; }
  const Network& at(int index) const { return _networks[index]; }

  // Adds a new saved network, or updates the password if the ssid is
  // already saved. Returns false only if the store is full and ssid is new.
  bool addOrUpdate(const char* ssid, const char* pass);
  bool remove(int index);

  void applyToWiFiMulti(WiFiMulti& multi) const;

 private:
  void persist();

  Network _networks[kMaxNetworks] = {};
  int _count = 0;
};
