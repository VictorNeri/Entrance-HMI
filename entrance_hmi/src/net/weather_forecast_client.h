#pragma once
#include <Arduino.h>

// 8 entries at OWM's 3-hourly cadence = 24h coverage, for the WEATHER
// screen's temperature-over-the-day chart. Bumped up from 4 (was ~12h)
// — NOT verified against the same on-device payload-size failure this
// project has hit before on a different endpoint (SL transit's forecast
// window; see README's walk_time_min note). The filtered JSON parse
// stays cheap either way (weather_forecast_client.cpp only keeps 3
// fields per entry), but the raw HTTP response size roughly doubles
// with the entry count — watch serial/net-log output after flashing
// for a fetch failure here specifically before trusting this.
constexpr uint8_t MAX_FORECAST_ENTRIES = 8;

// Forecast doesn't need to refresh as often as current conditions —
// separate cadence, hardcoded rather than an SD-config field since it
// doesn't need field tuning. Used by fetch_worker (scheduling) and
// ui_stale (when to flag the data as old).
constexpr unsigned long FORECAST_POLL_INTERVAL_MS = 30UL * 60 * 1000;

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

// Published copy the UI reads. Main thread only — written by
// fetch_worker_collect(), never by the fetch itself.
extern ForecastData forecast_data;

// One blocking fetch (bounded by the HTTP timeout) into `out`; `out` is
// untouched on failure. Called from the fetch worker task only, which
// also owns the 30-minute cadence — see fetch_worker.h.
// `http_status`, if given, receives the HTTP status code (see
// http_get_json()).
bool weather_forecast_client_fetch(ForecastData &out, int *http_status = nullptr);
