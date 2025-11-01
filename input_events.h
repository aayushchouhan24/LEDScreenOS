#pragma once
#include <Arduino.h>

// Bit flags for input events posted from BLE callback and processed in loop()
// Keep simple and atomic (32-bit)

enum InputEvent : uint32_t {
  EV_NONE          = 0,
  EV_HOME          = 1u << 0,
  EV_MENU          = 1u << 1,
  EV_NAV_UP        = 1u << 2,
  EV_NAV_DOWN      = 1u << 3,
  EV_SELECT        = 1u << 4,
  EV_BACK          = 1u << 5,
  EV_SNAKE_UP      = 1u << 6,
  EV_SNAKE_DOWN    = 1u << 7,
  EV_SNAKE_LEFT    = 1u << 8,
  EV_SNAKE_RIGHT   = 1u << 9,
  EV_SNAKE_RESTART = 1u << 10,
  // Generic adjustments for settings/values
  EV_DEC           = 1u << 11,
  EV_INC           = 1u << 12,
};

// Global event bitfield (written from BLE callback, read/cleared in loop)
extern volatile uint32_t gInputEvents;

// Helper to post an event (OR into bitfield)
inline void postInputEvent(InputEvent ev) {
  gInputEvents |= static_cast<uint32_t>(ev);
}
