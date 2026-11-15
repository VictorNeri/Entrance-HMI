#pragma once
#include <Arduino.h>

// Night mode: overnight the panel shows one static night screen instead
// of HOME, so it isn't repainted every minute (e-paper wear/ghosting)
// and stops background SL polling. A frozen clock or an hours-old
// "3 min" countdown would be misleading at 2am, so the night screen
// shows neither.
//
// Hours come from sd_config.night_start_hour / night_end_hour (local
// time, window may wrap midnight); equal values turn night mode off.

// How long the normal HOME screen stays up after a button press during
// night hours before dropping back to the night screen.
constexpr unsigned long NIGHT_WAKE_MS = 60UL * 1000;

// True inside the configured night window. False if night mode is off
// or local time isn't synced yet (can't tell, so stay in normal mode).
bool night_mode_in_hours();

// True when the night screen should be showing right now: night hours,
// on HOME, no pending broadcast, and no button press in the last
// NIGHT_WAKE_MS.
bool night_mode_active();
