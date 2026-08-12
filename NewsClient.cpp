#include "NewsClient.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <cctype>
#include <cstdio>
#include <cstring>

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

// Reads the whole body before parsing -- see CLAUDE.md's note on chunked
// responses tripping up deserializeJson() reading a live Stream.
int httpGetJson(const String& url, JsonDocument& doc) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, url)) {
    Serial.printf("[News] http.begin() failed for %s\n", url.c_str());
    return HTTPC_ERROR_CONNECTION_REFUSED;
  }

  int code = http.GET();
  Serial.printf("[News] GET %s -> %d\n", url.c_str(), code);

  if (code == HTTP_CODE_OK) {
    String body = http.getString();
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
      Serial.printf("[News] JSON parse error: %s (%u bytes)\n", err.c_str(),
                     (unsigned)body.length());
      http.end();
      return -3;
    }
  }
  http.end();
  return code;
}

// rss2json descriptions often carry raw HTML (<p>, <img>, ...); strip tags
// for plain-text display rather than showing markup literally.
void stripHtmlInto(const char* html, char* out, size_t outLen) {
  size_t o = 0;
  bool inTag = false;
  for (const char* p = html; *p && o < outLen - 1; ++p) {
    if (*p == '<') {
      inTag = true;
      continue;
    }
    if (*p == '>') {
      inTag = false;
      continue;
    }
    if (!inTag) out[o++] = *p;
  }
  out[o] = 0;
}

}  // namespace

bool NewsClient::fetch(const char* feedUrl, char* feedTitleOut,
                        size_t feedTitleLen) {
  String url =
      "https://api.rss2json.com/v1/api.json?rss_url=" + urlEncode(feedUrl);

  JsonDocument doc;
  _lastHttpCode = httpGetJson(url, doc);
  if (_lastHttpCode != HTTP_CODE_OK) return false;

  const char* status = doc["status"] | "";
  if (strcmp(status, "ok") != 0) {
    _lastHttpCode = kNoResults;
    return false;
  }

  if (feedTitleOut) {
    const char* title = doc["feed"]["title"] | "";
    snprintf(feedTitleOut, feedTitleLen, "%s", title);
  }

  JsonArray items = doc["items"].as<JsonArray>();
  _count = 0;
  for (JsonObject it : items) {
    if (_count >= kMaxItems) break;
    const char* title = it["title"] | "";
    const char* desc = it["description"] | "";
    const char* pubDate = it["pubDate"] | "";

    strncpy(_items[_count].title, title, sizeof(_items[_count].title) - 1);
    _items[_count].title[sizeof(_items[_count].title) - 1] = 0;
    stripHtmlInto(desc, _items[_count].description,
                  sizeof(_items[_count].description));
    strncpy(_items[_count].pubDate, pubDate,
            sizeof(_items[_count].pubDate) - 1);
    _items[_count].pubDate[sizeof(_items[_count].pubDate) - 1] = 0;
    _count++;
  }
  return true;
}
