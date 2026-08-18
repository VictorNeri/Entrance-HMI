#pragma once
#include <Arduino.h>

enum class Screen : uint8_t { HOME, WEATHER, TRANSIT, HA_CONTROL, STATUS };

struct AppState {
  Screen current_screen = Screen::HOME;
  int8_t list_cursor = 0;
  unsigned long last_full_refresh_ms = 0;
  int last_daily_refresh_yday = -1;  // tm_yday of the last forced daily refresh, -1 = never
};

extern AppState app_state;

// TRANSIT and HA_CONTROL own a scrollable list; other screens don't.
// This single flag is what nav.cpp uses to decide whether PRV/NEXT
// scroll the current screen's list or cycle to a different screen.
bool screen_has_list(Screen screen);

// Placeholder counts for M1 — replaced by real DepartureList/HaEntityList
// sizes once M4/M5 wire up live data.
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
