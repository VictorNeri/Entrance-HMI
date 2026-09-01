#include "screen_transit.h"
#include <stdio.h>
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"
#include "../net/transit_client.h"
#include "ui_chrome.h"
#include "ui_text_unicode.h"

namespace {

constexpr uint16_t PANEL_DIVIDER_X = 380;
constexpr uint16_t LEFT_PANEL_X = UI_CONTENT_LEFT;  // 20
constexpr uint16_t RIGHT_PANEL_X = 396;
constexpr uint8_t MAX_SHOWN = 3;

bool is_bus(const Departure &dep) {
  return dep.transport_mode == "BUS";
}

// Up to MAX_SHOWN soonest departures for one bucket. Everything non-BUS
// (METRO/TRAM/TRAIN/SHIP) buckets into the "TRAIN" panel — the simplest
// coherent bus-vs-everything-else split. Departures too soon to walk to
// (sd_config.walk_time_min) are skipped entirely, not just deprioritized.
uint8_t find_next_n(bool bus_bucket, const Departure *out[], uint8_t max_out) {
  uint8_t n = 0;
  for (uint8_t i = 0; i < departure_list.count && n < max_out; i++) {
    const Departure &dep = departure_list.items[i];
    if (is_bus(dep) != bus_bucket) continue;
    if (!transit_departure_is_reachable(dep)) continue;
    out[n++] = &dep;
  }
  return n;
}

// Boxy body, a front-cab divider, 2 window squares, 2 wheels — reads
// more like an actual bus silhouette than a plain divided rectangle.
void draw_bus_icon(uint16_t x, uint16_t y) {
  EPD_DrawRectangle(x, y, x + 28, y + 14, BLACK, 0);
  EPD_DrawLine(x + 22, y, x + 22, y + 14, BLACK);
  EPD_DrawRectangle(x + 3, y + 3, x + 9, y + 8, BLACK, 0);
  EPD_DrawRectangle(x + 12, y + 3, x + 18, y + 8, BLACK, 0);
  EPD_DrawCircle(x + 7, y + 16, 2, BLACK, 1);
  EPD_DrawCircle(x + 21, y + 16, 2, BLACK, 1);
}

// Pointed nose, a window strip, and 3 wheels (vs. the bus's 2) — reads
// more like a train silhouette and stays clearly distinct from the bus.
void draw_train_icon(uint16_t x, uint16_t y) {
  EPD_DrawRectangle(x + 6, y, x + 30, y + 14, BLACK, 0);
  EPD_DrawLine(x + 6, y, x, y + 7, BLACK);
  EPD_DrawLine(x, y + 7, x + 6, y + 14, BLACK);
  EPD_DrawRectangle(x + 10, y + 3, x + 26, y + 8, BLACK, 0);
  EPD_DrawCircle(x + 10, y + 16, 2, BLACK, 1);
  EPD_DrawCircle(x + 18, y + 16, 2, BLACK, 1);
  EPD_DrawCircle(x + 26, y + 16, 2, BLACK, 1);
}

void render_panel(uint16_t panel_x, const char *bucket_label,
                   void (*draw_icon)(uint16_t, uint16_t), bool bus_bucket) {
  draw_icon(panel_x, UI_CONTENT_BODY_TOP);
  EPD_ShowString(panel_x + 40, UI_CONTENT_BODY_TOP, bucket_label, 24, BLACK);

  const Departure *deps[MAX_SHOWN];
  uint8_t n = find_next_n(bus_bucket, deps, MAX_SHOWN);

  if (n == 0) {
    EPD_ShowString(panel_x, UI_CONTENT_BODY_TOP + 34, "No departures", 24, BLACK);
    return;
  }

  // Line + destination + (countdown). Destination and countdown are
  // each truncated independently before composing, rather than
  // truncating the whole assembled string — that way a long
  // destination can never eat into or cut off the "(countdown)" part,
  // which is the more time-critical piece of information. Worst case
  // (line<=5 + dest<=10 + countdown<=6 + separators/parens) is ~27
  // chars, ~324px at size 24 — safely inside both panels.
  for (uint8_t i = 0; i < n; i++) {
    char line_num[6];
    snprintf(line_num, sizeof(line_num), "%s", deps[i]->line.c_str());
    char dest[11];
    snprintf(dest, sizeof(dest), "%s", deps[i]->destination.c_str());
    char countdown[7];
    snprintf(countdown, sizeof(countdown), "%s", deps[i]->display.c_str());

    char row[40];
    snprintf(row, sizeof(row), "%s  %s  (%s)", line_num, dest, countdown);
    draw_utf8_string(panel_x, UI_CONTENT_BODY_TOP + 34 + i * 40, row, 24, BLACK);
  }
}

}  // namespace

void screen_transit_render() {
  if (!departure_list.valid || departure_list.count == 0) {
    EPD_ShowString(UI_CONTENT_LEFT, UI_CONTENT_BODY_TOP, "No departures yet - waiting for first fetch",
                   24, BLACK);
  } else {
    EPD_DrawLine(PANEL_DIVIDER_X, UI_CONTENT_BODY_TOP, PANEL_DIVIDER_X, UI_CONTENT_BOTTOM, BLACK);
    render_panel(LEFT_PANEL_X, "BUS", draw_bus_icon, true);
    render_panel(RIGHT_PANEL_X, "TRAIN", draw_train_icon, false);
  }

  EPD_ShowString(UI_CONTENT_LEFT, UI_CONTENT_HINT_Y, "OK: refresh   EXIT: home", 16, BLACK);
}
