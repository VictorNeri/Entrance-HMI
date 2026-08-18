#include "ui_common.h"
#include "../app/app_state.h"
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"
#include "screen_ha_control.h"
#include "screen_home.h"
#include "screen_status.h"
#include "screen_transit.h"
#include "screen_weather.h"

namespace {
constexpr uint8_t PANEL_POWER_PIN = 7;
uint8_t image_buffer[27200];
}  // namespace

void ui_init() {
  pinMode(PANEL_POWER_PIN, OUTPUT);
  digitalWrite(PANEL_POWER_PIN, HIGH);

  EPD_GPIOInit();
  Paint_NewImage(image_buffer, EPD_W, EPD_H, Rotation, WHITE);
  Paint_Clear(WHITE);
}

void ui_render_current_screen(bool force_full_refresh) {
  Paint_Clear(WHITE);

  switch (app_state.current_screen) {
    case Screen::HOME:
      screen_home_render();
      break;
    case Screen::WEATHER:
      screen_weather_render();
      break;
    case Screen::TRANSIT:
      screen_transit_render();
      break;
    case Screen::HA_CONTROL:
      screen_ha_control_render();
      break;
    case Screen::STATUS:
      screen_status_render();
      break;
  }

  // Upgrade to a full refresh if one is due regardless of what the
  // caller asked for (ghosting hygiene) — piggybacking on a render
  // that's already happening avoids a redundant extra push.
  bool full = force_full_refresh || app_state_needs_forced_full_refresh();

  // Re-init before every push, matching the vendor's proven pattern
  // (factory main.ino calls this ahead of both full and partial pushes,
  // not the bare EPD_Init() left in an unused commented-out code path).
  EPD_GPIOInit();
  EPD_FastMode1Init();
  EPD_Display(image_buffer);
  if (full) {
    EPD_Update();
    app_state_mark_full_refresh_done();
  } else {
    EPD_PartUpdate();
  }
  EPD_DeepSleep();
}
