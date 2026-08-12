#include "NewsApp.h"

#include "Theme.h"

#include <M5Unified.h>
#include <Preferences.h>

#include <cstdio>
#include <cstring>

#include "AppManager.h"
#include "TextBox.h"

namespace {
const int kTop = AppManager::kStatusBarHeight;
const char* kPrefsNamespace = "news";

struct FeedPreset {
  const char* name;
  const char* url;
};
// All three verified live and parsing correctly through rss2json.com.
const FeedPreset kPresets[NewsApp::kPresetCount] = {
    {"BBC News", "https://feeds.bbci.co.uk/news/rss.xml"},
    {"NPR News", "https://feeds.npr.org/1001/rss.xml"},
    {"The Guardian: World", "https://www.theguardian.com/world/rss"},
};
}  // namespace

void NewsApp::loadSavedFeed() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, /*readOnly=*/true);
  prefs.getString("feedurl", _feedUrl, sizeof(_feedUrl));
  prefs.getString("feedname", _feedTitle, sizeof(_feedTitle));
  prefs.end();
  if (strlen(_feedTitle) == 0) strcpy(_feedTitle, "News");
}

void NewsApp::saveFeed() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, /*readOnly=*/false);
  prefs.putString("feedurl", _feedUrl);
  prefs.putString("feedname", _feedTitle);
  prefs.end();
}

void NewsApp::layoutPicker() {
  const int screenW = M5.Display.width();
  const int top = kTop + 60;
  const int rowH = 64;
  const int padding = 10;

  if (_hasFeed) {
    _cancelPickerBtn.initButtonUL(&M5.Display, screenW - 220 - padding,
                                   kTop + 10, 220, 44, Theme::kOutline,
                                   Theme::kDanger, Theme::kTextPrimary, "",
                                   2.0f, 2.0f);
  }

  for (int i = 0; i < kPresetCount; ++i) {
    int y = top + i * (rowH + padding);
    _presetButtons[i].initButtonUL(&M5.Display, padding, y,
                                    screenW - padding * 2, rowH, Theme::kOutline,
                                    Theme::kPrimary, Theme::kTextPrimary, "", 2.2f, 2.2f);
  }
  int customY = top + kPresetCount * (rowH + padding) + 10;
  _customBtn.initButtonUL(&M5.Display, padding, customY, screenW - padding * 2,
                           rowH, Theme::kOutline, Theme::kSuccess, Theme::kTextPrimary,
                           "Custom URL...", 2.2f, 2.2f);
}

void NewsApp::layoutList() {
  const int screenW = M5.Display.width();
  const int padding = 8;
  const int btnY = kTop + 10;
  const int btnH = 44;

  _backBtn.initButtonUL(&M5.Display, padding, btnY, 150, btnH, Theme::kOutline,
                         Theme::kDanger, Theme::kTextPrimary, "", 2.0f, 2.0f);
  _refreshBtn.initButtonUL(&M5.Display, screenW - 150 - padding, btnY, 150,
                            btnH, Theme::kOutline, Theme::kSuccess, Theme::kTextPrimary,
                            "Refresh", 2.0f, 2.0f);

  const int listTop = btnY + btnH + 10;
  const int rowH = 56;
  int n = _client.itemCount();
  for (int i = 0; i < n; ++i) {
    int y = listTop + i * (rowH + padding);
    _headlineButtons[i].initButtonUL(&M5.Display, padding, y,
                                      screenW - padding * 2, rowH,
                                      Theme::kOutline, Theme::kPrimary, Theme::kTextPrimary, "",
                                      2.0f, 2.0f);
  }
}

void NewsApp::startFetch(const char* url) {
  strncpy(_feedUrl, url, sizeof(_feedUrl) - 1);
  _feedUrl[sizeof(_feedUrl) - 1] = 0;
  _state = State::Loading;
  M5.Display.fillScreen(Theme::kBackground);
}

void NewsApp::onEnter() {
  loadSavedFeed();
  layoutPicker();
  if (strlen(_feedUrl) > 0) {
    startFetch(_feedUrl);
  } else {
    _state = State::Picker;
    M5.Display.fillScreen(Theme::kBackground);
  }
}

void NewsApp::doFetch() {
  char fetchedTitle[64] = {0};
  bool ok = _client.fetch(_feedUrl, fetchedTitle, sizeof(fetchedTitle));
  layoutList();  // positions Back/Refresh regardless of outcome

  if (ok) {
    if (strlen(fetchedTitle) > 0) {
      strncpy(_feedTitle, fetchedTitle, sizeof(_feedTitle) - 1);
      _feedTitle[sizeof(_feedTitle) - 1] = 0;
    }
    saveFeed();
    _hasFeed = true;
    _state = State::List;
  } else {
    int code = _client.lastHttpCode();
    if (code == NewsClient::kNoResults) {
      strcpy(_errorMsg, "Feed not found or empty");
    } else {
      snprintf(_errorMsg, sizeof(_errorMsg), "Network error (code %d)", code);
    }
    _state = State::Error;
  }
  M5.Display.fillScreen(Theme::kBackground);
}

void NewsApp::updatePicker() {
  if (_enteringCustomUrl) {
    _keyboard.update();
    if (_keyboard.isDone()) {
      const char* url = _keyboard.text();
      if (strlen(url) > 0) {
        strcpy(_feedTitle, "Custom Feed");
        _enteringCustomUrl = false;
        startFetch(url);
      }
    } else if (_keyboard.isCancelled()) {
      _enteringCustomUrl = false;
      layoutPicker();
      M5.Display.fillScreen(Theme::kBackground);
    }
    return;
  }

  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  if (_hasFeed) {
    bool insideCancel = pressed && _cancelPickerBtn.contains(tx, ty);
    _cancelPickerBtn.press(insideCancel);
    if (_cancelPickerBtn.justReleased()) {
      layoutList();
      _state = State::List;
      M5.Display.fillScreen(Theme::kBackground);
      return;
    }
  }

  for (int i = 0; i < kPresetCount; ++i) {
    bool inside = pressed && _presetButtons[i].contains(tx, ty);
    _presetButtons[i].press(inside);
    if (_presetButtons[i].justReleased()) {
      strncpy(_feedTitle, kPresets[i].name, sizeof(_feedTitle) - 1);
      _feedTitle[sizeof(_feedTitle) - 1] = 0;
      startFetch(kPresets[i].url);
      return;
    }
  }

  bool insideCustom = pressed && _customBtn.contains(tx, ty);
  _customBtn.press(insideCustom);
  if (_customBtn.justReleased()) {
    _keyboard.begin(&M5.Display, "RSS Feed URL", "https://", 150,
                     /*maskInput=*/false);
    _enteringCustomUrl = true;
    M5.Display.fillScreen(Theme::kBackground);
  }
}

void NewsApp::updateList() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  int n = _client.itemCount();
  for (int i = 0; i < n; ++i) {
    bool inside = pressed && _headlineButtons[i].contains(tx, ty);
    _headlineButtons[i].press(inside);
    if (_headlineButtons[i].justReleased()) {
      _selectedItem = i;
      _state = State::Detail;
      M5.Display.fillScreen(Theme::kBackground);
      return;
    }
  }

  bool insideBack = pressed && _backBtn.contains(tx, ty);
  _backBtn.press(insideBack);
  if (_backBtn.justReleased()) {
    layoutPicker();
    _state = State::Picker;
    M5.Display.fillScreen(Theme::kBackground);
    return;
  }

  bool insideRefresh = pressed && _refreshBtn.contains(tx, ty);
  _refreshBtn.press(insideRefresh);
  if (_refreshBtn.justReleased()) {
    startFetch(_feedUrl);
  }
}

void NewsApp::updateDetail() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  bool insideBack = pressed && _backBtn.contains(tx, ty);
  _backBtn.press(insideBack);
  if (_backBtn.justReleased()) {
    _state = State::List;
    M5.Display.fillScreen(Theme::kBackground);
  }
}

void NewsApp::updateError() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  bool insideBack = pressed && _backBtn.contains(tx, ty);
  _backBtn.press(insideBack);
  if (_backBtn.justReleased()) {
    layoutPicker();
    _state = State::Picker;
    M5.Display.fillScreen(Theme::kBackground);
  }
}

void NewsApp::update() {
  switch (_state) {
    case State::Picker:
      updatePicker();
      break;
    case State::Loading:
      doFetch();
      break;
    case State::List:
      updateList();
      break;
    case State::Detail:
      updateDetail();
      break;
    case State::Error:
      updateError();
      break;
  }
}

void NewsApp::drawPicker() {
  if (_enteringCustomUrl) {
    _keyboard.draw();
    return;
  }
  M5.Display.setTextDatum(top_left);
  M5.Display.setTextColor(Theme::kTextSecondary, Theme::kBackground);
  M5.Display.setTextSize(2);
  M5.Display.drawString("Choose a news feed", 10, kTop + 12);

  if (_hasFeed) {
    _cancelPickerBtn.drawButton(false, "Keep Current");
  }

  for (int i = 0; i < kPresetCount; ++i) {
    _presetButtons[i].drawButton(false, kPresets[i].name);
  }
  _customBtn.drawButton();
}

void NewsApp::drawList() {
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(Theme::kTextSecondary, Theme::kBackground);
  M5.Display.setTextSize(2);
  M5.Display.fillRect(160, kTop + 10, M5.Display.width() - 320, 44,
                       Theme::kBackground);
  M5.Display.drawString(_feedTitle, M5.Display.width() / 2, kTop + 32);

  int n = _client.itemCount();
  if (n == 0) {
    M5.Display.setTextDatum(top_left);
    M5.Display.drawString("No headlines", 10, kTop + 70);
  }
  for (int i = 0; i < n; ++i) {
    _headlineButtons[i].drawButton(false, _client.item(i).title);
  }
  _backBtn.drawButton(false, "Change Feed");
  _refreshBtn.drawButton();
}

void NewsApp::drawDetail() {
  const NewsItem& it = _client.item(_selectedItem);
  const int contentTop = kTop + 64;

  M5.Display.fillRect(0, contentTop, M5.Display.width(), 60, Theme::kBackground);
  M5.Display.setTextDatum(top_left);
  M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
  M5.Display.setTextSize(2);
  M5.Display.drawString(it.title, 10, contentTop);

  drawWrappedText(&M5.Display, 10, contentTop + 70, M5.Display.width() - 20,
                   M5.Display.height() - (contentTop + 70) - 10,
                   it.description, 2);

  _backBtn.drawButton(false, "Back");
}

void NewsApp::drawError() {
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
  M5.Display.setTextSize(3);
  M5.Display.drawString(_errorMsg, M5.Display.width() / 2,
                         M5.Display.height() / 2 - 30);
  _backBtn.drawButton(false, "Back");
}

void NewsApp::draw() {
  switch (_state) {
    case State::Picker:
      drawPicker();
      break;
    case State::Loading:
      M5.Display.setTextDatum(middle_center);
      M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
      M5.Display.setTextSize(3);
      M5.Display.drawString("Loading news...", M5.Display.width() / 2,
                             M5.Display.height() / 2);
      break;
    case State::List:
      drawList();
      break;
    case State::Detail:
      drawDetail();
      break;
    case State::Error:
      drawError();
      break;
  }
}
