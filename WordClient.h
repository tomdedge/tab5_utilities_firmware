#pragma once

#include <Arduino.h>

// Talks to two free, keyless HTTPS APIs for word lookups:
// dictionaryapi.dev (definitions) and Datamuse (synonyms). Both write a
// formatted, display-ready string into the caller's buffer rather than
// returning structured data, since the only consumer (WordLookupApp) just
// shows it via drawWrappedText().
class WordClient {
 public:
  bool lookupDefinitions(const char* word, char* out, size_t outLen);
  bool lookupSynonyms(const char* word, char* out, size_t outLen);

  // HTTP status code from the last request, or a negative HTTPClient
  // transport error, or kNoResults if the word wasn't found / had no
  // synonyms. Also logged to Serial.
  static constexpr int kNoResults = -2;
  int lastHttpCode() const { return _lastHttpCode; }

 private:
  int _lastHttpCode = 0;
};
