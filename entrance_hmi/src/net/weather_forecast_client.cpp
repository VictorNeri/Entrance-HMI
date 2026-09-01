#include "weather_forecast_client.h"
#include <ArduinoJson.h>
#include <time.h>
#include "../storage/sd_config.h"
#include "http_client_helper.h"
#include "wifi_manager.h"

ForecastData forecast_data;

namespace {

constexpr unsigned long FORECAST_POLL_INTERVAL_MS = 30UL * 60 * 1000;  // 30 min
unsigned long last_fetch_ms = 0;

bool fetch_once() {
  // cnt=MAX_FORECAST_ENTRIES bounds the response to just the entries we
  // display (OWM's 3-hourly forecast, so 4 entries = next ~12h) —
  // keeping the payload small matters a lot on this device: an
  // oversized response on the transit fetch was previously confirmed
  // to fail mid-download on-device even though the API itself responds
  // fine, so this endpoint is deliberately kept minimal from the start.
  char url[256];
  snprintf(url, sizeof(url),
           "https://api.openweathermap.org/data/2.5/forecast?lat=%s&lon=%s&units=metric&appid=%s&cnt=%u",
           sd_config.owm_lat.c_str(), sd_config.owm_lon.c_str(), sd_config.owm_api_key.c_str(),
           (unsigned)MAX_FORECAST_ENTRIES);

  // Field filter: only parse what we display, same reasoning/pattern as
  // transit_client's filter.
  JsonDocument filter;
  filter["list"][0]["dt"] = true;
  filter["list"][0]["main"]["temp"] = true;
  filter["list"][0]["weather"][0]["icon"] = true;

  JsonDocument doc;
  bool ok = http_get_json(url, doc, 10000, &filter, "forecast");

  if (ok) {
    ForecastData next;
    next.valid = true;
    next.fetched_at_ms = millis();

    for (JsonObject entry : doc["list"].as<JsonArray>()) {
      if (next.count >= MAX_FORECAST_ENTRIES) break;
      ForecastEntry &item = next.items[next.count];

      time_t dt = (time_t)(entry["dt"] | 0);
      struct tm *local_tm = localtime(&dt);
      item.hour = local_tm->tm_hour;
      item.temp_c = entry["main"]["temp"] | 0.0f;
      JsonArray weather_arr = entry["weather"].as<JsonArray>();
      if (!weather_arr.isNull() && weather_arr.size() > 0) {
        item.icon = weather_arr[0]["icon"].as<const char *>();
      }
      next.count++;
    }

    if (next.count == 0) {
      ok = false;
    } else {
      forecast_data = next;
    }
  }

  Serial.printf("[forecast] fetch %s, %u entries, free heap: %u bytes\n", ok ? "ok" : "failed",
                (unsigned)forecast_data.count, (unsigned)ESP.getFreeHeap());
  return ok;
}

}  // namespace

bool weather_forecast_client_tick() {
  if (!wifi_is_connected()) return false;
  unsigned long now = millis();
  if (now - last_fetch_ms < FORECAST_POLL_INTERVAL_MS) return false;
  last_fetch_ms = now;
  return fetch_once();
}

bool weather_forecast_client_fetch_now() {
  if (!wifi_is_connected()) return false;
  last_fetch_ms = millis();
  return fetch_once();
}
