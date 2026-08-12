#pragma once

#include <FS.h>
#include <M5GFX.h>

#include <cstdint>

#include "App.h"

// Records mono 16kHz PCM to a WAV file on SD in small chunks (via M5.Mic),
// and plays a selected recording back (via M5.Speaker) by loading the whole
// clip into a PSRAM buffer -- simple and, given 32MB of PSRAM, more than
// large enough for ordinary voice-memo lengths. Mic and speaker share one
// I2S bus and can't run at once, so every state transition explicitly ends
// one before beginning the other.
class VoiceRecorderApp : public App {
 public:
  void onEnter() override;
  void onExit() override;
  void update() override;
  void draw() override;
  const char* name() const override { return "Voice Recorder"; }

 private:
  enum class State { NoSd, List, Recording, Playing };

  static constexpr int kMaxRows = 8;
  static constexpr uint32_t kSampleRate = 16000;
  static constexpr int kChunkSamples = 1600;        // 100ms per chunk
  static constexpr uint32_t kMaxRecordMs = 300000;  // 5 min safety cap

  struct RecordingInfo {
    char filename[32];
  };

  void refreshList();
  void layoutList();
  void updateList();
  void drawList();

  void startRecording();
  void updateRecording();
  void stopRecording();
  void drawRecording();

  void startPlayback(int index);
  void updatePlayback();
  void stopPlayback();
  void drawPlaying();

  State _state = State::List;

  RecordingInfo _recordings[kMaxRows];
  int _count = 0;

  LGFX_Button _rowButtons[kMaxRows];
  LGFX_Button _deleteButtons[kMaxRows];
  LGFX_Button _recordBtn;
  LGFX_Button _stopBtn;

  File _file;
  int16_t _chunkBuf[kChunkSamples];
  uint32_t _dataBytesWritten = 0;
  char _currentFilename[32] = {0};
  uint32_t _recordStartMs = 0;

  int16_t* _playBuf = nullptr;
  size_t _playSamples = 0;
  uint32_t _playSampleRate = kSampleRate;
};
