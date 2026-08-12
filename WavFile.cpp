#include "WavFile.h"

#include <cstring>

namespace {

void writeU32LE(File& f, uint32_t v) {
  uint8_t b[4] = {(uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16),
                  (uint8_t)(v >> 24)};
  f.write(b, 4);
}

void writeU16LE(File& f, uint16_t v) {
  uint8_t b[2] = {(uint8_t)v, (uint8_t)(v >> 8)};
  f.write(b, 2);
}

uint32_t readU32LE(File& f) {
  uint8_t b[4] = {0, 0, 0, 0};
  f.read(b, 4);
  return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) |
         ((uint32_t)b[3] << 24);
}

uint16_t readU16LE(File& f) {
  uint8_t b[2] = {0, 0};
  f.read(b, 2);
  return (uint16_t)b[0] | ((uint16_t)b[1] << 8);
}

}  // namespace

namespace WavFile {

void writeHeaderPlaceholder(File& f, uint32_t sampleRate,
                             uint16_t bitsPerSample, uint16_t numChannels) {
  f.write((const uint8_t*)"RIFF", 4);
  writeU32LE(f, 0);  // ChunkSize, filled in by finalizeHeader()
  f.write((const uint8_t*)"WAVE", 4);

  f.write((const uint8_t*)"fmt ", 4);
  writeU32LE(f, 16);
  writeU16LE(f, 1);  // PCM
  writeU16LE(f, numChannels);
  writeU32LE(f, sampleRate);
  uint32_t byteRate = sampleRate * numChannels * (bitsPerSample / 8);
  writeU32LE(f, byteRate);
  uint16_t blockAlign = numChannels * (bitsPerSample / 8);
  writeU16LE(f, blockAlign);
  writeU16LE(f, bitsPerSample);

  f.write((const uint8_t*)"data", 4);
  writeU32LE(f, 0);  // Subchunk2Size, filled in by finalizeHeader()
}

void finalizeHeader(File& f, uint32_t dataBytes) {
  f.seek(4);
  writeU32LE(f, 36 + dataBytes);
  f.seek(40);
  writeU32LE(f, dataBytes);
}

bool readHeader(File& f, uint32_t& sampleRate, uint16_t& bitsPerSample,
                 uint16_t& numChannels, uint32_t& dataBytes) {
  char tag[4];
  f.seek(0);

  f.read((uint8_t*)tag, 4);
  if (memcmp(tag, "RIFF", 4) != 0) return false;
  readU32LE(f);  // chunk size, unused

  f.read((uint8_t*)tag, 4);
  if (memcmp(tag, "WAVE", 4) != 0) return false;

  f.read((uint8_t*)tag, 4);
  if (memcmp(tag, "fmt ", 4) != 0) return false;
  uint32_t fmtSize = readU32LE(f);
  readU16LE(f);  // audio format, unused
  numChannels = readU16LE(f);
  sampleRate = readU32LE(f);
  readU32LE(f);  // byte rate, unused
  readU16LE(f);  // block align, unused
  bitsPerSample = readU16LE(f);
  if (fmtSize > 16) f.seek(f.position() + (fmtSize - 16));

  f.read((uint8_t*)tag, 4);
  if (memcmp(tag, "data", 4) != 0) return false;
  dataBytes = readU32LE(f);
  return true;
}

}  // namespace WavFile
