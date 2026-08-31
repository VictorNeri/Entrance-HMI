#include "transit_client.h"
#include <ArduinoJson.h>
#include "../storage/sd_config.h"
#include "http_client_helper.h"
#include "wifi_manager.h"

DepartureList departure_list;

namespace {

unsigned long last_fetch_ms = 0;

bool fetch_once() {
  // forecast=10 bounds the response to the next 10 minutes. We only
  // ever display the single soonest departure per mode, so a short
  // window is enough — and it matters a lot more than that: at a busy
  // interchange (verified against the live API: site 9109 returns
  // ~30KB/57 departures at forecast=60 vs. ~11KB/20 at forecast=10),
  // the larger response was consistently failing to fully download on
  // device (confirmed via serial: "HTTP 200 but JSON parse failed:
  // IncompleteInput" — the connection closes before the body finishes
  // arriving), causing every fetch at that stop to fail outright.
  char url[160];
  snprintf(url, sizeof(url),
           "https://transport.integration.sl.se/v1/sites/%s/departures?forecast=10",
           sd_config.sl_site_id.c_str());

  // Field filter: only parse what we display. Keeps peak memory
  // bounded even at a busy stop with many departures, since fields we
  // don't need (journey, stop_area, stop_point, deviations, ...) are
  // never allocated into `doc`.
  JsonDocument filter;
  filter["departures"][0]["destination"] = true;
  filter["departures"][0]["display"] = true;
  filter["departures"][0]["line"]["designation"] = true;
  filter["departures"][0]["line"]["transport_mode"] = true;

  // The default 8s budget (http_get_json's default) is tuned for small
  // responses like weather's; a busy interchange can return 50+
  // departures (~30KB unfiltered) here, and on-device that download
  // plus a fresh TLS handshake each poll (no session reuse) can run
  // past 8s even though the API itself responds in well under a
  // second — confirmed by direct testing against the live SL API.
  JsonDocument doc;
  bool ok = http_get_json(url, doc, 15000, &filter, "transit");

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
