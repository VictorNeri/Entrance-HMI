#include "transit_client.h"
#include <ArduinoJson.h>
#include "../../config.h"
#include "http_client_helper.h"
#include "wifi_manager.h"

DepartureList departure_list;

namespace {

unsigned long last_fetch_ms = 0;

bool fetch_once() {
  // forecast=60 bounds the response to the next hour — SL's default
  // window can otherwise return dozens of departures at a busy hub
  // (verified: ~40KB unfiltered JSON), which is more than this device
  // needs to hold in memory at once.
  char url[160];
  snprintf(url, sizeof(url),
           "https://transport.integration.sl.se/v1/sites/%s/departures?forecast=60", SL_SITE_ID);

  // Field filter: only parse what we display. Keeps peak memory
  // bounded even at a busy stop with many departures, since fields we
  // don't need (journey, stop_area, stop_point, deviations, ...) are
  // never allocated into `doc`.
  JsonDocument filter;
  filter["departures"][0]["destination"] = true;
  filter["departures"][0]["display"] = true;
  filter["departures"][0]["line"]["designation"] = true;
  filter["departures"][0]["line"]["transport_mode"] = true;

  JsonDocument doc;
  bool ok = http_get_json(url, doc, 8000, &filter);

  if (ok) {
    DepartureList next;
    next.valid = true;
    next.fetched_at_ms = millis();

    for (JsonObject dep : doc["departures"].as<JsonArray>()) {
      if (next.count >= MAX_DEPARTURES) break;
      Departure &item = next.items[next.count];
      item.line = dep["line"]["designation"].as<const char *>();
      item.transport_mode = dep["line"]["transport_mode"].as<const char *>();
      item.destination = dep["destination"].as<const char *>();
      item.display = dep["display"].as<const char *>();
      next.count++;
    }

    departure_list = next;
  }

  Serial.printf("[transit] fetch %s, %u departures, free heap: %u bytes\n", ok ? "ok" : "failed",
                (unsigned)departure_list.count, (unsigned)ESP.getFreeHeap());
  return ok;
}

}  // namespace

bool transit_client_tick(unsigned long poll_interval_ms) {
  if (!wifi_is_connected()) return false;
  unsigned long now = millis();
  if (now - last_fetch_ms < poll_interval_ms) return false;
  last_fetch_ms = now;
  return fetch_once();
}

bool transit_client_fetch_now() {
  if (!wifi_is_connected()) return false;
  last_fetch_ms = millis();
  return fetch_once();
}
