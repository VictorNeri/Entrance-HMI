#pragma once
#include <Arduino.h>

// Background FreeRTOS task (pinned to core 0, alongside the WiFi stack)
// that owns every blocking HTTPS fetch: current weather, forecast, and
// SL transit. Before this, those ran inline in loop() and could stall
// it for 30s+ (a transit fetch alone is bounded at 15s and retried
// once) — buttons weren't polled during that time, so a short press
// that started and ended inside a fetch was never seen at all.
//
// Threading model: the worker never touches the published globals the
// UI reads (weather_data, forecast_data, departure_list). It fetches
// into private copies and stages each finished result under a mutex;
// fetch_worker_collect(), called from loop(), moves staged results
// into the globals. So the globals stay main-thread-only and the UI
// code needs no locking.

struct FetchUpdates {
  // New current-weather data was published AND either the values
  // differ from what was shown or the fetch was user-requested (OK
  // button) — a silent background poll with identical data doesn't
  // cost an e-paper refresh.
  bool weather = false;
  // New forecast / departures were published (any successful fetch).
  bool forecast = false;
  bool transit = false;
};

// Starts the task. Call once from setup(), after sd_config_load().
void fetch_worker_begin();

// Transit polls at sd_config.transit_poll_active_ms while true,
// transit_poll_background_ms otherwise. Cheap — call every loop().
void fetch_worker_set_transit_active(bool active);

// While true, scheduled transit polls are skipped (night mode — nothing
// on screen shows departures). Explicit fetch_worker_request_transit()
// calls still run. Cheap — call every loop().
void fetch_worker_set_transit_paused(bool paused);

// Non-blocking: ask for an immediate refetch (OK button, or arriving
// on the TRANSIT screen). The result shows up in a later
// fetch_worker_collect(). Requests made while WiFi is down are kept
// and run as soon as it's back. Requests made while that endpoint is
// backing off after an HTTP 429 are dropped — the next scheduled poll
// after the backoff picks it up.
void fetch_worker_request_weather();  // current conditions + forecast
void fetch_worker_request_transit();

// Main thread only. Publishes any staged results into the globals and
// reports what changed. Call every loop() iteration.
FetchUpdates fetch_worker_collect();
