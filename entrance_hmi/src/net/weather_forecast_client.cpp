#include "weather_forecast_client.h"
#include <ArduinoJson.h>
#include <time.h>
#include "../storage/sd_config.h"
#include "http_client_helper.h"
#include "root_certs.h"
#include "wifi_manager.h"
#include "net_log_shadow.h"

ForecastData forecast_data;

bool weather_forecast_client_fetch(ForecastData &out, int *http_status) {
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
  bool ok = http_get_json(url, ROOT_CA_OPENWEATHERMAP, doc, 10000, &filter, "forecast", http_status);

  if (ok) {
    ForecastData next;
    next.valid = true;
    next.fetched_at_ms = millis();

    for (JsonObject entry : doc["list"].as<JsonArray>()) {
      if (next.count >= MAX_FORECAST_ENTRIES) break;
      ForecastEntry &item = next.items[next.count];

      time_t dt = (time_t)(entry["dt"] | 0);
      struct tm local_tm;
      localtime_r(&dt, &local_tm);  // _r: runs on the fetch worker task, not loop()
      item.hour = local_tm.tm_hour;
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
      out = next;
    }
  }

  Serial.printf("[forecast] fetch %s, %u entries, free heap: %u bytes\n", ok ? "ok" : "failed",
                ok ? (unsigned)out.count : 0u, (unsigned)ESP.getFreeHeap());
  return ok;
}

