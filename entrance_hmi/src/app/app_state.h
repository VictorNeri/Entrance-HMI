#pragma once
#include <Arduino.h>

enum class Screen : uint8_t { HOME, WEATHER, TRANSIT, HA_CONTROL, STATUS };

struct AppState {
  Screen current_screen = Screen::HOME;
  int8_t list_cursor = 0;
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
