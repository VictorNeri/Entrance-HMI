#include "screen_night.h"
#include <stdio.h>
#include <string.h>
#include "../epd_driver/EPD.h"
#include "../storage/sd_config.h"

namespace {

constexpr uint16_t CANVAS_W = 792;

void draw_centered(uint16_t y, const char *text, uint16_t size) {
  uint16_t w = strlen(text) * (size / 2);
  EPD_ShowString((CANVAS_W - w) / 2, y, text, size, BLACK);
}

// Crescent moon: a filled disc with an offset white disc cut out of it.
void draw_moon(uint16_t cx, uint16_t cy, uint16_t r) {
  EPD_DrawCircle(cx, cy, r, BLACK, 1);
  EPD_DrawCircle(cx + r / 2, cy - r / 3, r - 2, WHITE, 1);
}

}  // namespace

void screen_night_render() {
  draw_moon(CANVAS_W / 2 - 4, 62, 26);
  draw_centered(110, "Night mode", 48);

  char resumes[40];
  snprintf(resumes, sizeof(resumes), "Display resumes at %02u:00", (unsigned)sd_config.night_end_hour);
  draw_centered(176, resumes, 24);
  draw_centered(214, "Press any button to wake", 16);
}
