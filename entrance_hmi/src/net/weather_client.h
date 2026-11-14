#pragma once
#include <Arduino.h>

struct WeatherData {
  bool valid = false;
  float temp_c = 0;
  float feels_like_c = 0;
  int humidity = 0;
  float wind_speed_ms = 0;  // meters/second, from OWM's wind.speed
  String description;  // e.g. "clear sky"
  String icon;          // OpenWeatherMap icon code, e.g. "01d"
  unsigned long fetched_at_ms = 0;
};

// Published copy the UI reads. Main thread only — written by
// fetch_worker_collect(), never by the fetch itself.
extern WeatherData weather_data;

// One blocking fetch (bounded by the HTTP timeout) into `out`; `out` is
// untouched on failure. Called from the fetch worker task only — see
// fetch_worker.h for scheduling.
// `http_status`, if given, receives the HTTP status code (see
// http_get_json()).
bool weather_client_fetch(WeatherData &out, int *http_status = nullptr);

// True if the visible values differ enough to be worth an e-paper
// refresh (skips redraws on a background poll with identical data).
bool weather_data_differs(const WeatherData &a, const WeatherData &b);
