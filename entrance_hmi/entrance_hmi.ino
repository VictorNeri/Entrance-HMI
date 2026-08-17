// M3: live weather data on the WEATHER screen (OpenWeatherMap, HTTPS).
// TRANSIT/HA_CONTROL/HOME remain M1 placeholders.
#include "src/app/app_state.h"
#include "src/app/nav.h"
#include "src/input/buttons.h"
#include "src/net/time_sync.h"
#include "src/net/weather_client.h"
#include "src/net/wifi_manager.h"
#include "src/ui/ui_common.h"

void setup() {
  Serial.begin(115200);
  ui_init();
  buttons_init();
  wifi_manager_begin();
  ui_render_current_screen(true);
  Serial.println("M3 weather screen ready.");
}

void loop() {
  wifi_manager_tick();
  time_sync_tick();

  // Silent background poll: only redraw if the WEATHER screen is
  // actually visible and the fetched values changed.
  bool weather_updated_visible =
      weather_client_tick() && app_state.current_screen == Screen::WEATHER;
  if (weather_updated_visible) {
    ui_render_current_screen(false);
  }

  ButtonEvent event = buttons_poll();

  // OK on WEATHER forces an immediate re-fetch (TRANSIT/HA_CONTROL gain
  // their own OK behavior at M4/M5).
  if (event.button == Button::OK && app_state.current_screen == Screen::WEATHER) {
    if (weather_client_fetch_now()) {
      ui_render_current_screen(false);
    }
    return;
  }

  RedrawKind redraw = nav_handle_event(event);
  if (redraw != RedrawKind::NONE) {
    ui_render_current_screen(redraw == RedrawKind::FULL);
  }
}
