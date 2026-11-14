#include "weather_client.h"
#include <ArduinoJson.h>
#include <math.h>
#include "../storage/sd_config.h"
#include "http_client_helper.h"
#include "wifi_manager.h"
#include "net_log_shadow.h"

WeatherData weather_data;

bool weather_client_fetch(WeatherData &out, int *http_status) {
  char url[256];
  snprintf(url, sizeof(url),
           "https://api.openweathermap.org/data/2.5/weather?lat=%s&lon=%s&units=metric&appid=%s",
           sd_config.owm_lat.c_str(), sd_config.owm_lon.c_str(), sd_config.owm_api_key.c_str());

  JsonDocument doc;
  bool ok = http_get_json(url, doc, 8000, nullptr, "weather", http_status);

  if (ok) {
    JsonArray weather_arr = doc["weather"].as<JsonArray>();
    if (weather_arr.isNull() || weather_arr.size() == 0) {
      ok = false;
    } else {
      WeatherData next;
      next.temp_c = doc["main"]["temp"] | 0.0f;
      next.feels_like_c = doc["main"]["feels_like"] | 0.0f;
      next.humidity = doc["main"]["humidity"] | 0;
      next.wind_speed_ms = doc["wind"]["speed"] | 0.0f;
      next.description = weather_arr[0]["description"].as<const char *>();
      next.icon = weather_arr[0]["icon"].as<const char *>();
      next.valid = true;
      next.fetched_at_ms = millis();
      out = next;
    }
  }

  Serial.printf("[weather] fetch %s, free heap: %u bytes\n", ok ? "ok" : "failed",
                (unsigned)ESP.getFreeHeap());
  return ok;
}

bool weather_data_differs(const WeatherData &a, const WeatherData &b) {
  return a.valid != b.valid || fabsf(a.temp_c - b.temp_c) >= 0.05f || a.humidity != b.humidity ||
         a.description != b.description;
}
