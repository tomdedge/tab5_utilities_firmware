#include "WordClient.h"

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
    Serial.printf("[Word] http.begin() failed for %s\n", url.c_str());
    return HTTPC_ERROR_CONNECTION_REFUSED;
  }

  int code = http.GET();
  Serial.printf("[Word] GET %s -> %d\n", url.c_str(), code);

  if (code == HTTP_CODE_OK) {
    String body = http.getString();
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
      Serial.printf("[Word] JSON parse error: %s (%u bytes)\n", err.c_str(),
                     (unsigned)body.length());
      http.end();
      return -3;
    }
  }
  http.end();
  return code;
}

}  // namespace

bool WordClient::lookupDefinitions(const char* word, char* out,
                                    size_t outLen) {
  String url = "https://api.dictionaryapi.dev/api/v2/entries/en/" +
               urlEncode(word);

  JsonDocument doc;
  _lastHttpCode = httpGetJson(url, doc);
  if (_lastHttpCode != HTTP_CODE_OK) {
    if (_lastHttpCode == 404) _lastHttpCode = kNoResults;
    return false;
  }

  JsonArray entries = doc.as<JsonArray>();
  if (entries.isNull() || entries.size() == 0) {
    _lastHttpCode = kNoResults;
    return false;
  }

  out[0] = 0;
  size_t pos = 0;
  JsonObject entry = entries[0];
  JsonArray meanings = entry["meanings"].as<JsonArray>();
  for (JsonObject meaning : meanings) {
    const char* partOfSpeech = meaning["partOfSpeech"] | "";
    JsonArray defs = meaning["definitions"].as<JsonArray>();
    int shown = 0;
    for (JsonObject def : defs) {
      if (shown >= 2) break;
      const char* text = def["definition"] | "";
      char line[300];
      snprintf(line, sizeof(line), "%s: %s\n\n", partOfSpeech, text);
      size_t lineLen = strlen(line);
      if (pos + lineLen >= outLen - 1) break;
      strcpy(out + pos, line);
      pos += lineLen;
      shown++;
    }
  }

  if (pos == 0) {
    strncpy(out, "No definitions found.", outLen - 1);
    out[outLen - 1] = 0;
  }
  return true;
}

bool WordClient::lookupSynonyms(const char* word, char* out, size_t outLen) {
  String url =
      "https://api.datamuse.com/words?rel_syn=" + urlEncode(word) + "&max=12";

  JsonDocument doc;
  _lastHttpCode = httpGetJson(url, doc);
  if (_lastHttpCode != HTTP_CODE_OK) return false;

  JsonArray results = doc.as<JsonArray>();
  if (results.isNull() || results.size() == 0) {
    _lastHttpCode = kNoResults;
    return false;
  }

  out[0] = 0;
  size_t pos = 0;
  bool first = true;
  for (JsonObject r : results) {
    const char* w = r["word"] | "";
    size_t wlen = strlen(w);
    size_t addLen = wlen + (first ? 0 : 2);
    if (pos + addLen >= outLen - 1) break;
    if (!first) {
      strcpy(out + pos, ", ");
      pos += 2;
    }
    strcpy(out + pos, w);
    pos += wlen;
    first = false;
  }
  return true;
}
