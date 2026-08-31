#include "calendar_store.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "../net/time_sync.h"

CalendarEventList calendar_event_list;

namespace {

constexpr const char *CONFIG_PATH = "/config/calendar.json";

bool parse_into(const char *payload, size_t length, CalendarEventList &out) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err != DeserializationError::Ok) return false;

  JsonArray events = doc["events"].as<JsonArray>();
  if (events.isNull()) return false;

  CalendarEventList next;
  for (JsonObject e : events) {
    if (next.count >= MAX_CALENDAR_EVENTS) break;
    time_t start = (time_t)(e["start"] | 0);
    if (start <= 0) continue;  // start is required — skip malformed entries

    CalendarEvent &item = next.items[next.count];
    item.title = (const char *)(e["title"] | "");
    item.start_epoch = start;
    item.end_epoch = (time_t)(e["end"] | 0);
    next.count++;
  }

  if (next.count == 0) return false;
  next.valid = true;
  out = next;
  return true;
}

}  // namespace

void calendar_store_load_from_disk() {
  if (!LittleFS.exists(CONFIG_PATH)) return;

  File f = LittleFS.open(CONFIG_PATH, "r");
  if (!f) return;

  String contents = f.readString();
  f.close();

  CalendarEventList loaded;
  if (parse_into(contents.c_str(), contents.length(), loaded)) {
    calendar_event_list = loaded;
    Serial.printf("[calendar] loaded %u events from disk cache\n", (unsigned)loaded.count);
  }
}

bool calendar_store_apply_payload(const char *payload, size_t length) {
  CalendarEventList parsed;
  if (!parse_into(payload, length, parsed)) {
    Serial.println("[calendar] rejected invalid calendar payload, keeping previous data");
    return false;
  }

  calendar_event_list = parsed;

  // Re-serialize rather than writing the raw payload back, so the
  // on-disk cache always matches what was actually parsed/accepted.
  File f = LittleFS.open(CONFIG_PATH, "w");
  if (f) {
    JsonDocument doc;
    doc["version"] = 1;
    JsonArray arr = doc["events"].to<JsonArray>();
    for (uint8_t i = 0; i < parsed.count; i++) {
      JsonObject o = arr.add<JsonObject>();
      o["title"] = parsed.items[i].title;
      o["start"] = (long)parsed.items[i].start_epoch;
      o["end"] = (long)parsed.items[i].end_epoch;
    }
    serializeJson(doc, f);
    f.close();
  }

  Serial.printf("[calendar] applied %u events, free heap: %u bytes\n", (unsigned)parsed.count,
                (unsigned)ESP.getFreeHeap());
  return true;
}

uint8_t calendar_pending_count() {
  if (!calendar_event_list.valid || calendar_event_list.count == 0) return 0;

  struct tm now_tm;
  if (!time_sync_get_local(now_tm)) return 0;
  time_t now = mktime(&now_tm);

  uint8_t pending = 0;
  for (uint8_t i = 0; i < calendar_event_list.count; i++) {
    const CalendarEvent &ev = calendar_event_list.items[i];
    time_t effective_end = ev.end_epoch != 0 ? ev.end_epoch : ev.start_epoch;
    if (effective_end >= now) pending++;
  }
  return pending;
}
