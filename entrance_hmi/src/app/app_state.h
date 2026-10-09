#pragma once
#include <Arduino.h>

enum class Screen : uint8_t { HOME, WEATHER, TRANSIT, HA_CONTROL, STATUS };

struct AppState {
  Screen current_screen = Screen::HOME;
  int8_t list_cursor = 0;
  unsigned long last_full_refresh_ms = 0;
  int last_daily_refresh_yday = -1;  // tm_yday of the last forced daily refresh, -1 = never
  unsigned long last_manual_interaction_ms = 0;
};

extern AppState app_state;

// Single source of truth for "all screens" — nav.cpp's PRV/NEXT cycle
// ring and the footer dock's labels both derive from this instead of
// each hand-maintaining their own list. Order here is the physical
// layout: HOME is the center the user always lands back on, PRV walks
// "up" toward WEATHER, NEXT walks "down" toward TRANSIT/HA_CONTROL/
// STATUS — see app_state_should_return_home() for what brings them
// back.
struct ScreenInfo {
  Screen screen;
  const char *label;      // footer dock label, keep <=7 chars for the cell width
  bool in_cycle_ring;      // PRV/NEXT (when not list-scrolling) cycles onto it
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

void app_state_mark_manual_interaction();

// True once sd_config.screen_rotation_interval_ms has passed since the
// last button press AND the current screen isn't already HOME — the
// main loop responds by entering HOME. Deliberately uniform across
// every other screen (WEATHER/TRANSIT/HA_CONTROL/STATUS alike): the
// point is a quick glance anywhere always settles back on HOME on its
// own, rather than the old behavior of endlessly auto-rotating through
// a subset of screens. Naturally self-limiting — once current_screen
// is HOME this returns false, so the main loop's app_state_enter_screen
// call only fires once per idle period, not every tick.
bool app_state_should_return_home();
