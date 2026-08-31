#pragma once
#include <Arduino.h>
#include <time.h>

constexpr uint8_t MAX_CALENDAR_EVENTS = 16;

struct CalendarEvent {
  String title;         // not rendered anywhere yet (header shows only a
                         // count) — kept for a possible future events screen.
  time_t start_epoch = 0;  // unix epoch seconds
  time_t end_epoch = 0;    // unix epoch seconds; 0 = point-in-time (== start_epoch)
};

struct CalendarEventList {
  bool valid = false;
  uint8_t count = 0;
  CalendarEvent items[MAX_CALENDAR_EVENTS];
};

extern CalendarEventList calendar_event_list;

// Loads the last-persisted calendar from LittleFS into calendar_event_list.
// Safe to call even if nothing was ever saved (leaves the list empty).
// Call this at boot, before WiFi/MQTT connect, so the header's
// pending-events indicator has something to show immediately even if the
// broker is unreachable.
void calendar_store_load_from_disk();

// Parses an MQTT calendar payload (see README for the JSON schema). On
// success, updates calendar_event_list in memory AND persists it to
// LittleFS. On parse failure, the existing (possibly stale but
// last-known-good) calendar_event_list and on-disk cache are left
// untouched. Returns true on success.
bool calendar_store_apply_payload(const char *payload, size_t length);

// Count of events not yet ended, compared against the current local time
// (time_sync_get_local()). Returns 0 if the list is empty/invalid, or if
// time isn't synced yet (can't safely compare).
uint8_t calendar_pending_count();
