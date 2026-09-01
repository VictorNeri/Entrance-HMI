#include "screen_home.h"
#include <stdio.h>
#include <time.h>
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"
#include "../net/time_sync.h"
#include "../net/transit_client.h"
#include "../net/weather_client.h"
#include "../storage/calendar_store.h"
#include "ui_chrome.h"
#include "ui_text_unicode.h"
#include "weather_icons.h"

namespace {

// Three vertical columns (not horizontal bands) so each section gets
// the full content height to spread its info across, rather than a
// cramped single row. Geometry shared with WEATHER's 3-column layout
// via ui_chrome.h.
constexpr uint16_t COL1_X = UI_COL1_X;
constexpr uint16_t COL2_X = UI_COL2_X;
constexpr uint16_t COL3_X = UI_COL3_X;
constexpr uint16_t DIVIDER1_X = UI_COL_DIVIDER1_X;
constexpr uint16_t DIVIDER2_X = UI_COL_DIVIDER2_X;

// Soonest not-yet-ended event, mirroring calendar_pending_count()'s
// "pending" definition. Returns nullptr if there's nothing pending or
// local time isn't synced yet (can't safely compare).
const CalendarEvent *find_next_event() {
  if (!calendar_event_list.valid || calendar_event_list.count == 0) return nullptr;

  struct tm now_tm;
  if (!time_sync_get_local(now_tm)) return nullptr;
  time_t now = mktime(&now_tm);

  const CalendarEvent *best = nullptr;
  for (uint8_t i = 0; i < calendar_event_list.count; i++) {
    const CalendarEvent &ev = calendar_event_list.items[i];
    time_t effective_end = ev.end_epoch != 0 ? ev.end_epoch : ev.start_epoch;
    if (effective_end < now) continue;
    if (best == nullptr || ev.start_epoch < best->start_epoch) best = &ev;
  }
  return best;
}

// Soonest non-BUS departure (METRO/TRAM/TRAIN/SHIP) — same "everything
// but bus" bucket TRANSIT's train panel uses, since this line is meant
// to answer "when's my train," not duplicate the bus panel too.
const Departure *find_next_train() {
  for (uint8_t i = 0; i < departure_list.count; i++) {
    const Departure &dep = departure_list.items[i];
    if (dep.transport_mode == "BUS") continue;
    if (!transit_departure_is_reachable(dep)) continue;
    return &dep;
  }
  return nullptr;
}

void render_weather_column() {
  EPD_ShowString(COL1_X, UI_CONTENT_BODY_TOP, "WEATHER", 24, BLACK);

  if (!weather_data.valid) {
    EPD_ShowString(COL1_X, UI_CONTENT_BODY_TOP + 34, "Waiting for", 24, BLACK);
    EPD_ShowString(COL1_X, UI_CONTENT_BODY_TOP + 66, "first fetch", 24, BLACK);
    return;
  }

  draw_weather_icon(COL1_X + 18, UI_CONTENT_BODY_TOP + 55, weather_data.icon);

  char line[48];
  snprintf(line, sizeof(line), "%.0f C", weather_data.temp_c);
  EPD_ShowString(COL1_X + 44, UI_CONTENT_BODY_TOP + 32, line, 48, BLACK);

  // OWM's description text is normally short ("clear sky") but some
  // are longer ("light intensity drizzle" is a real one) — at size 24
  // this column is only ~18 chars wide, so cap it rather than let it
  // run into the next column.
  char desc[19];
  snprintf(desc, sizeof(desc), "%s", weather_data.description.c_str());
  EPD_ShowString(COL1_X, UI_CONTENT_BODY_TOP + 88, desc, 24, BLACK);

  snprintf(line, sizeof(line), "Humidity: %d%%", weather_data.humidity);
  EPD_ShowString(COL1_X, UI_CONTENT_BODY_TOP + 120, line, 24, BLACK);
}

void render_event_column() {
  EPD_ShowString(COL2_X, UI_CONTENT_BODY_TOP, "NEXT EVENT", 24, BLACK);

  const CalendarEvent *ev = find_next_event();
  if (ev == nullptr) {
    EPD_ShowString(COL2_X, UI_CONTENT_BODY_TOP + 34, "No events", 24, BLACK);
    EPD_ShowString(COL2_X, UI_CONTENT_BODY_TOP + 66, "today", 24, BLACK);
    return;
  }

  struct tm *event_tm = localtime(&ev->start_epoch);
  char line[32];
  snprintf(line, sizeof(line), "%02d:%02d", event_tm->tm_hour, event_tm->tm_min);
  EPD_ShowString(COL2_X, UI_CONTENT_BODY_TOP + 32, line, 48, BLACK);

  // Event titles come from the calendar MQTT payload with no length
  // cap — truncate defensively, same reasoning as the other columns.
  char title[19];
  snprintf(title, sizeof(title), "%s", ev->title.c_str());
  draw_utf8_string(COL2_X, UI_CONTENT_BODY_TOP + 88, title, 24, BLACK);
}

void render_train_column() {
  EPD_ShowString(COL3_X, UI_CONTENT_BODY_TOP, "NEXT TRAIN", 24, BLACK);

  const Departure *dep = find_next_train();
  if (!departure_list.valid || dep == nullptr) {
    EPD_ShowString(COL3_X, UI_CONTENT_BODY_TOP + 34, "No trains", 24, BLACK);
    return;
  }

  char line[32];
  snprintf(line, sizeof(line), "Line %s", dep->line.c_str());
  EPD_ShowString(COL3_X, UI_CONTENT_BODY_TOP + 32, line, 24, BLACK);

  // SL's `display` field is normally short ("Nu", "5 min") but is
  // external API text with no length guarantee — at size 48 (24px/
  // char) this column only has ~9 safe chars before running toward the
  // rail, so cap it defensively.
  char countdown[10];
  snprintf(countdown, sizeof(countdown), "%s", dep->display.c_str());
  EPD_ShowString(COL3_X, UI_CONTENT_BODY_TOP + 64, countdown, 48, BLACK);

  char destination[19];
  snprintf(destination, sizeof(destination), "%s", dep->destination.c_str());
  draw_utf8_string(COL3_X, UI_CONTENT_BODY_TOP + 120, destination, 24, BLACK);
}

}  // namespace

void screen_home_render() {
  EPD_DrawLine(DIVIDER1_X, UI_CONTENT_BODY_TOP, DIVIDER1_X, UI_CONTENT_BOTTOM, BLACK);
  EPD_DrawLine(DIVIDER2_X, UI_CONTENT_BODY_TOP, DIVIDER2_X, UI_CONTENT_BOTTOM, BLACK);

  render_weather_column();
  render_event_column();
  render_train_column();
}
