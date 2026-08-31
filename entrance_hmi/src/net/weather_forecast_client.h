#pragma once
#include <Arduino.h>

constexpr uint8_t MAX_FORECAST_ENTRIES = 4;

struct ForecastEntry {
  int hour = 0;       // local hour (0-23) this entry represents
  float temp_c = 0;
  String icon;          // OpenWeatherMap icon code, e.g. "10d"
};

struct ForecastData {
  bool valid = false;
  uint8_t count = 0;
  ForecastEntry items[MAX_FORECAST_ENTRIES];
  unsigned long fetched_at_ms = 0;
};

extern ForecastData forecast_data;

// Call every loop() iteration. No-op until 30 minutes have passed since
// the last fetch (forecast data doesn't need to refresh as often as
// current conditions) — separate cadence from weather_client, hardcoded
// rather than a new SD-config field since this doesn't need field
// tuning. Returns true only if a fetch happened AND changed.
bool weather_forecast_client_tick();

// Force an immediate fetch (e.g. OK button on WEATHER). Blocking,
// bounded by the HTTP timeout.
bool weather_forecast_client_fetch_now();
