#include "screen_weather.h"
#include <stdio.h>
#include <string.h>
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"
#include "../net/weather_client.h"
#include "../net/weather_forecast_client.h"
#include "ui_chrome.h"
#include "weather_icons.h"

namespace {

constexpr float WINDY_THRESHOLD_MS = 8.0f;  // ~29 km/h

void render_current_column() {
  EPD_ShowString(UI_COL1_X, UI_CONTENT_BODY_TOP, "NOW", 24, BLACK);

  if (!weather_data.valid) {
    EPD_ShowString(UI_COL1_X, UI_CONTENT_BODY_TOP + 34, "Waiting for", 24, BLACK);
    EPD_ShowString(UI_COL1_X, UI_CONTENT_BODY_TOP + 66, "first fetch", 24, BLACK);
    return;
  }

  draw_weather_icon(UI_COL1_X + 18, UI_CONTENT_BODY_TOP + 55, weather_data.icon);

  char line[48];
  snprintf(line, sizeof(line), "%.0f C", weather_data.temp_c);
  EPD_ShowString(UI_COL1_X + 44, UI_CONTENT_BODY_TOP + 32, line, 48, BLACK);

  // OWM's description text is normally short ("clear sky") but some
  // are longer ("light intensity drizzle" is a real one) — at size 24
  // this column is only ~18 chars wide, so cap it rather than let it
  // run into the next column.
  char desc[19];
  snprintf(desc, sizeof(desc), "%s", weather_data.description.c_str());
  EPD_ShowString(UI_COL1_X, UI_CONTENT_BODY_TOP + 88, desc, 24, BLACK);

  snprintf(line, sizeof(line), "Humidity: %d%%", weather_data.humidity);
  EPD_ShowString(UI_COL1_X, UI_CONTENT_BODY_TOP + 120, line, 24, BLACK);
}

// Temperature-over-the-day line chart, replacing the old 3-row icon+
// text list (which only ever showed 3 of the then-4 fetched entries
// anyway) — this uses however many entries actually came back
// (MAX_FORECAST_ENTRIES, currently 8 at OWM's 3-hourly cadence =~24h),
// spaced evenly across the column regardless of count so it still
// looks reasonable if a fetch came back short.
void render_hourly_chart() {
  EPD_ShowString(UI_COL2_X, UI_CONTENT_BODY_TOP, "NEXT HOURS", 24, BLACK);

  if (!forecast_data.valid || forecast_data.count == 0) {
    EPD_ShowString(UI_COL2_X, UI_CONTENT_BODY_TOP + 34, "Waiting for", 16, BLACK);
    EPD_ShowString(UI_COL2_X, UI_CONTENT_BODY_TOP + 54, "forecast", 16, BLACK);
    return;
  }

  uint8_t count = forecast_data.count;
  float min_t = forecast_data.items[0].temp_c, max_t = forecast_data.items[0].temp_c;
  for (uint8_t i = 1; i < count; i++) {
    float t = forecast_data.items[i].temp_c;
    if (t < min_t) min_t = t;
    if (t > max_t) max_t = t;
  }

  // High/low called out in their own fixed-position row rather than
  // pinned next to their points on the chart — a label pinned to
  // whichever point happens to be the extreme risks colliding with the
  // header above or the hour-label row below depending on where that
  // point lands, which varies with live data. A fixed row never does.
  char hilo[32];
  snprintf(hilo, sizeof(hilo), "High %.0fC  Low %.0fC", max_t, min_t);
  EPD_ShowString(UI_COL2_X, UI_CONTENT_BODY_TOP + 28, hilo, 16, BLACK);

  constexpr uint16_t CHART_TOP = UI_CONTENT_BODY_TOP + 48;
  constexpr uint16_t CHART_HEIGHT = 80;
  constexpr uint16_t CHART_RIGHT_MARGIN = 8;  // clear of the column divider
  uint16_t available_w = (UI_COL_DIVIDER2_X - CHART_RIGHT_MARGIN) - UI_COL2_X;
  uint16_t pitch = count > 1 ? available_w / (count - 1) : 0;
  float range = (max_t - min_t) > 1.0f ? (max_t - min_t) : 1.0f;  // avoid a divide-by-zero flat line

  uint16_t xs[MAX_FORECAST_ENTRIES], ys[MAX_FORECAST_ENTRIES];
  for (uint8_t i = 0; i < count; i++) {
    xs[i] = UI_COL2_X + i * pitch;
    ys[i] = CHART_TOP + (uint16_t)((max_t - forecast_data.items[i].temp_c) / range * CHART_HEIGHT);
    if (i > 0) EPD_DrawLine(xs[i - 1], ys[i - 1], xs[i], ys[i], BLACK);
  }
  for (uint8_t i = 0; i < count; i++) {
    EPD_DrawCircle(xs[i], ys[i], 2, BLACK, 1);
  }

  // Every point gets an hour label when there's room for it; thin out
  // to every other one once there are enough points that they'd
  // otherwise crowd into each other (checked against this font's
  // 8px/char advance at the pitch 8 entries actually produces).
  uint8_t label_step = count > 6 ? 2 : 1;
  for (uint8_t i = 0; i < count; i += label_step) {
    char label[6];
    snprintf(label, sizeof(label), "%02dh", forecast_data.items[i].hour);
    uint16_t label_w = strlen(label) * 8;  // size-16 glyph advance = size/2 = 8px/char
    uint16_t label_x = xs[i] > label_w / 2 ? xs[i] - label_w / 2 : UI_COL2_X;
    EPD_ShowString(label_x, CHART_TOP + CHART_HEIGHT + 6, label, 16, BLACK);
  }
}

// Derives up to 3 short alert lines from the same forecast data
// (rain/snow/thunderstorm icon codes among the upcoming entries) and
// current wind speed. No dedicated alerts API on OpenWeatherMap's free
// tier (that needs the paid One Call subscription) — this is a
// best-effort summary from data we already fetch, kept local to this
// screen like TRANSIT's bucketing logic rather than pushed into the
// client modules.
uint8_t compute_alerts(const char *out[], uint8_t max_out) {
  bool rain = false, snow = false, thunder = false;
  for (uint8_t i = 0; i < forecast_data.count; i++) {
    const String &icon = forecast_data.items[i].icon;
    if (icon.length() < 2) continue;
    String code = icon.substring(0, 2);
    if (code == "09" || code == "10") rain = true;
    else if (code == "13") snow = true;
    else if (code == "11") thunder = true;
  }

  uint8_t n = 0;
  if (thunder && n < max_out) {
    out[n++] = "Thunderstorm";
  } else if (rain && n < max_out) {
    out[n++] = "Rain later";
  }
  if (snow && n < max_out) out[n++] = "Snow expected";
  if (weather_data.valid && weather_data.wind_speed_ms >= WINDY_THRESHOLD_MS && n < max_out) {
    out[n++] = "Windy";
  }
  return n;
}

void render_alerts_column() {
  EPD_ShowString(UI_COL3_X, UI_CONTENT_BODY_TOP, "ALERTS", 24, BLACK);

  const char *alerts[3];
  uint8_t count = compute_alerts(alerts, 3);
  if (count == 0) {
    EPD_ShowString(UI_COL3_X, UI_CONTENT_BODY_TOP + 34, "No alerts", 24, BLACK);
    return;
  }
  for (uint8_t i = 0; i < count; i++) {
    EPD_ShowString(UI_COL3_X, UI_CONTENT_BODY_TOP + 34 + i * 32, alerts[i], 24, BLACK);
  }
}

}  // namespace

void screen_weather_render() {
  EPD_DrawLine(UI_COL_DIVIDER1_X, UI_CONTENT_BODY_TOP, UI_COL_DIVIDER1_X, UI_CONTENT_BOTTOM, BLACK);
  EPD_DrawLine(UI_COL_DIVIDER2_X, UI_CONTENT_BODY_TOP, UI_COL_DIVIDER2_X, UI_CONTENT_BOTTOM, BLACK);

  render_current_column();
  render_hourly_chart();
  render_alerts_column();

  EPD_ShowString(UI_CONTENT_LEFT, UI_CONTENT_HINT_Y, "OK: refresh now", 16, BLACK);
}
