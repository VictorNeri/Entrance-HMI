#include "screen_weather.h"
#include <stdio.h>
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"
#include "../net/weather_client.h"

void screen_weather_render() {
  EPD_ShowString(20, 20, "WEATHER", 48, BLACK);

  if (!weather_data.valid) {
    EPD_ShowString(20, 90, "No data yet - waiting for first fetch", 16, BLACK);
  } else {
    char line[64];
    snprintf(line, sizeof(line), "%.1f C  (feels like %.1f C)", weather_data.temp_c,
             weather_data.feels_like_c);
    EPD_ShowString(20, 90, line, 24, BLACK);

    EPD_ShowString(20, 130, weather_data.description.c_str(), 24, BLACK);

    snprintf(line, sizeof(line), "Humidity: %d%%", weather_data.humidity);
    EPD_ShowString(20, 170, line, 16, BLACK);
  }

  EPD_ShowString(20, 230, "PRV/NEXT: cycle screens   OK: refresh now", 12, BLACK);
}
