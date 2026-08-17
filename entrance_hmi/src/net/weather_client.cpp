#include "weather_client.h"
#include <ArduinoJson.h>
#include <math.h>
#include "../../config.h"
#include "http_client_helper.h"
#include "wifi_manager.h"

WeatherData weather_data;

namespace {

constexpr unsigned long POLL_INTERVAL_MS = 10UL * 60 * 1000;  // 10 min
unsigned long last_fetch_ms = 0;
bool changed_this_fetch = false;

bool fetch_once() {
  char url[256];
  snprintf(url, sizeof(url),
           "https://api.openweathermap.org/data/2.5/weather?lat=%s&lon=%s&units=metric&appid=%s",
           OWM_LAT, OWM_LON, OWM_API_KEY);

  JsonDocument doc;
  bool ok = http_get_json(url, doc);
  changed_this_fetch = false;

  if (ok) {
    JsonArray weather_arr = doc["weather"].as<JsonArray>();
    if (weather_arr.isNull() || weather_arr.size() == 0) {
      ok = false;
    } else {
      WeatherData next;
      next.temp_c = doc["main"]["temp"] | 0.0f;
      next.feels_like_c = doc["main"]["feels_like"] | 0.0f;
      next.humidity = doc["main"]["humidity"] | 0;
      next.description = weather_arr[0]["description"].as<const char *>();
      next.icon = weather_arr[0]["icon"].as<const char *>();
      next.valid = true;
      next.fetched_at_ms = millis();

      changed_this_fetch = !weather_data.valid ||
                            fabsf(next.temp_c - weather_data.temp_c) >= 0.05f ||
                            next.humidity != weather_data.humidity ||
                            next.description != weather_data.description;

      weather_data = next;
    }
  }

  Serial.printf("[weather] fetch %s, free heap: %u bytes\n", ok ? "ok" : "failed",
                (unsigned)ESP.getFreeHeap());
  return ok;
}

}  // namespace

bool weather_client_tick() {
  if (!wifi_is_connected()) return false;
  unsigned long now = millis();
  if (now - last_fetch_ms < POLL_INTERVAL_MS) return false;
  last_fetch_ms = now;
  bool ok = fetch_once();
  return ok && changed_this_fetch;
}

bool weather_client_fetch_now() {
  if (!wifi_is_connected()) return false;
  last_fetch_ms = millis();
  return fetch_once();
}
