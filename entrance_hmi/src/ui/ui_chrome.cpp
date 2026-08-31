#include "ui_chrome.h"
#include <WiFi.h>
#include <stdio.h>
#include <string.h>
#include "../app/app_state.h"
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"
#include "../net/mqtt_client.h"
#include "../net/time_sync.h"
#include "../net/wifi_manager.h"
#include "../storage/calendar_store.h"

namespace {

constexpr uint16_t CANVAS_RIGHT = 792;

const char *const WEEKDAY_NAMES[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *const MONTH_NAMES[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// Classic "signal bars" indicator: 4 bars of increasing height, filled
// up to the current signal strength, outline beyond it. All 4 outline
// (level 0) when WiFi isn't connected at all.
void draw_wifi_signal(uint16_t cx, uint16_t cy) {
  constexpr uint8_t BAR_W = 4, GAP = 2;
  constexpr int16_t HEIGHTS[4] = {5, 9, 13, 17};
  constexpr int16_t BASELINE_OFFSET = 9;

  uint8_t level = 0;
  if (wifi_is_connected()) {
    int32_t rssi = WiFi.RSSI();
    if (rssi >= -50) level = 4;
    else if (rssi >= -60) level = 3;
    else if (rssi >= -70) level = 2;
    else level = 1;
  }

  int16_t baseline = cy + BASELINE_OFFSET;
  int16_t start_x = cx - 11;
  for (uint8_t i = 0; i < 4; i++) {
    int16_t x0 = start_x + i * (BAR_W + GAP);
    int16_t y0 = baseline - HEIGHTS[i];
    EPD_DrawRectangle(x0, y0, x0 + BAR_W, baseline, BLACK, i < level ? 1 : 0);
  }
}

// Solid filled circle when MQTT is connected and syncing; a crossed-out
// hollow circle otherwise (covers both "WiFi is down" and "WiFi is up
// but the broker isn't reachable/rejecting us" — both show the same
// "not synced" state here, since either way HA control data isn't
// flowing).
void draw_mqtt_status(uint16_t cx, uint16_t cy) {
  bool connected = wifi_is_connected() && mqtt_is_connected();
  if (connected) {
    EPD_DrawCircle(cx, cy, 7, BLACK, 1);
  } else {
    EPD_DrawCircle(cx, cy, 7, BLACK, 0);
    EPD_DrawLine(cx - 5, cy - 5, cx + 5, cy + 5, BLACK);
    EPD_DrawLine(cx - 5, cy + 5, cx + 5, cy - 5, BLACK);
  }
}

void render_hero() {
  struct tm now;
  bool synced = time_sync_get_local(now);

  char time_str[6];
  if (synced) {
    snprintf(time_str, sizeof(time_str), "%02d:%02d", now.tm_hour, now.tm_min);
  } else {
    strncpy(time_str, "--:--", sizeof(time_str));
  }
  EPD_ShowString(UI_CONTENT_LEFT, UI_HERO_TIME_Y, time_str, 48, BLACK);

  if (synced) {
    char date_str[16];
    snprintf(date_str, sizeof(date_str), "%s %d %s", WEEKDAY_NAMES[now.tm_wday], now.tm_mday,
              MONTH_NAMES[now.tm_mon]);
    EPD_ShowString(UI_HERO_DATE_X, UI_HERO_DATE_Y, date_str, 24, BLACK);
  }

  // WiFi signal strength and MQTT sync status, after the date.
  draw_wifi_signal(330, 28);
  draw_mqtt_status(380, 28);

  // Pending-events indicator: always drawn (filled when there's something
  // pending, hollow otherwise) so it's a fixed, learnable anchor point
  // rather than appearing/disappearing. Positioned well clear of the
  // rail (starts at UI_RAIL_X0=752).
  constexpr uint16_t DOT_CX = 712, DOT_CY = 26, DOT_R = 8;
  uint8_t pending = calendar_pending_count();
  if (pending > 0) {
    char count_str[4];
    snprintf(count_str, sizeof(count_str), "%u", (unsigned)pending);
    uint16_t text_w = strlen(count_str) * 12;  // size-24 glyph width
    EPD_ShowString(DOT_CX - DOT_R - 6 - text_w, DOT_CY - 12, count_str, 24, BLACK);
    EPD_DrawCircle(DOT_CX, DOT_CY, DOT_R, BLACK, 1);
  } else {
    EPD_DrawCircle(DOT_CX, DOT_CY, DOT_R, BLACK, 0);
  }

  // Stops at the rail's own vertical divider rather than crossing all
  // the way to the edge — the rail spans the full height now, so a
  // full-width line here would cut straight through the HOME button's
  // icon/label in the rail's first cell.
  EPD_DrawLine(0, UI_HERO_DIVIDER_Y, UI_RAIL_X0 - 1, UI_HERO_DIVIDER_Y, BLACK);
}

void draw_home_icon(uint16_t cx, uint16_t cy) {
  EPD_DrawLine(cx - 10, cy + 2, cx, cy - 8, BLACK);
  EPD_DrawLine(cx, cy - 8, cx + 10, cy + 2, BLACK);
  EPD_DrawRectangle(cx - 7, cy + 2, cx + 7, cy + 10, BLACK, 0);
  EPD_DrawRectangle(cx - 2, cy + 4, cx + 2, cy + 10, BLACK, 1);  // door
}

// A proper arrow (shaft + head), not just an open triangle — clearer
// at a glance for "previous/scroll up."
void draw_up_arrow_icon(uint16_t cx, uint16_t cy) {
  EPD_DrawLine(cx, cy - 8, cx, cy + 8, BLACK);
  EPD_DrawLine(cx, cy - 8, cx - 6, cy - 2, BLACK);
  EPD_DrawLine(cx, cy - 8, cx + 6, cy - 2, BLACK);
}

void draw_down_arrow_icon(uint16_t cx, uint16_t cy) {
  EPD_DrawLine(cx, cy - 8, cx, cy + 8, BLACK);
  EPD_DrawLine(cx, cy + 8, cx - 6, cy + 2, BLACK);
  EPD_DrawLine(cx, cy + 8, cx + 6, cy + 2, BLACK);
}

// Classic "exit/logout" glyph — an open door frame with an arrow
// pointing out of it — rather than a bare arrow with no context.
void draw_exit_icon(uint16_t cx, uint16_t cy) {
  EPD_DrawLine(cx - 6, cy - 8, cx - 6, cy + 8, BLACK);
  EPD_DrawLine(cx - 6, cy - 8, cx + 2, cy - 8, BLACK);
  EPD_DrawLine(cx - 6, cy + 8, cx + 2, cy + 8, BLACK);
  EPD_DrawLine(cx - 4, cy, cx + 8, cy, BLACK);
  EPD_DrawLine(cx + 8, cy, cx + 3, cy - 4, BLACK);
  EPD_DrawLine(cx + 8, cy, cx + 3, cy + 4, BLACK);
}

void render_rail_cell(uint16_t cell_top, uint16_t cell_height, const char *label,
                       void (*draw_icon)(uint16_t, uint16_t)) {
  uint16_t cx = (UI_RAIL_X0 + UI_RAIL_X1) / 2;
  uint16_t cy = cell_top + cell_height / 2 - 6;
  draw_icon(cx, cy);

  // Size-16 glyph width is 8px/char, height is exactly 16 (no rounding
  // surprise like size-12 has — confirmed against EPD_ShowChar's
  // ceil(size1/8)*8 row math).
  uint16_t label_w = strlen(label) * 8;
  uint16_t label_x = UI_RAIL_X0 + (UI_RAIL_X1 - UI_RAIL_X0 - label_w) / 2;
  EPD_ShowString(label_x, cell_top + cell_height - 18, label, 16, BLACK);
}

void render_button_rail() {
  // Divider sits at the rail's left edge, separating it from content.
  EPD_DrawLine(UI_RAIL_X0, 0, UI_RAIL_X0, UI_FOOTER_Y0 - 1, BLACK);

  constexpr uint16_t RAIL_TOP = 0;
  constexpr uint16_t RAIL_HEIGHT = UI_FOOTER_Y0 - RAIL_TOP;  // 252
  constexpr uint16_t CELL_H = RAIL_HEIGHT / 4;                // 63

  render_rail_cell(RAIL_TOP + 0 * CELL_H, CELL_H, "HOME", draw_home_icon);
  render_rail_cell(RAIL_TOP + 1 * CELL_H, CELL_H, "PRV", draw_up_arrow_icon);
  render_rail_cell(RAIL_TOP + 2 * CELL_H, CELL_H, "NEXT", draw_down_arrow_icon);
  render_rail_cell(RAIL_TOP + 3 * CELL_H, CELL_H, "EXIT", draw_exit_icon);
}

void render_footer() {
  EPD_DrawLine(0, UI_FOOTER_Y0, CANVAS_RIGHT - 1, UI_FOOTER_Y0, BLACK);

  // Footer band is 20px; size-16 glyph height is exactly 16, so +2
  // centers it with a 2px margin top and bottom.
  constexpr uint16_t LABEL_Y = UI_FOOTER_Y0 + 2;

  uint16_t cell_w = CANVAS_RIGHT / SCREEN_TABLE_LEN;
  for (size_t i = 0; i < SCREEN_TABLE_LEN; i++) {
    uint16_t x0 = i * cell_w;
    uint16_t x1 = (i == SCREEN_TABLE_LEN - 1) ? CANVAS_RIGHT : (i + 1) * cell_w;
    const char *label = SCREEN_TABLE[i].label;
    uint16_t label_w = strlen(label) * 8;  // size-16 glyph width
    uint16_t label_x = x0 + (x1 - x0 - label_w) / 2;

    bool current = SCREEN_TABLE[i].screen == app_state.current_screen;
    if (current) {
      EPD_DrawRectangle(x0 + 1, UI_FOOTER_Y0 + 1, x1 - 1, UI_FOOTER_Y1 - 1, BLACK, 1);
      EPD_ShowString(label_x, LABEL_Y, label, 16, WHITE);
    } else {
      EPD_ShowString(label_x, LABEL_Y, label, 16, BLACK);
    }
    if (i > 0) EPD_DrawLine(x0, UI_FOOTER_Y0, x0, UI_FOOTER_Y1 - 1, BLACK);
  }
}

}  // namespace

void ui_chrome_render() {
  render_hero();
  render_button_rail();
  render_footer();
}
