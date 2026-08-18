// M4: live SL departures on the TRANSIT screen. HA_CONTROL/HOME
// remain M1 placeholders.
#include "src/app/app_state.h"
#include "src/app/nav.h"
#include "src/input/buttons.h"
#include "src/net/time_sync.h"
#include "src/net/transit_client.h"
#include "src/net/weather_client.h"
#include "src/net/wifi_manager.h"
#include "src/ui/ui_common.h"

namespace {
constexpr unsigned long TRANSIT_POLL_ACTIVE_MS = 90UL * 1000;    // while TRANSIT is visible
constexpr unsigned long TRANSIT_POLL_BACKGROUND_MS = 300UL * 1000;  // otherwise
}  // namespace

void setup() {
  Serial.begin(115200);
  ui_init();
  buttons_init();
  wifi_manager_begin();
  ui_render_current_screen(true);
  Serial.println("M4 transit screen ready.");
}

void loop() {
  wifi_manager_tick();
  time_sync_tick();

  bool weather_updated_visible =
      weather_client_tick() && app_state.current_screen == Screen::WEATHER;
  if (weather_updated_visible) {
    ui_render_current_screen(false);
  }

  bool transit_on_screen = app_state.current_screen == Screen::TRANSIT;
  unsigned long transit_poll_interval_ms =
      transit_on_screen ? TRANSIT_POLL_ACTIVE_MS : TRANSIT_POLL_BACKGROUND_MS;
  bool transit_updated_visible = transit_client_tick(transit_poll_interval_ms) && transit_on_screen;
  if (transit_updated_visible) {
    ui_render_current_screen(false);
  }

  ButtonEvent event = buttons_poll();

  if (event.button == Button::OK) {
    if (app_state.current_screen == Screen::WEATHER && weather_client_fetch_now()) {
      ui_render_current_screen(false);
      return;
    }
    if (app_state.current_screen == Screen::TRANSIT && transit_client_fetch_now()) {
      ui_render_current_screen(false);
      return;
    }
  }

  RedrawKind redraw = nav_handle_event(event);

  // A FULL redraw on TRANSIT means we just switched onto it (scrolling
  // within the screen only ever produces PARTIAL) — re-arm fast
  // polling immediately rather than waiting for the next tick.
  if (redraw == RedrawKind::FULL && app_state.current_screen == Screen::TRANSIT) {
    transit_client_fetch_now();
  }

  if (redraw != RedrawKind::NONE) {
    ui_render_current_screen(redraw == RedrawKind::FULL);
  }
}
