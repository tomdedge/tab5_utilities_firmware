#include "NoteStore.h"

#include <M5Unified.h>
#include <SD_MMC.h>

#include <cstdio>
#include <cstring>

namespace {
const char* kNotesDir = "/notes";
}

bool NoteStore::sdReady() const { return SD_MMC.cardType() != CARD_NONE; }

int NoteStore::refresh() {
  _count = 0;
  if (!sdReady()) return 0;

  if (!SD_MMC.exists(kNotesDir)) {
    SD_MMC.mkdir(kNotesDir);
    return 0;
  }

  File dir = SD_MMC.open(kNotesDir);
  if (!dir || !dir.isDirectory()) return 0;

  File f = dir.openNextFile();
  while (f && _count < kMaxNotes) {
    if (!f.isDirectory()) {
      const char* n = f.name();
      const char* slash = strrchr(n, '/');
      const char* base = slash ? slash + 1 : n;
      strncpy(_notes[_count].filename, base, kNameLen - 1);
      _notes[_count].filename[kNameLen - 1] = 0;
      _count++;
    }
    f.close();
    f = dir.openNextFile();
  }
  dir.close();

  // Filenames are timestamps, so reversing gives newest-first.
  for (int i = 0; i < _count / 2; ++i) {
    NoteInfo tmp = _notes[i];
    _notes[i] = _notes[_count - 1 - i];
    _notes[_count - 1 - i] = tmp;
  }
  return _count;
}

bool NoteStore::readNote(int index, char* buf, size_t bufLen) {
  if (index < 0 || index >= _count || bufLen == 0) return false;
  char path[48];
  snprintf(path, sizeof(path), "%s/%s", kNotesDir, _notes[index].filename);

  File f = SD_MMC.open(path, FILE_READ);
  if (!f) return false;
  size_t n = f.readBytes(buf, bufLen - 1);
  buf[n] = 0;
  f.close();
  return true;
}

bool NoteStore::createNote(const char* text, char* outFilename,
                            size_t outLen) {
  if (!sdReady()) return false;
  if (!SD_MMC.exists(kNotesDir)) SD_MMC.mkdir(kNotesDir);

  char filename[kNameLen];
  char path[48];
  int suffix = 0;
  do {
    auto dt = M5.Rtc.getDateTime();
    if (suffix == 0) {
      snprintf(filename, sizeof(filename), "%04d%02d%02d_%02d%02d%02d.txt",
               dt.date.year, dt.date.month, dt.date.date, dt.time.hours,
               dt.time.minutes, dt.time.seconds);
    } else {
      snprintf(filename, sizeof(filename), "%04d%02d%02d_%02d%02d%02d_%d.txt",
               dt.date.year, dt.date.month, dt.date.date, dt.time.hours,
               dt.time.minutes, dt.time.seconds, suffix);
    }
    snprintf(path, sizeof(path), "%s/%s", kNotesDir, filename);
    suffix++;
  } while (SD_MMC.exists(path) && suffix < 10);

  File f = SD_MMC.open(path, FILE_WRITE);
  if (!f) return false;
  f.print(text);
  f.close();

  if (outFilename) {
    strncpy(outFilename, filename, outLen - 1);
    outFilename[outLen - 1] = 0;
  }
  return true;
}

bool NoteStore::updateNote(int index, const char* text) {
  if (index < 0 || index >= _count) return false;
  char path[48];
  snprintf(path, sizeof(path), "%s/%s", kNotesDir, _notes[index].filename);

  File f = SD_MMC.open(path, FILE_WRITE);
  if (!f) return false;
  f.print(text);
  f.close();
  return true;
}

bool NoteStore::deleteNote(int index) {
  if (index < 0 || index >= _count) return false;
  char path[48];
  snprintf(path, sizeof(path), "%s/%s", kNotesDir, _notes[index].filename);
  return SD_MMC.remove(path);
}
