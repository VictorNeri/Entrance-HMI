#include "nav.h"
#include "app_state.h"

namespace {

// Filters SCREEN_TABLE (the single source of truth, shared with the
// idle auto-rotation ring in app_state.cpp and the footer dock in
// ui_chrome.cpp) down to the screens PRV/NEXT cycle through. Rebuilt
// on each call rather than cached — SCREEN_TABLE_LEN is 5, the cost is
// negligible, and it avoids relying on cross-translation-unit global
// constructor ordering.
size_t build_cycle_ring(Screen out[]) {
  size_t len = 0;
  for (size_t i = 0; i < SCREEN_TABLE_LEN; i++) {
    if (SCREEN_TABLE[i].in_cycle_ring) out[len++] = SCREEN_TABLE[i].screen;
  }
  return len;
}

void cycle_screen(int8_t direction) {
  Screen ring[SCREEN_TABLE_LEN];
  size_t len = build_cycle_ring(ring);
  if (len == 0) return;

  size_t idx = 0;
  for (size_t i = 0; i < len; i++) {
    if (ring[i] == app_state.current_screen) {
      idx = i;
      break;
    }
  }
  idx = (idx + len + direction) % len;
  app_state_enter_screen(ring[idx]);
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
