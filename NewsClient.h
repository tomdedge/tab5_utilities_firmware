#pragma once

#include <Arduino.h>

struct NewsItem {
  char title[120];
  char description[400];
  char pubDate[24];
};

// Fetches an RSS feed via rss2json.com (free tier, no key -- converts RSS
// to JSON so this can reuse ArduinoJson instead of an XML parser).
class NewsClient {
 public:
  static constexpr int kMaxItems = 8;

  // feedTitleOut may be null if the caller doesn't need it.
  bool fetch(const char* feedUrl, char* feedTitleOut, size_t feedTitleLen);
  int itemCount() const { return _count; }
  const NewsItem& item(int index) const { return _items[index]; }

  static constexpr int kNoResults = -2;
  int lastHttpCode() const { return _lastHttpCode; }

 private:
  NewsItem _items[kMaxItems];
  int _count = 0;
  int _lastHttpCode = 0;
};
