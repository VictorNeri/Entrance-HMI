// Entrance HMI — weather, SL transit departures, and MQTT-configured
// Home Assistant control buttons on a 5-screen, 5-button e-paper kiosk.
#include "src/app/app_state.h"
#include "src/app/nav.h"
#include "src/input/buttons.h"
#include "src/net/mqtt_client.h"
#include "src/net/time_sync.h"
#include "src/net/transit_client.h"
#include "src/net/weather_client.h"
#include "src/net/wifi_manager.h"
#include "src/storage/button_config_store.h"
#include "src/storage/littlefs_setup.h"
#include "src/storage/sd_config.h"
#include "src/ui/ui_common.h"

void setup() {
  Serial.begin(115200);
  delay(500);

  ui_init();
  buttons_init();

  // Before anything WiFi-dependent — supplies wifi_manager's credentials
  // and transit_client's station ID.
  sd_config_load();

  littlefs_setup_begin();
  // Shows cached HA entities immediately, even before MQTT connects.
  button_config_store_load_from_disk();

  wifi_manager_begin();
  ui_render_current_screen(true);
  Serial.println("Entrance HMI ready.");
}

void loop() {
  // Periodic free-heap/WiFi-state log — the M7 soak-test checkpoint
  // for watching heap trend and reconnect resilience over days of
  // uptime, not just debug scaffolding.
  static unsigned long last_heartbeat_ms = 0;
  if (millis() - last_heartbeat_ms >= 60UL * 1000) {
    last_heartbeat_ms = millis();
    Serial.printf("[heartbeat] up=%lums wifi=%d heap=%u\n", millis(), wifi_is_connected(),
                  (unsigned)ESP.getFreeHeap());
  }

  wifi_manager_tick();
  time_sync_tick();

  // Ghosting hygiene: fires even during long idle periods with no
  // other render trigger (ui_common's own check only fires piggybacked
  // on a render that's already happening for some other reason).
  if (app_state_needs_forced_full_refresh()) {
    ui_render_current_screen(true);
  }

  bool weather_updated_visible =
      weather_client_tick() && app_state.current_screen == Screen::WEATHER;
  if (weather_updated_visible) {
    ui_render_current_screen(false);
  }

  bool transit_on_screen = app_state.current_screen == Screen::TRANSIT;
  unsigned long transit_poll_interval_ms =
      transit_on_screen ? sd_config.transit_poll_active_ms : sd_config.transit_poll_background_ms;
  bool transit_updated_visible = transit_client_tick(transit_poll_interval_ms) && transit_on_screen;
  if (transit_updated_visible) {
    ui_render_current_screen(false);
  }

  bool ha_updated_visible = mqtt_client_tick() && app_state.current_screen == Screen::HA_CONTROL;
  if (ha_updated_visible) {
    ui_render_current_screen(false);
  }

  // Auto-rotation only cycles HOME/WEATHER/TRANSIT and only fires once
  // idle — see app_state_should_auto_rotate() for the exact rule.
  if (app_state_should_auto_rotate()) {
    app_state_enter_screen(app_state_next_rotation_screen());
    app_state_mark_auto_rotated();
    // Matches manual navigation's re-arm-on-entry behavior below —
    // without this, rotating onto TRANSIT would show stale data until
    // the next background-cadence tick, up to 5 minutes later.
    if (app_state.current_screen == Screen::TRANSIT) {
      transit_client_fetch_now();
    }
    ui_render_current_screen(true);
  }

  ButtonEvent event = buttons_poll();
  if (event.button != Button::NONE) {
    app_state_mark_manual_interaction();
  }

  if (event.button == Button::OK) {
    if (app_state.current_screen == Screen::WEATHER && weather_client_fetch_now()) {
      ui_render_current_screen(false);
      return;
    }
    if (app_state.current_screen == Screen::TRANSIT && transit_client_fetch_now()) {
      ui_render_current_screen(false);
      return;
    }
    if (app_state.current_screen == Screen::HA_CONTROL && ha_entity_list.count > 0) {
      uint8_t index = app_state.list_cursor < ha_entity_list.count ? app_state.list_cursor
                                                                      : ha_entity_list.count - 1;
      const HaEntity &entity = ha_entity_list.items[index];
      if (entity.action == "toggle") {
        mqtt_client_publish_toggle(entity.id);
        // No redraw here — the displayed state only updates once HA's
        // statestream confirms the change (via mqtt_client_tick()
        // above), not optimistically.
      }
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
