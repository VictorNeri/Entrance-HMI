#include "weather_icons.h"
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"

namespace {

// Scales an offset from center and truncates to int — simple
// truncation rather than round-to-nearest, matching how the rest of
// this file's hand-picked pixel offsets are already just "close
// enough" approximations, not precision geometry.
inline int16_t s(int v, float scale) {
  return (int16_t)(v * scale);
}

void draw_sun(uint16_t cx, uint16_t cy, float scale) {
  EPD_DrawCircle(cx, cy, s(5, scale), BLACK, 1);
  EPD_DrawLine(cx, cy - s(7, scale), cx, cy - s(10, scale), BLACK);
  EPD_DrawLine(cx, cy + s(7, scale), cx, cy + s(10, scale), BLACK);
  EPD_DrawLine(cx - s(7, scale), cy, cx - s(10, scale), cy, BLACK);
  EPD_DrawLine(cx + s(7, scale), cy, cx + s(10, scale), cy, BLACK);
  EPD_DrawLine(cx + s(5, scale), cy - s(5, scale), cx + s(8, scale), cy - s(8, scale), BLACK);
  EPD_DrawLine(cx - s(5, scale), cy - s(5, scale), cx - s(8, scale), cy - s(8, scale), BLACK);
  EPD_DrawLine(cx + s(5, scale), cy + s(5, scale), cx + s(8, scale), cy + s(8, scale), BLACK);
  EPD_DrawLine(cx - s(5, scale), cy + s(5, scale), cx - s(8, scale), cy + s(8, scale), BLACK);
}

// Crescent: a filled circle with a same-size WHITE circle erasing part
// of it — Paint_SetPixel treats non-BLACK as "clear to background", so
// this carves a silhouette rather than needing a dedicated arc primitive.
void draw_moon(uint16_t cx, uint16_t cy, float scale) {
  EPD_DrawCircle(cx, cy, s(7, scale), BLACK, 1);
  EPD_DrawCircle(cx + s(4, scale), cy - s(2, scale), s(7, scale), WHITE, 1);
}

// Base cloud silhouette, reused (shifted up) by rain/snow/thunder so
// there's room below for their own decoration.
void draw_cloud(uint16_t cx, uint16_t cy, float scale) {
  EPD_DrawCircle(cx - s(6, scale), cy, s(6, scale), BLACK, 1);
  EPD_DrawCircle(cx + s(2, scale), cy - s(2, scale), s(8, scale), BLACK, 1);
  EPD_DrawCircle(cx + s(9, scale), cy, s(6, scale), BLACK, 1);
  EPD_DrawRectangle(cx - s(10, scale), cy, cx + s(14, scale), cy + s(7, scale), BLACK, 1);
}

void draw_rain(uint16_t cx, uint16_t cy, float scale) {
  draw_cloud(cx, cy - s(4, scale), scale);
  EPD_DrawLine(cx - s(6, scale), cy + s(10, scale), cx - s(8, scale), cy + s(15, scale), BLACK);
  EPD_DrawLine(cx, cy + s(10, scale), cx - s(2, scale), cy + s(15, scale), BLACK);
  EPD_DrawLine(cx + s(6, scale), cy + s(10, scale), cx + s(4, scale), cy + s(15, scale), BLACK);
}

void draw_thunderstorm(uint16_t cx, uint16_t cy, float scale) {
  draw_cloud(cx, cy - s(4, scale), scale);
  EPD_DrawLine(cx, cy + s(8, scale), cx - s(4, scale), cy + s(14, scale), BLACK);
  EPD_DrawLine(cx - s(4, scale), cy + s(14, scale), cx + s(2, scale), cy + s(14, scale), BLACK);
  EPD_DrawLine(cx + s(2, scale), cy + s(14, scale), cx - s(2, scale), cy + s(20, scale), BLACK);
}

void draw_snow(uint16_t cx, uint16_t cy, float scale) {
  draw_cloud(cx, cy - s(4, scale), scale);
  int16_t step = s(6, scale);
  for (int16_t dx = -step; dx <= step; dx += step) {
    EPD_DrawLine(cx + dx - s(2, scale), cy + s(13, scale), cx + dx + s(2, scale), cy + s(13, scale), BLACK);
    EPD_DrawLine(cx + dx, cy + s(11, scale), cx + dx, cy + s(15, scale), BLACK);
  }
}

void draw_mist(uint16_t cx, uint16_t cy, float scale) {
  EPD_DrawLine(cx - s(10, scale), cy - s(6, scale), cx + s(10, scale), cy - s(6, scale), BLACK);
  EPD_DrawLine(cx - s(10, scale), cy, cx + s(10, scale), cy, BLACK);
  EPD_DrawLine(cx - s(10, scale), cy + s(6, scale), cx + s(10, scale), cy + s(6, scale), BLACK);
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

void draw_weather_icon(uint16_t cx, uint16_t cy, const String &owm_icon_code, float scale) {
  if (owm_icon_code.length() < 2) return;
  String code = owm_icon_code.substring(0, 2);
  bool night = owm_icon_code.endsWith("n");

  if (code == "01") {
    night ? draw_moon(cx, cy, scale) : draw_sun(cx, cy, scale);
  } else if (code == "02" || code == "03" || code == "04") {
    draw_cloud(cx, cy, scale);
  } else if (code == "09" || code == "10") {
    draw_rain(cx, cy, scale);
  } else if (code == "11") {
    draw_thunderstorm(cx, cy, scale);
  } else if (code == "13") {
    draw_snow(cx, cy, scale);
  } else if (code == "50") {
    draw_mist(cx, cy, scale);
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
