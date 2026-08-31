#pragma once
#include <Arduino.h>

enum class Screen : uint8_t { HOME, WEATHER, TRANSIT, HA_CONTROL, STATUS };

struct AppState {
  Screen current_screen = Screen::HOME;
  int8_t list_cursor = 0;
  unsigned long last_full_refresh_ms = 0;
  int last_daily_refresh_yday = -1;  // tm_yday of the last forced daily refresh, -1 = never
  unsigned long last_manual_interaction_ms = 0;
  unsigned long last_auto_rotation_ms = 0;
};

extern AppState app_state;

// Single source of truth for "all screens" — nav.cpp's cycle ring, the
// idle auto-rotation ring, and the footer dock's labels all derive from
// this instead of each hand-maintaining their own list.
struct ScreenInfo {
  Screen screen;
  const char *label;      // footer dock label, keep <=7 chars for the cell width
  bool in_cycle_ring;      // PRV/NEXT (when not list-scrolling) cycles onto it
  bool in_rotation_ring;    // idle auto-rotation cycles onto it
};
extern const ScreenInfo SCREEN_TABLE[];
extern const size_t SCREEN_TABLE_LEN;

// HA_CONTROL owns a scrollable list; other screens don't. This single
// flag is what nav.cpp uses to decide whether PRV/NEXT scroll the
// current screen's list or cycle to a different screen.
bool screen_has_list(Screen screen);

int8_t screen_list_length(Screen screen);

void app_state_enter_screen(Screen screen);

// Standard e-paper ghosting hygiene: true once 30 minutes have passed
// since the last full refresh, or it's the daily quiet hour and today's
// forced refresh hasn't happened yet. ui_common checks this on every
// render (upgrading a partial to a full one) and the main loop also
// checks it directly so the timer fires even during long idle periods
// with no other render trigger.
bool app_state_needs_forced_full_refresh();

// Call whenever a full refresh actually happens, so the scheduling
// clock above resets.
void app_state_mark_full_refresh_done();

// Auto-rotation: cycles HOME -> WEATHER -> TRANSIT on its own, using
// sd_config.screen_rotation_interval_ms both as the cycle cadence and
// as the "how long since the last button press" idle threshold before
// resuming — one knob, not two. Never fires while on HA_CONTROL/STATUS,
// so it can't interrupt someone mid-interaction with the HA controls;
// it also won't pull the user back into rotation from those screens —
// resuming only happens once they navigate back themselves.
void app_state_mark_manual_interaction();
bool app_state_should_auto_rotate();
Screen app_state_next_rotation_screen();
void app_state_mark_auto_rotated();
