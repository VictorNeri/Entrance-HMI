#include "buttons.h"

namespace {

constexpr uint8_t PIN_HOME = 2;
constexpr uint8_t PIN_EXIT = 1;
constexpr uint8_t PIN_PRV = 6;
constexpr uint8_t PIN_NEXT = 4;
constexpr uint8_t PIN_OK = 5;

constexpr unsigned long DEBOUNCE_MS = 30;
constexpr unsigned long HOME_LONG_PRESS_MS = 2000;

struct ButtonPin {
  Button id;
  uint8_t pin;
  bool raw_pressed = false;
  bool stable_pressed = false;
  unsigned long last_change_ms = 0;
  unsigned long pressed_since_ms = 0;
  bool long_fired = false;
};

ButtonPin buttons[] = {
    {Button::HOME, PIN_HOME}, {Button::EXIT, PIN_EXIT}, {Button::PRV, PIN_PRV},
    {Button::NEXT, PIN_NEXT}, {Button::OK, PIN_OK},
};
constexpr size_t NUM_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);

}  // namespace

void buttons_init() {
  for (size_t i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttons[i].pin, INPUT);
  }
}

ButtonEvent buttons_poll() {
  ButtonEvent event;
  unsigned long now = millis();

  for (size_t i = 0; i < NUM_BUTTONS; i++) {
    ButtonPin &b = buttons[i];
    bool raw = digitalRead(b.pin) == LOW;  // active-low

    if (raw != b.raw_pressed) {
      b.raw_pressed = raw;
      b.last_change_ms = now;
    }

    if (now - b.last_change_ms >= DEBOUNCE_MS && b.stable_pressed != b.raw_pressed) {
      b.stable_pressed = b.raw_pressed;
      if (b.stable_pressed) {
        b.pressed_since_ms = now;
        b.long_fired = false;
      } else if (!b.long_fired && event.button == Button::NONE) {
        // Released without a long-press already having fired: short press.
        event.button = b.id;
        event.long_press = false;
      }
    }

    // Long-press fires once, the moment the hold threshold is crossed,
    // rather than waiting for release.
    if (b.id == Button::HOME && b.stable_pressed && !b.long_fired &&
        now - b.pressed_since_ms >= HOME_LONG_PRESS_MS) {
      b.long_fired = true;
      if (event.button == Button::NONE) {
        event.button = b.id;
        event.long_press = true;
      }
    }
  }

  return event;
}
