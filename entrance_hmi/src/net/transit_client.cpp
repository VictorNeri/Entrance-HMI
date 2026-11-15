#include "transit_client.h"
#include <ArduinoJson.h>
#include <stdio.h>
#include <time.h>
#include "../storage/sd_config.h"
#include "http_client_helper.h"
#include "root_certs.h"
#include "time_sync.h"
#include "wifi_manager.h"
#include "net_log_shadow.h"

DepartureList departure_list;

namespace {

// SL's `expected`/`scheduled` fields are "YYYY-MM-DDTHH:MM:SS", no UTC
// offset — local Stockholm wall-clock time, same frame as this
// device's own NTP-synced local clock, so mktime() (which uses the
// configured TZ) lines them up correctly without any manual offset
// math. Returns 0 on anything that doesn't parse.
time_t parse_sl_datetime(const char *iso) {
  if (iso == nullptr) return 0;
  struct tm tm_val = {};
  int matched = sscanf(iso, "%d-%d-%dT%d:%d:%d", &tm_val.tm_year, &tm_val.tm_mon, &tm_val.tm_mday,
                        &tm_val.tm_hour, &tm_val.tm_min, &tm_val.tm_sec);
  if (matched != 6) return 0;
  tm_val.tm_year -= 1900;
  tm_val.tm_mon -= 1;
  tm_val.tm_isdst = -1;
  return mktime(&tm_val);
}

// Adds one deviation to `list`, keeping its notices deduplicated by
// message and sorted most important first; a new notice less
// important than everything in an already-full list is dropped.
void add_notice(DepartureList &list, JsonVariantConst deviation) {
  const char *message = deviation["message"].as<const char *>();
  if (message == nullptr || message[0] == '\0') return;
  int importance = deviation["importance_level"] | 0;

  String text(message);
  if (text.length() > MAX_TRANSIT_NOTICE_LEN) text = text.substring(0, MAX_TRANSIT_NOTICE_LEN);
  for (uint8_t i = 0; i < list.notice_count; i++) {
    if (list.notices[i].message == text) return;
  }

  uint8_t pos = list.notice_count;
  while (pos > 0 && list.notices[pos - 1].importance < importance) pos--;
  if (pos >= MAX_TRANSIT_NOTICES) return;

  uint8_t last = list.notice_count < MAX_TRANSIT_NOTICES ? list.notice_count : MAX_TRANSIT_NOTICES - 1;
  for (uint8_t i = last; i > pos; i--) list.notices[i] = list.notices[i - 1];
  list.notices[pos].message = text;
  list.notices[pos].importance = importance;
  if (list.notice_count < MAX_TRANSIT_NOTICES) list.notice_count++;
}

}  // namespace

bool transit_client_fetch(DepartureList &out, int *http_status) {
  // The forecast window has to cover at least the walk time, or the
  // walk-time filter would hide every departure the API even returns.
  // Scaled with a buffer so there's still something left to show after
  // filtering, capped at 30 — the earlier size investigation (site
  // 9109: forecast=10 -> ~11KB, forecast=30 -> ~22KB, forecast=60 ->
  // ~30KB and consistently failed to fully download on-device) puts 30
  // comfortably below the confirmed-failing size while still well
  // above the confirmed-working one.
  unsigned long forecast_min = sd_config.walk_time_min > 0 ? sd_config.walk_time_min + 15 : 10;
  if (forecast_min > 30) forecast_min = 30;

  char url[160];
  snprintf(url, sizeof(url),
           "https://transport.integration.sl.se/v1/sites/%s/departures?forecast=%lu",
           sd_config.sl_site_id.c_str(), forecast_min);

  // Field filter: only parse what we display. Keeps peak memory
  // bounded even at a busy stop with many departures, since fields we
  // don't need (journey, stop_area, stop_point, deviation scopes, ...)
  // are never allocated into `doc`.
  JsonDocument filter;
  filter["departures"][0]["destination"] = true;
  filter["departures"][0]["display"] = true;
  filter["departures"][0]["expected"] = true;
  filter["departures"][0]["line"]["designation"] = true;
  filter["departures"][0]["line"]["transport_mode"] = true;
  filter["departures"][0]["deviations"][0]["message"] = true;
  filter["departures"][0]["deviations"][0]["importance_level"] = true;
  filter["stop_deviations"][0]["message"] = true;
  filter["stop_deviations"][0]["importance_level"] = true;

  // The default 8s budget (http_get_json's default) is tuned for small
  // responses like weather's; a busy interchange can return 50+
  // departures (~30KB unfiltered) here, and on-device that download
  // plus a fresh TLS handshake each poll (no session reuse) can run
  // past 8s even though the API itself responds in well under a
  // second — confirmed by direct testing against the live SL API.
  JsonDocument doc;
  int status = 0;
  bool ok = http_get_json(url, ROOT_CA_SL, doc, 15000, &filter, "transit", &status);
  // ...except when SL is rate-limiting us (429): an immediate retry
  // only adds to the request count that got us limited. The fetch
  // worker backs off instead.
  if (!ok && status != HTTP_STATUS_TOO_MANY_REQUESTS) {
    // Observed on-device: HTTP 200 but a truncated body (IncompleteInput)
    // even with strong WiFi signal and plenty of free heap — a one-off
    // blip on the connection to SL, not a resource limit. Retry once
    // immediately rather than leaving stale data on screen for a full
    // poll interval.
    doc.clear();
    ok = http_get_json(url, ROOT_CA_SL, doc, 15000, &filter, "transit", &status);
  }

  if (ok) {
    DepartureList next;
    next.valid = true;
    next.fetched_at_ms = millis();

    for (JsonVariantConst dev : doc["stop_deviations"].as<JsonArrayConst>()) add_notice(next, dev);

    for (JsonObject dep : doc["departures"].as<JsonArray>()) {
      // Notices from every returned departure, not just the ones that
      // fit in MAX_DEPARTURES — a disruption on a later departure of
      // the same line is still worth knowing about.
      for (JsonVariantConst dev : dep["deviations"].as<JsonArrayConst>()) add_notice(next, dev);
      if (next.count >= MAX_DEPARTURES) continue;
      Departure &item = next.items[next.count];
      item.line = dep["line"]["designation"].as<const char *>();
      item.transport_mode = dep["line"]["transport_mode"].as<const char *>();
      item.destination = dep["destination"].as<const char *>();
      item.display = dep["display"].as<const char *>();
      item.expected_epoch = parse_sl_datetime(dep["expected"].as<const char *>());
      next.count++;
    }

    out = next;
  }
  if (http_status != nullptr) *http_status = status;

  Serial.printf("[transit] fetch %s, %u departures, %u notices, free heap: %u bytes\n",
                ok ? "ok" : "failed", ok ? (unsigned)out.count : 0u, ok ? (unsigned)out.notice_count : 0u,
                (unsigned)ESP.getFreeHeap());
  return ok;
}

bool transit_departure_is_reachable(const Departure &dep) {
  if (sd_config.walk_time_min == 0) return true;
  if (dep.expected_epoch == 0) return true;

  struct tm now_tm;
  if (!time_sync_get_local(now_tm)) return true;
  time_t now = mktime(&now_tm);

  long minutes_until = (long)(dep.expected_epoch - now) / 60;
  return minutes_until >= (long)sd_config.walk_time_min;
}
