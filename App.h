#pragma once

#include <cstdint>

#include "Theme.h"

// Common interface every mini-app implements so AppManager can drive them
// uniformly. Big buffers/peripherals should be allocated in onEnter() and
// released in onExit() rather than at construction time, so idle apps cost
// near-zero RAM.
class App {
 public:
  virtual void onEnter() {}
  virtual void onExit() {}
  virtual void update() = 0;
  virtual void draw() = 0;
  virtual const char* name() const = 0;

  // Fill color for this app's tile in MenuApp's grid. Override to stand
  // out from the default (e.g. Settings uses green).
  virtual uint32_t tileColor() const { return Theme::kPrimary; }

  virtual ~App() {}
};
