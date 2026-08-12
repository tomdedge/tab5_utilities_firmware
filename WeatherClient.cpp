#include "WeatherClient.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <cctype>
#include <cstdio>

namespace {

String urlEncode(const char* s) {
  String out;
  char buf[4];
  while (*s) {
    unsigned char c = (unsigned char)*s++;
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += (char)c;
    } else if (c == ' ') {
      out += '+';
    } else {
      snprintf(buf, sizeof(buf), "%%%02X", c);
      out += buf;
    }
  }
  return out;
}

// Skips TLS certificate validation. Reasonable for a hobby weather widget
// hitting a fixed, trusted public API -- not appropriate for anything
// security-sensitive.
//
// Returns the HTTP status code (e.g. 200), or a negative HTTPClient
// transport-error code if the request never got a response at all (DNS
// failure, TLS handshake failure, connection refused, timeout, etc. --
// see HTTPClient.h's HTTPC_ERROR_* constants). doc is only filled in when
// the code is HTTP_CODE_OK.
int httpGetJson(const String& url, JsonDocument& doc) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, url)) {
    Serial.printf("[Weather] http.begin() failed for %s\n", url.c_str());
    return HTTPC_ERROR_CONNECTION_REFUSED;
  }

  int code = http.GET();
  Serial.printf("[Weather] GET %s -> %d\n", url.c_str(), code);

  if (code == HTTP_CODE_OK) {
    // Read the whole body first rather than parsing straight off the
    // HTTPClient stream. Open-Meteo serves chunked responses, and
    // deserializeJson() reading a live Stream can bail out early if the
    // underlying socket delivers bytes in bursts with small gaps between
    // them -- producing spurious parse failures on JSON that's actually
    // complete and valid.
    String body = http.getString();
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
      Serial.printf("[Weather] JSON parse error: %s (%u bytes)\n", err.c_str(),
                     (unsigned)body.length());
      http.end();
      return WeatherClient::kParseError;
    }
  }
  http.end();
  return code;
}

}  // namespace

bool WeatherClient::geocode(const char* city, double& lat, double& lon,
                             char* resolvedName, size_t resolvedLen) {
  String url =
      "https://geocoding-api.open-meteo.com/v1/search?count=1&name=" +
      urlEncode(city);

  JsonDocument doc;
  _lastHttpCode = httpGetJson(url, doc);
  if (_lastHttpCode != HTTP_CODE_OK) return false;

  JsonArray results = doc["results"].as<JsonArray>();
  if (results.isNull() || results.size() == 0) {
    _lastHttpCode = kNoResults;
    return false;
  }

  JsonObject r = results[0];
  lat = r["latitude"] | 0.0;
  lon = r["longitude"] | 0.0;
  const char* name = r["name"] | city;
  const char* country = r["country"] | "";
  snprintf(resolvedName, resolvedLen, "%s, %s", name, country);
  return true;
}

void WeatherClient::weatherCodeToText(int code, char* out, size_t outLen) {
  const char* text = "Unknown";
  switch (code) {
    case 0:
      text = "Clear sky";
      break;
    case 1:
      text = "Mainly clear";
      break;
    case 2:
      text = "Partly cloudy";
      break;
    case 3:
      text = "Overcast";
      break;
    case 45:
    case 48:
      text = "Fog";
      break;
    case 51:
    case 53:
    case 55:
      text = "Drizzle";
      break;
    case 56:
    case 57:
      text = "Freezing drizzle";
      break;
    case 61:
    case 63:
    case 65:
      text = "Rain";
      break;
    case 66:
    case 67:
      text = "Freezing rain";
      break;
    case 71:
    case 73:
    case 75:
      text = "Snow";
      break;
    case 77:
      text = "Snow grains";
      break;
    case 80:
    case 81:
    case 82:
      text = "Rain showers";
      break;
    case 85:
    case 86:
      text = "Snow showers";
      break;
    case 95:
      text = "Thunderstorm";
      break;
    case 96:
    case 99:
      text = "Thunderstorm, hail";
      break;
    default:
      break;
  }
  snprintf(out, outLen, "%s", text);
}

bool WeatherClient::fetchCurrent(double lat, double lon, WeatherResult& out) {
  char url[192];
  snprintf(
      url, sizeof(url),
      "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
      "&current_weather=true&temperature_unit=fahrenheit&wind_speed_unit=mph",
      lat, lon);

  JsonDocument doc;
  _lastHttpCode = httpGetJson(String(url), doc);
  if (_lastHttpCode != HTTP_CODE_OK) return false;

  JsonObject cw = doc["current_weather"];
  if (cw.isNull()) {
    _lastHttpCode = kNoResults;
    return false;
  }

  out.temperature = cw["temperature"] | 0.0;
  out.windSpeed = cw["windspeed"] | 0.0;
  out.weatherCode = cw["weathercode"] | -1;
  out.isDay = (cw["is_day"] | 1) != 0;
  weatherCodeToText(out.weatherCode, out.description, sizeof(out.description));
  return true;
}
