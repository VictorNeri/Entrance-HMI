#pragma once
#include <Arduino.h>

enum class Button : uint8_t { NONE, HOME, EXIT, PRV, NEXT, OK };

struct ButtonEvent {
  Button button = Button::NONE;
  bool long_press = false;
};

void buttons_init();

// Call once per loop() iteration. Non-blocking: returns immediately with
// Button::NONE most ticks, and at most one event per call otherwise.
ButtonEvent buttons_poll();
