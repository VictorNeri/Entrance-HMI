#include "screen_weather.h"
#include <stdio.h>
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

void render_hourly_column() {
  EPD_ShowString(UI_COL2_X, UI_CONTENT_BODY_TOP, "NEXT HOURS", 24, BLACK);

  if (!forecast_data.valid || forecast_data.count == 0) {
    EPD_ShowString(UI_COL2_X, UI_CONTENT_BODY_TOP + 34, "Waiting for", 16, BLACK);
    EPD_ShowString(UI_COL2_X, UI_CONTENT_BODY_TOP + 54, "forecast", 16, BLACK);
    return;
  }

  // 3 rows, not the 4 fetched — leaves a safe margin above the hint
  // line at the bigger font size (verified: 4 rows left only ~4px
  // clearance, too tight given how EPD_ShowChar has zero bounds
  // checking and a miscalculation here previously caused a real crash).
  for (uint8_t i = 0; i < forecast_data.count && i < 3; i++) {
    uint16_t row_y = UI_CONTENT_BODY_TOP + 34 + i * 36;
    const ForecastEntry &entry = forecast_data.items[i];
    draw_weather_icon_mini(UI_COL2_X + 8, row_y + 8, entry.icon);
    char line[24];
    snprintf(line, sizeof(line), "%02d:00  %.0fC", entry.hour, entry.temp_c);
    EPD_ShowString(UI_COL2_X + 22, row_y, line, 16, BLACK);
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
  render_hourly_column();
  render_alerts_column();

  EPD_ShowString(UI_CONTENT_LEFT, UI_CONTENT_HINT_Y, "OK: refresh now", 16, BLACK);
}
