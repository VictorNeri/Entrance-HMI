#include "nav.h"
#include "app_state.h"

namespace {

constexpr Screen SCREEN_RING[] = {Screen::HOME, Screen::WEATHER, Screen::TRANSIT,
                                   Screen::HA_CONTROL};
constexpr size_t SCREEN_RING_LEN = sizeof(SCREEN_RING) / sizeof(SCREEN_RING[0]);

size_t ring_index_of(Screen screen) {
  for (size_t i = 0; i < SCREEN_RING_LEN; i++) {
    if (SCREEN_RING[i] == screen) return i;
  }
  return 0;
}

void cycle_screen(int8_t direction) {
  size_t idx = ring_index_of(app_state.current_screen);
  idx = (idx + SCREEN_RING_LEN + direction) % SCREEN_RING_LEN;
  app_state_enter_screen(SCREEN_RING[idx]);
}

// Clamped, not wraparound — avoids confusing jumps near real-time list data.
bool scroll_list(int8_t direction) {
  int8_t length = screen_list_length(app_state.current_screen);
  if (length <= 0) return false;
  int8_t next = app_state.list_cursor + direction;
  if (next < 0) next = 0;
  if (next > length - 1) next = length - 1;
  if (next == app_state.list_cursor) return false;
  app_state.list_cursor = next;
  return true;
}

}  // namespace

RedrawKind nav_handle_event(const ButtonEvent &event) {
  switch (event.button) {
    case Button::HOME:
      if (event.long_press) {
        if (app_state.current_screen == Screen::STATUS) return RedrawKind::NONE;
        app_state_enter_screen(Screen::STATUS);
      } else {
        if (app_state.current_screen == Screen::HOME) return RedrawKind::NONE;
        app_state_enter_screen(Screen::HOME);
      }
      return RedrawKind::FULL;

    case Button::EXIT:
      if (app_state.current_screen == Screen::HOME) return RedrawKind::NONE;
      app_state_enter_screen(Screen::HOME);
      return RedrawKind::FULL;

    case Button::PRV:
      if (screen_has_list(app_state.current_screen)) {
        return scroll_list(-1) ? RedrawKind::PARTIAL : RedrawKind::NONE;
      }
      cycle_screen(-1);
      return RedrawKind::FULL;

    case Button::NEXT:
      if (screen_has_list(app_state.current_screen)) {
        return scroll_list(1) ? RedrawKind::PARTIAL : RedrawKind::NONE;
      }
      cycle_screen(1);
      return RedrawKind::FULL;

    case Button::OK:
      // Context action (data refresh / MQTT toggle) wired up starting M3.
      return RedrawKind::NONE;

    default:
      return RedrawKind::NONE;
  }
}
