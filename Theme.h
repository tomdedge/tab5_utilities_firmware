#pragma once

#include <cstdint>

// Shared color palette. Values are raw 24-bit RGB888 hex -- LGFX_Button's
// initButtonUL()/drawButton() are templated on color type and deduce it
// from the arguments, so every color passed into the *same* call must be
// this same uint32_t type (mixing e.g. a uint32_t Theme color with a plain
// `int`-typed TFT_* macro in one call fails template deduction -- see
// CLAUDE.md). Keep all UI color literals routed through here rather than
// TFT_* named colors so the whole app reads as one cohesive dark theme
// instead of a grab-bag of saturated primaries.
namespace Theme {

constexpr uint32_t kBackground = 0x0F1115;     // screen background
constexpr uint32_t kSurface = 0x1A1D24;        // status bar / raised surfaces
constexpr uint32_t kOutline = 0x30343D;        // button borders

constexpr uint32_t kPrimary = 0x3B82F6;        // primary action / selected state
constexpr uint32_t kSuccess = 0x22C55E;        // confirm / connected / positive
constexpr uint32_t kDanger = 0xEF4444;         // destructive / cancel / alert

constexpr uint32_t kTextPrimary = 0xF3F4F6;    // main text on dark backgrounds
constexpr uint32_t kTextSecondary = 0x9CA3AF;  // muted/secondary text

}  // namespace Theme
