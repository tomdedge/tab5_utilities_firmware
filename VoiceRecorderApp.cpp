#include "VoiceRecorderApp.h"

#include "Theme.h"

#include <M5Unified.h>
#include <SD_MMC.h>
#include <esp_heap_caps.h>

#include <cstdio>
#include <cstring>

#include "AppManager.h"
#include "WavFile.h"

namespace {
const int kTop = AppManager::kStatusBarHeight;
const char* kRecordingsDir = "/recordings";
}  // namespace

void VoiceRecorderApp::onEnter() {
  if (SD_MMC.cardType() == CARD_NONE) {
    _state = State::NoSd;
    M5.Display.fillScreen(Theme::kBackground);
    return;
  }
  refreshList();
  layoutList();
  _state = State::List;
  M5.Display.fillScreen(Theme::kBackground);
}

void VoiceRecorderApp::onExit() {
  if (_state == State::Recording) {
    stopRecording();
  } else if (_state == State::Playing) {
    M5.Speaker.stop();
    stopPlayback();
  }
}

void VoiceRecorderApp::refreshList() {
  _count = 0;
  if (!SD_MMC.exists(kRecordingsDir)) {
    SD_MMC.mkdir(kRecordingsDir);
    return;
  }

  File dir = SD_MMC.open(kRecordingsDir);
  if (!dir || !dir.isDirectory()) return;

  File f = dir.openNextFile();
  while (f && _count < kMaxRows) {
    if (!f.isDirectory()) {
      const char* n = f.name();
      const char* slash = strrchr(n, '/');
      const char* base = slash ? slash + 1 : n;
      strncpy(_recordings[_count].filename, base,
              sizeof(_recordings[_count].filename) - 1);
      _recordings[_count].filename[sizeof(_recordings[_count].filename) - 1] =
          0;
      _count++;
    }
    f.close();
    f = dir.openNextFile();
  }
  dir.close();

  // Filenames are timestamps, so reversing gives newest-first.
  for (int i = 0; i < _count / 2; ++i) {
    RecordingInfo tmp = _recordings[i];
    _recordings[i] = _recordings[_count - 1 - i];
    _recordings[_count - 1 - i] = tmp;
  }
}

void VoiceRecorderApp::layoutList() {
  const int screenW = M5.Display.width();
  const int screenH = M5.Display.height();
  const int top = kTop + 10;
  const int rowH = 56;
  const int padding = 8;
  const int delW = 100;

  for (int i = 0; i < _count; ++i) {
    int y = top + i * (rowH + padding);
    _rowButtons[i].initButtonUL(&M5.Display, padding, y,
                                 screenW - delW - padding * 3, rowH,
                                 Theme::kOutline, Theme::kPrimary, Theme::kTextPrimary, "", 2.0f,
                                 2.0f);
    _deleteButtons[i].initButtonUL(&M5.Display, screenW - delW - padding, y,
                                    delW, rowH, Theme::kOutline, Theme::kDanger,
                                    Theme::kTextPrimary, "Delete", 1.8f, 1.8f);
  }

  // When the list is empty, reserve one row's worth of height for the
  // "No recordings yet" message so the Record button doesn't land on top
  // of it.
  int effectiveRows = _count > 0 ? _count : 1;
  int recY = top + effectiveRows * (rowH + padding) + 10;
  _recordBtn.initButtonUL(&M5.Display, padding, recY, screenW - padding * 2,
                           rowH, Theme::kOutline, Theme::kDanger, Theme::kTextPrimary, "", 2.2f,
                           2.2f);

  _stopBtn.initButtonUL(&M5.Display, screenW / 2 - 110, screenH - 100, 220,
                         70, Theme::kOutline, Theme::kDanger, Theme::kTextPrimary, "Stop", 2.0f,
                         2.0f);
}

void VoiceRecorderApp::updateList() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }

  for (int i = 0; i < _count; ++i) {
    bool insideRow = pressed && _rowButtons[i].contains(tx, ty);
    _rowButtons[i].press(insideRow);
    if (_rowButtons[i].justReleased()) {
      startPlayback(i);
      return;
    }

    bool insideDel = pressed && _deleteButtons[i].contains(tx, ty);
    _deleteButtons[i].press(insideDel);
    if (_deleteButtons[i].justReleased()) {
      char path[48];
      snprintf(path, sizeof(path), "%s/%s", kRecordingsDir,
               _recordings[i].filename);
      SD_MMC.remove(path);
      refreshList();
      layoutList();
      M5.Display.fillScreen(Theme::kBackground);
      return;
    }
  }

  bool insideRec = pressed && _recordBtn.contains(tx, ty);
  _recordBtn.press(insideRec);
  if (_recordBtn.justReleased()) {
    startRecording();
  }
}

void VoiceRecorderApp::drawList() {
  if (_count == 0) {
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextColor(Theme::kTextSecondary, Theme::kBackground);
    M5.Display.setTextSize(2);
    M5.Display.drawString("No recordings yet", 10, kTop + 10);
  }
  for (int i = 0; i < _count; ++i) {
    _rowButtons[i].drawButton(false, _recordings[i].filename);
    _deleteButtons[i].drawButton();
  }
  _recordBtn.drawButton(false, "Record New");
}

void VoiceRecorderApp::startRecording() {
  if (!SD_MMC.exists(kRecordingsDir)) SD_MMC.mkdir(kRecordingsDir);

  auto dt = M5.Rtc.getDateTime();
  snprintf(_currentFilename, sizeof(_currentFilename),
           "%04d%02d%02d_%02d%02d%02d.wav", dt.date.year, dt.date.month,
           dt.date.date, dt.time.hours, dt.time.minutes, dt.time.seconds);

  char path[48];
  snprintf(path, sizeof(path), "%s/%s", kRecordingsDir, _currentFilename);

  _file = SD_MMC.open(path, FILE_WRITE);
  if (!_file) return;

  WavFile::writeHeaderPlaceholder(_file, kSampleRate, /*bitsPerSample=*/16,
                                   /*numChannels=*/1);
  _dataBytesWritten = 0;
  _recordStartMs = millis();

  M5.Speaker.end();

  // Tab5's board defaults set mic magnification to 2 (vs. the library's
  // usual default of 16), which reads as very quiet for normal speech;
  // also force mono input to match our record() calls below (the board
  // default configures the mic for stereo capture).
  auto micCfg = M5.Mic.config();
  micCfg.magnification = 8;
  micCfg.input_channel = m5::input_channel_t::input_only_left;
  M5.Mic.config(micCfg);
  M5.Mic.begin();

  _state = State::Recording;
  M5.Display.fillScreen(Theme::kBackground);
}

void VoiceRecorderApp::updateRecording() {
  if (M5.Mic.record(_chunkBuf, kChunkSamples, kSampleRate)) {
    _file.write((const uint8_t*)_chunkBuf, kChunkSamples * sizeof(int16_t));
    _dataBytesWritten += kChunkSamples * sizeof(int16_t);
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
  bool insideStop = pressed && _stopBtn.contains(tx, ty);
  _stopBtn.press(insideStop);
  if (_stopBtn.justReleased() || millis() - _recordStartMs > kMaxRecordMs) {
    stopRecording();
  }
}

void VoiceRecorderApp::stopRecording() {
  while (M5.Mic.isRecording()) delay(1);

  WavFile::finalizeHeader(_file, _dataBytesWritten);
  _file.close();
  M5.Mic.end();
  M5.Speaker.begin();

  _state = State::List;
  refreshList();
  layoutList();
  M5.Display.fillScreen(Theme::kBackground);
}

void VoiceRecorderApp::drawRecording() {
  const int screenW = M5.Display.width();
  const int screenH = M5.Display.height();

  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(Theme::kDanger, Theme::kBackground);
  M5.Display.setTextSize(4);
  M5.Display.drawString("Recording...", screenW / 2, screenH / 2 - 80);

  uint32_t elapsedSec = (millis() - _recordStartMs) / 1000;
  char t[16];
  snprintf(t, sizeof(t), "%lu:%02lu", (unsigned long)(elapsedSec / 60),
           (unsigned long)(elapsedSec % 60));
  M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
  M5.Display.setTextSize(7);
  M5.Display.drawString(t, screenW / 2, screenH / 2);

  _stopBtn.drawButton();
}

void VoiceRecorderApp::startPlayback(int index) {
  char path[48];
  snprintf(path, sizeof(path), "%s/%s", kRecordingsDir,
           _recordings[index].filename);

  File f = SD_MMC.open(path, FILE_READ);
  if (!f) return;

  uint32_t sampleRate = kSampleRate, dataBytes = 0;
  uint16_t bits = 16, channels = 1;
  if (!WavFile::readHeader(f, sampleRate, bits, channels, dataBytes)) {
    f.close();
    return;
  }

  if (_playBuf) {
    heap_caps_free(_playBuf);
    _playBuf = nullptr;
  }
  _playBuf = (int16_t*)heap_caps_malloc(dataBytes, MALLOC_CAP_SPIRAM);
  if (!_playBuf) {
    f.close();
    return;
  }

  f.read((uint8_t*)_playBuf, dataBytes);
  f.close();

  _playSamples = dataBytes / sizeof(int16_t);
  _playSampleRate = sampleRate;
  strncpy(_currentFilename, _recordings[index].filename,
          sizeof(_currentFilename) - 1);
  _currentFilename[sizeof(_currentFilename) - 1] = 0;

  M5.Mic.end();
  M5.Speaker.begin();
  M5.Speaker.setVolume(255);  // default master volume is only 64/255
  M5.Speaker.playRaw(_playBuf, _playSamples, _playSampleRate, false, 1, 0);

  _state = State::Playing;
  M5.Display.fillScreen(Theme::kBackground);
}

void VoiceRecorderApp::updatePlayback() {
  bool touched = M5.Touch.getCount() > 0;
  int tx = 0, ty = 0;
  bool pressed = false;
  if (touched) {
    auto t = M5.Touch.getDetail(0);
    tx = t.x;
    ty = t.y;
    pressed = t.isPressed();
  }
  bool insideStop = pressed && _stopBtn.contains(tx, ty);
  _stopBtn.press(insideStop);
  if (_stopBtn.justReleased()) {
    M5.Speaker.stop();
    stopPlayback();
    return;
  }

  if (!M5.Speaker.isPlaying()) {
    stopPlayback();
  }
}

void VoiceRecorderApp::stopPlayback() {
  if (_playBuf) {
    heap_caps_free(_playBuf);
    _playBuf = nullptr;
  }
  M5.Speaker.end();
  _state = State::List;
  layoutList();
  M5.Display.fillScreen(Theme::kBackground);
}

void VoiceRecorderApp::drawPlaying() {
  const int screenW = M5.Display.width();
  const int screenH = M5.Display.height();

  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
  M5.Display.setTextSize(4);
  M5.Display.drawString("Playing...", screenW / 2, screenH / 2 - 60);
  M5.Display.setTextSize(2);
  M5.Display.drawString(_currentFilename, screenW / 2, screenH / 2);

  _stopBtn.drawButton();
}

void VoiceRecorderApp::update() {
  switch (_state) {
    case State::NoSd:
      break;
    case State::List:
      updateList();
      break;
    case State::Recording:
      updateRecording();
      break;
    case State::Playing:
      updatePlayback();
      break;
  }
}

void VoiceRecorderApp::draw() {
  switch (_state) {
    case State::NoSd:
      M5.Display.setTextDatum(middle_center);
      M5.Display.setTextColor(Theme::kTextPrimary, Theme::kBackground);
      M5.Display.setTextSize(3);
      M5.Display.drawString("No SD card detected", M5.Display.width() / 2,
                             M5.Display.height() / 2);
      break;
    case State::List:
      drawList();
      break;
    case State::Recording:
      drawRecording();
      break;
    case State::Playing:
      drawPlaying();
      break;
  }
}
