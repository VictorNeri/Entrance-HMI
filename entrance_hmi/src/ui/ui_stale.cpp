#include "ui_stale.h"
#include <stdio.h>
#include <string.h>
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"
#include "../net/weather_forecast_client.h"
#include "../storage/sd_config.h"

namespace {

constexpr uint16_t FONT_SIZE = 16;
constexpr uint16_t CHAR_W = FONT_SIZE / 2;  // size-16 glyph advance = 8px/char
constexpr uint16_t PAD_X = 4;
constexpr uint16_t PAD_Y = (UI_STALE_BADGE_H - FONT_SIZE) / 2;  // 2px

// Writes e.g. "25m ago" / "3h ago" / "2d ago" into `out` and returns
// true if stale; false (out untouched) if fresh.
bool format_age(unsigned long fetched_at_ms, unsigned long stale_after_ms, char *out,
                size_t out_len) {
  // Wrap-safe: unsigned subtraction stays correct across the ~49.7-day
  // millis() rollover.
  unsigned long age_ms = millis() - fetched_at_ms;
  if (age_ms <= stale_after_ms) return false;

  unsigned long minutes = age_ms / 60000UL;
  if (minutes < 60) {
    snprintf(out, out_len, "%lum ago", minutes);
  } else if (minutes < 48UL * 60) {
    snprintf(out, out_len, "%luh ago", minutes / 60);
  } else {
    snprintf(out, out_len, "%lud ago", minutes / (24UL * 60));
  }
  return true;
}

}  // namespace

unsigned long ui_weather_stale_after_ms() {
  return 3 * sd_config.weather_poll_interval_ms;
}

unsigned long ui_forecast_stale_after_ms() {
  return 3 * FORECAST_POLL_INTERVAL_MS;
}

unsigned long ui_transit_stale_after_ms() {
  return 2 * sd_config.transit_poll_background_ms;
}

uint16_t ui_stale_badge_width(unsigned long fetched_at_ms, unsigned long stale_after_ms) {
  char text[16];
  if (!format_age(fetched_at_ms, stale_after_ms, text, sizeof(text))) return 0;
  return strlen(text) * CHAR_W + 2 * PAD_X;
}

uint16_t ui_draw_stale_badge(uint16_t x, uint16_t y, unsigned long fetched_at_ms,
                             unsigned long stale_after_ms) {
  char text[16];
  if (!format_age(fetched_at_ms, stale_after_ms, text, sizeof(text))) return 0;

  uint16_t w = strlen(text) * CHAR_W + 2 * PAD_X;
  EPD_DrawRectangle(x, y, x + w - 1, y + UI_STALE_BADGE_H - 1, BLACK, 1);
  EPD_ShowString(x + PAD_X, y + PAD_Y, text, FONT_SIZE, WHITE);
  return w;
}
