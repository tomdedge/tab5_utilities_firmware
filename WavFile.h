#pragma once

#include <FS.h>

#include <cstdint>

// Minimal 44-byte PCM WAV header read/write, used to stream recordings to/
// from SD in small chunks instead of holding a whole clip in RAM.
namespace WavFile {

// Writes a header with a zeroed data size; call finalizeHeader() once the
// total data byte count is known (after all chunks have been written).
void writeHeaderPlaceholder(File& f, uint32_t sampleRate,
                             uint16_t bitsPerSample, uint16_t numChannels);

void finalizeHeader(File& f, uint32_t dataBytes);

bool readHeader(File& f, uint32_t& sampleRate, uint16_t& bitsPerSample,
                uint16_t& numChannels, uint32_t& dataBytes);

}  // namespace WavFile
