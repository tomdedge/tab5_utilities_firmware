#pragma once

#include <Arduino.h>

struct WeatherResult {
  double temperature = 0;  // Fahrenheit
  double windSpeed = 0;    // mph
  int weatherCode = -1;    // WMO weather code
  bool isDay = true;
  char description[32] = "";
};

// Talks to Open-Meteo (free, HTTPS, no API key) for city -> lat/long
// geocoding and current-conditions lookup.
class WeatherClient {
 public:
  bool geocode(const char* city, double& lat, double& lon,
               char* resolvedName, size_t resolvedLen);
  bool fetchCurrent(double lat, double lon, WeatherResult& out);

  // HTTP status code from the last request (e.g. 200, 404), or a negative
  // HTTPClient transport error (DNS/TLS/connect failure), or kNoResults if
  // the request succeeded but returned no matches, or kParseError if the
  // response body couldn't be parsed as JSON. Also logged to Serial.
  static constexpr int kNoResults = -2;
  static constexpr int kParseError = -3;
  int lastHttpCode() const { return _lastHttpCode; }

 private:
  static void weatherCodeToText(int code, char* out, size_t outLen);
  int _lastHttpCode = 0;
};
