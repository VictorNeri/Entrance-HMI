#pragma once
#include <Arduino.h>
#include <time.h>

constexpr uint8_t MAX_DEPARTURES = 8;

struct Departure {
  String line;             // e.g. "11"
  String transport_mode;   // e.g. "METRO", "BUS", "TRAM", "TRAIN", "SHIP"
  String destination;      // may contain Swedish characters — draw with draw_utf8_string()
  String display;          // human-friendly countdown, e.g. "Nu", "3 min"
  time_t expected_epoch = 0;  // parsed from SL's `expected` field; 0 if unparseable
};

// SL disruption notices ("deviations"): stop-wide ones from the
// response's `stop_deviations`, plus any attached to individual
// departures. Deduplicated by message and capped at the
// MAX_TRANSIT_NOTICES most important; messages are truncated to
// MAX_TRANSIT_NOTICE_LEN bytes (Swedish UTF-8 text — draw with the
// ui_text_unicode helpers).
constexpr uint8_t MAX_TRANSIT_NOTICES = 2;
constexpr size_t MAX_TRANSIT_NOTICE_LEN = 300;

struct TransitNotice {
  String message;
  int importance = 0;  // SL's importance_level; higher = more important
};

struct DepartureList {
  bool valid = false;
  uint8_t count = 0;
  Departure items[MAX_DEPARTURES];
  uint8_t notice_count = 0;
  TransitNotice notices[MAX_TRANSIT_NOTICES];  // most important first
  unsigned long fetched_at_ms = 0;
};

// Published copy the UI reads. Main thread only — written by
// fetch_worker_collect(), never by the fetch itself.
extern DepartureList departure_list;

// One blocking fetch (retried once on failure, except on HTTP 429;
// each attempt bounded by the HTTP timeout) into `out`; `out` is
// untouched on failure. `http_status`, if given, receives the last
// attempt's HTTP status code (see http_get_json()). Called from the
// fetch worker task only, which owns the active/background poll
// cadence and the 429 backoff — see fetch_worker.h.
bool transit_client_fetch(DepartureList &out, int *http_status = nullptr);

// True if this departure is still catchable given sd_config.walk_time_min
// minutes to walk to the station — i.e. its expected time is at least
// that far in the future. Always true if walk_time_min is 0 (filter
// off), the timestamp didn't parse, or local time isn't synced yet
// (can't safely compare — showing everything is the safer default over
// hiding real departures on a bad assumption).
bool transit_departure_is_reachable(const Departure &dep);
