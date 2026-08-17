#pragma once
#include <Arduino.h>

struct WeatherData {
  bool valid = false;
  float temp_c = 0;
  float feels_like_c = 0;
  int humidity = 0;
  String description;  // e.g. "clear sky"
  String icon;          // OpenWeatherMap icon code, e.g. "01d"
  unsigned long fetched_at_ms = 0;
};

extern WeatherData weather_data;

// Call every loop() iteration. No-op until 10 minutes have passed
// since the last fetch. Returns true only if a fetch happened AND the
// values actually changed (so callers can skip a redraw on identical
// data — avoids needless e-paper wear on the automatic background poll).
bool weather_client_tick();

// Force an immediate fetch (e.g. OK button on the WEATHER screen).
// Blocking, bounded by the HTTP timeout. Returns true if the fetch
// succeeded (regardless of whether values changed — this is a
// deliberate user-triggered action, not the silent background poll).
bool weather_client_fetch_now();
