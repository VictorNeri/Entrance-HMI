#include "weather_client.h"
#include <ArduinoJson.h>
#include <stdio.h>
#include <string.h>
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

namespace {

// Same "%.0f" the screens draw the temperature with, so the comparison
// below sees exactly the text that would change on screen — including
// printf's half-to-even rounding and the "-0" it prints for values in
// (-0.5, 0), which plain float rounding would treat as equal to 0.
bool same_rendered_temp(float a, float b) {
  char a_str[16], b_str[16];
  snprintf(a_str, sizeof(a_str), "%.0f", a);
  snprintf(b_str, sizeof(b_str), "%.0f", b);
  return strcmp(a_str, b_str) == 0;
}

}  // namespace

bool weather_is_windy(const WeatherData &data) {
  return data.valid && data.wind_speed_ms >= WEATHER_WINDY_THRESHOLD_MS;
}

bool weather_data_differs(const WeatherData &a, const WeatherData &b) {
  // Compares exactly what the screens render, nothing finer: the
  // temperature as its drawn text, the icon (flips day/night even when
  // the description doesn't), and wind only as the on/off "Windy"
  // alert it drives. feels_like isn't shown anywhere, so it's left out
  // on purpose.
  return a.valid != b.valid || !same_rendered_temp(a.temp_c, b.temp_c) || a.humidity != b.humidity ||
         a.description != b.description || a.icon != b.icon ||
         weather_is_windy(a) != weather_is_windy(b);
}
