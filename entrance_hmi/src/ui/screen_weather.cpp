#include "screen_weather.h"
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"

void screen_weather_render() {
  EPD_ShowString(20, 20, "WEATHER", 48, BLACK);
  EPD_ShowString(20, 90, "Placeholder - live data starts at M3", 16, BLACK);
  EPD_ShowString(20, 230, "PRV/NEXT: cycle screens   EXIT: home", 12, BLACK);
}
