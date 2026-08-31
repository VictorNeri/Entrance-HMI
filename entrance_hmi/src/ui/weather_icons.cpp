#include "weather_icons.h"
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"

namespace {

void draw_sun(uint16_t cx, uint16_t cy) {
  EPD_DrawCircle(cx, cy, 5, BLACK, 1);
  EPD_DrawLine(cx, cy - 7, cx, cy - 10, BLACK);
  EPD_DrawLine(cx, cy + 7, cx, cy + 10, BLACK);
  EPD_DrawLine(cx - 7, cy, cx - 10, cy, BLACK);
  EPD_DrawLine(cx + 7, cy, cx + 10, cy, BLACK);
  EPD_DrawLine(cx + 5, cy - 5, cx + 8, cy - 8, BLACK);
  EPD_DrawLine(cx - 5, cy - 5, cx - 8, cy - 8, BLACK);
  EPD_DrawLine(cx + 5, cy + 5, cx + 8, cy + 8, BLACK);
  EPD_DrawLine(cx - 5, cy + 5, cx - 8, cy + 8, BLACK);
}

// Crescent: a filled circle with a same-size WHITE circle erasing part
// of it — Paint_SetPixel treats non-BLACK as "clear to background", so
// this carves a silhouette rather than needing a dedicated arc primitive.
void draw_moon(uint16_t cx, uint16_t cy) {
  EPD_DrawCircle(cx, cy, 7, BLACK, 1);
  EPD_DrawCircle(cx + 4, cy - 2, 7, WHITE, 1);
}

// Base cloud silhouette, reused (shifted up) by rain/snow/thunder so
// there's room below for their own decoration.
void draw_cloud(uint16_t cx, uint16_t cy) {
  EPD_DrawCircle(cx - 6, cy, 6, BLACK, 1);
  EPD_DrawCircle(cx + 2, cy - 2, 8, BLACK, 1);
  EPD_DrawCircle(cx + 9, cy, 6, BLACK, 1);
  EPD_DrawRectangle(cx - 10, cy, cx + 14, cy + 7, BLACK, 1);
}

void draw_rain(uint16_t cx, uint16_t cy) {
  draw_cloud(cx, cy - 4);
  EPD_DrawLine(cx - 6, cy + 10, cx - 8, cy + 15, BLACK);
  EPD_DrawLine(cx, cy + 10, cx - 2, cy + 15, BLACK);
  EPD_DrawLine(cx + 6, cy + 10, cx + 4, cy + 15, BLACK);
}

void draw_thunderstorm(uint16_t cx, uint16_t cy) {
  draw_cloud(cx, cy - 4);
  EPD_DrawLine(cx, cy + 8, cx - 4, cy + 14, BLACK);
  EPD_DrawLine(cx - 4, cy + 14, cx + 2, cy + 14, BLACK);
  EPD_DrawLine(cx + 2, cy + 14, cx - 2, cy + 20, BLACK);
}

void draw_snow(uint16_t cx, uint16_t cy) {
  draw_cloud(cx, cy - 4);
  for (int16_t dx = -6; dx <= 6; dx += 6) {
    EPD_DrawLine(cx + dx - 2, cy + 13, cx + dx + 2, cy + 13, BLACK);
    EPD_DrawLine(cx + dx, cy + 11, cx + dx, cy + 15, BLACK);
  }
}

void draw_mist(uint16_t cx, uint16_t cy) {
  EPD_DrawLine(cx - 10, cy - 6, cx + 10, cy - 6, BLACK);
  EPD_DrawLine(cx - 10, cy, cx + 10, cy, BLACK);
  EPD_DrawLine(cx - 10, cy + 6, cx + 10, cy + 6, BLACK);
}

void draw_mini_sun(uint16_t cx, uint16_t cy) {
  EPD_DrawCircle(cx, cy, 3, BLACK, 1);
  EPD_DrawLine(cx, cy - 4, cx, cy - 5, BLACK);
  EPD_DrawLine(cx, cy + 4, cx, cy + 5, BLACK);
  EPD_DrawLine(cx - 4, cy, cx - 5, cy, BLACK);
  EPD_DrawLine(cx + 4, cy, cx + 5, cy, BLACK);
}

void draw_mini_moon(uint16_t cx, uint16_t cy) {
  EPD_DrawCircle(cx, cy, 3, BLACK, 1);
  EPD_DrawCircle(cx + 2, cy - 1, 3, WHITE, 1);
}

void draw_mini_cloud(uint16_t cx, uint16_t cy) {
  EPD_DrawCircle(cx - 2, cy, 3, BLACK, 1);
  EPD_DrawCircle(cx + 2, cy - 1, 3, BLACK, 1);
  EPD_DrawRectangle(cx - 4, cy, cx + 5, cy + 3, BLACK, 1);
}

void draw_mini_rain(uint16_t cx, uint16_t cy) {
  draw_mini_cloud(cx, cy - 2);
  EPD_DrawLine(cx - 2, cy + 4, cx - 3, cy + 6, BLACK);
  EPD_DrawLine(cx + 2, cy + 4, cx + 1, cy + 6, BLACK);
}

void draw_mini_thunder(uint16_t cx, uint16_t cy) {
  draw_mini_cloud(cx, cy - 2);
  EPD_DrawLine(cx, cy + 3, cx - 2, cy + 6, BLACK);
  EPD_DrawLine(cx - 2, cy + 6, cx + 1, cy + 6, BLACK);
  EPD_DrawLine(cx + 1, cy + 6, cx - 1, cy + 9, BLACK);
}

void draw_mini_snow(uint16_t cx, uint16_t cy) {
  draw_mini_cloud(cx, cy - 2);
  EPD_DrawCircle(cx - 2, cy + 5, 1, BLACK, 1);
  EPD_DrawCircle(cx + 2, cy + 5, 1, BLACK, 1);
}

void draw_mini_mist(uint16_t cx, uint16_t cy) {
  EPD_DrawLine(cx - 5, cy - 2, cx + 5, cy - 2, BLACK);
  EPD_DrawLine(cx - 5, cy + 2, cx + 5, cy + 2, BLACK);
}

}  // namespace

void draw_weather_icon(uint16_t cx, uint16_t cy, const String &owm_icon_code) {
  if (owm_icon_code.length() < 2) return;
  String code = owm_icon_code.substring(0, 2);
  bool night = owm_icon_code.endsWith("n");

  if (code == "01") {
    night ? draw_moon(cx, cy) : draw_sun(cx, cy);
  } else if (code == "02" || code == "03" || code == "04") {
    draw_cloud(cx, cy);
  } else if (code == "09" || code == "10") {
    draw_rain(cx, cy);
  } else if (code == "11") {
    draw_thunderstorm(cx, cy);
  } else if (code == "13") {
    draw_snow(cx, cy);
  } else if (code == "50") {
    draw_mist(cx, cy);
  }
}

void draw_weather_icon_mini(uint16_t cx, uint16_t cy, const String &owm_icon_code) {
  if (owm_icon_code.length() < 2) return;
  String code = owm_icon_code.substring(0, 2);
  bool night = owm_icon_code.endsWith("n");

  if (code == "01") {
    night ? draw_mini_moon(cx, cy) : draw_mini_sun(cx, cy);
  } else if (code == "02" || code == "03" || code == "04") {
    draw_mini_cloud(cx, cy);
  } else if (code == "09" || code == "10") {
    draw_mini_rain(cx, cy);
  } else if (code == "11") {
    draw_mini_thunder(cx, cy);
  } else if (code == "13") {
    draw_mini_snow(cx, cy);
  } else if (code == "50") {
    draw_mini_mist(cx, cy);
  }
}
