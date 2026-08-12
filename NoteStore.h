#pragma once

#include <cstddef>

// Lists/reads/writes/deletes plain-text notes under /notes on the SD card
// (mounted via SD_MMC in Test.ino's setup()). Filenames are RTC timestamps
// so the list can be sorted newest-first without a separate index file.
class NoteStore {
 public:
  static constexpr int kMaxNotes = 64;
  static constexpr int kNameLen = 32;
  static constexpr size_t kMaxNoteBytes = 4096;

  struct NoteInfo {
    char filename[kNameLen];
  };

  bool sdReady() const;
  int refresh();  // rescans /notes, newest first; returns count
  int count() const { return _count; }
  const NoteInfo& at(int index) const { return _notes[index]; }

  bool readNote(int index, char* buf, size_t bufLen);
  bool createNote(const char* text, char* outFilename, size_t outLen);
  bool updateNote(int index, const char* text);
  bool deleteNote(int index);

 private:
  NoteInfo _notes[kMaxNotes];
  int _count = 0;
};
