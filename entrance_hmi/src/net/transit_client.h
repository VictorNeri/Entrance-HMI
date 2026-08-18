#pragma once
#include <Arduino.h>

constexpr uint8_t MAX_DEPARTURES = 8;

struct Departure {
  String line;             // e.g. "11"
  String transport_mode;   // e.g. "METRO", "BUS", "TRAM", "TRAIN", "SHIP"
  String destination;      // may contain Swedish characters — draw with draw_utf8_string()
  String display;          // human-friendly countdown, e.g. "Nu", "3 min"
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
