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

struct DepartureList {
  bool valid = false;
  uint8_t count = 0;
  Departure items[MAX_DEPARTURES];
  unsigned long fetched_at_ms = 0;
};

extern DepartureList departure_list;

// Call every loop() iteration. `poll_interval_ms` is supplied by the
// caller so it can implement screen-active-gated polling (fast while
// TRANSIT is visible, slow otherwise) without this module needing to
// know about screen state.
bool transit_client_tick(unsigned long poll_interval_ms);

// Force an immediate fetch (OK button, or re-arming fast polling right
// after navigating onto the TRANSIT screen). Blocking, bounded by the
// HTTP timeout.
bool transit_client_fetch_now();

// True if this departure is still catchable given sd_config.walk_time_min
// minutes to walk to the station — i.e. its expected time is at least
// that far in the future. Always true if walk_time_min is 0 (filter
// off), the timestamp didn't parse, or local time isn't synced yet
// (can't safely compare — showing everything is the safer default over
// hiding real departures on a bad assumption).
bool transit_departure_is_reachable(const Departure &dep);
