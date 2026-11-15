#include "screen_transit.h"
#include <stdio.h>
#include <string.h>
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"
#include "../net/transit_client.h"
#include "ui_chrome.h"
#include "ui_stale.h"
#include "ui_text_unicode.h"

namespace {

constexpr uint16_t PANEL_DIVIDER_X = 380;
constexpr uint16_t LEFT_PANEL_X = UI_CONTENT_LEFT;  // 20
constexpr uint16_t RIGHT_PANEL_X = 396;
constexpr uint8_t MAX_SHOWN = 3;

// Disruption-notice strip: when SL reports any, each panel gives up its
// third departure row (rows end at y=164) so notices get the band
// between the panels and the hint line — up to NOTICE_MAX_LINES at
// size 16 (8px/char, so ~90 chars per line across the content width).
constexpr uint8_t MAX_SHOWN_WITH_NOTICES = 2;
constexpr uint16_t NOTICE_DIVIDER_Y = 168;
constexpr uint16_t NOTICE_TOP_Y = 174;
constexpr uint16_t NOTICE_LINE_H = 17;
constexpr uint8_t NOTICE_MAX_LINES = 3;
constexpr uint8_t NOTICE_FONT = 16;
constexpr size_t NOTICE_LINE_CHARS = (UI_CONTENT_RIGHT - UI_CONTENT_LEFT) / (NOTICE_FONT / 2);

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

// Word-wraps `text` (ASCII) into at most `max_lines` lines of
// NOTICE_LINE_CHARS starting at line `first_line`; a word longer than
// a line is hard-split. If the text doesn't fit, the last line ends in
// "...". Returns the number of lines drawn.
uint8_t draw_wrapped(const char *text, uint8_t first_line, uint8_t max_lines) {
  uint8_t lines = 0;
  const char *p = text;

  while (*p != '\0' && lines < max_lines) {
    while (*p == ' ') p++;
    if (*p == '\0') break;

    size_t remaining = strlen(p);
    size_t take = remaining;
    if (remaining > NOTICE_LINE_CHARS) {
      take = NOTICE_LINE_CHARS;
      while (take > 0 && p[take] != ' ') take--;  // break at the last space that fits
      if (take == 0) take = NOTICE_LINE_CHARS;
    }

    char line[NOTICE_LINE_CHARS + 1];
    bool last_line = lines + 1 == max_lines;
    if (last_line && take < remaining) {
      // Out of room: cut to make space for the ellipsis.
      size_t keep = take;
      if (keep > NOTICE_LINE_CHARS - 3) keep = NOTICE_LINE_CHARS - 3;
      while (keep > 0 && p[keep - 1] == ' ') keep--;
      memcpy(line, p, keep);
      memcpy(line + keep, "...", 4);
    } else {
      memcpy(line, p, take);
      line[take] = '\0';
    }

    EPD_ShowString(UI_CONTENT_LEFT, NOTICE_TOP_Y + (first_line + lines) * NOTICE_LINE_H, line, NOTICE_FONT,
                   BLACK);
    lines++;
    p += take;
  }
  return lines;
}

// Notices most-important first, each starting on a new line with a
// "! " marker; whatever doesn't fit in NOTICE_MAX_LINES is truncated.
void render_notices() {
  EPD_DrawLine(UI_CONTENT_LEFT, NOTICE_DIVIDER_Y, UI_CONTENT_RIGHT, NOTICE_DIVIDER_Y, BLACK);

  uint8_t used = 0;
  for (uint8_t i = 0; i < departure_list.notice_count && used < NOTICE_MAX_LINES; i++) {
    char text[MAX_TRANSIT_NOTICE_LEN + 3];
    text[0] = '!';
    text[1] = ' ';
    utf8_to_ascii(departure_list.notices[i].message.c_str(), text + 2, sizeof(text) - 2);
    used += draw_wrapped(text, used, NOTICE_MAX_LINES - used);
  }
}

void render_panel(uint16_t panel_x, const char *bucket_label,
                   void (*draw_icon)(uint16_t, uint16_t), bool bus_bucket, uint8_t max_shown) {
  draw_icon(panel_x, UI_CONTENT_BODY_TOP);
  EPD_ShowString(panel_x + 40, UI_CONTENT_BODY_TOP, bucket_label, 24, BLACK);

  const Departure *deps[MAX_SHOWN];
  uint8_t n = find_next_n(bus_bucket, deps, max_shown);

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
    bool notices = departure_list.notice_count > 0;
    uint8_t max_shown = notices ? MAX_SHOWN_WITH_NOTICES : MAX_SHOWN;
    EPD_DrawLine(PANEL_DIVIDER_X, UI_CONTENT_BODY_TOP, PANEL_DIVIDER_X, notices ? NOTICE_DIVIDER_Y : UI_CONTENT_BOTTOM,
                 BLACK);
    render_panel(LEFT_PANEL_X, "BUS", draw_bus_icon, true, max_shown);
    render_panel(RIGHT_PANEL_X, "TRAIN", draw_train_icon, false, max_shown);
    if (notices) render_notices();
  }

  EPD_ShowString(UI_CONTENT_LEFT, UI_CONTENT_HINT_Y, "OK: refresh   EXIT: home", 16, BLACK);

  // One fetch feeds both panels, so one badge — right-aligned on the
  // hint line (bottom edge on UI_CONTENT_BOTTOM) rather than repeated in
  // each panel heading. Drawn whenever a fetch has ever succeeded,
  // including when it returned no departures.
  if (departure_list.valid) {
    unsigned long stale_after = ui_transit_stale_after_ms();
    uint16_t w = ui_stale_badge_width(departure_list.fetched_at_ms, stale_after);
    if (w > 0) {
      ui_draw_stale_badge(UI_CONTENT_RIGHT - w, UI_CONTENT_BOTTOM - UI_STALE_BADGE_H, departure_list.fetched_at_ms,
                          stale_after);
    }
  }
}
