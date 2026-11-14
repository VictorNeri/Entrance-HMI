// Entrance HMI — weather, SL transit departures, and MQTT-configured
// Home Assistant control buttons on a 5-screen, 5-button e-paper kiosk.
#include <esp_system.h>
#include <esp_task_wdt.h>
#include "src/app/app_state.h"
#include "src/app/error_log.h"
#include "src/app/nav.h"
#include "src/input/buttons.h"
#include "src/net/fetch_worker.h"
#include "src/net/mqtt_client.h"
#include "src/net/ota_manager.h"
#include "src/net/time_sync.h"
#include "src/net/wifi_manager.h"
#include "src/storage/broadcast_store.h"
#include "src/storage/button_config_store.h"
#include "src/storage/calendar_store.h"
#include "src/storage/littlefs_setup.h"
#include "src/storage/sd_config.h"
#include "src/ui/ui_common.h"
#include "src/net/net_log_shadow.h"

namespace {

// Task watchdog budget for loop() and the fetch worker. The core's
// default is 5s, too short for normal blocking work here: a full
// e-paper refresh, an MQTT connect attempt (DNS + TCP + CONNACK wait),
// or one HTTPS attempt on the worker (connect + handshake + read, each
// bounded at <=15s). 60s is comfortably above all of those, and still
// short enough that a genuinely hung device (e.g. the EPD driver's
// unbounded busy-wait, or a wedged socket) reboots itself within a
// minute instead of sitting frozen on the wall.
constexpr uint32_t WATCHDOG_TIMEOUT_MS = 60UL * 1000;

void watchdog_begin() {
  esp_task_wdt_config_t config = {
      .timeout_ms = WATCHDOG_TIMEOUT_MS,
      .idle_core_mask = 1 << 0,  // keep the core's default: watch core 0's idle task
      .trigger_panic = true,     // panic -> reboot, rather than just logging
  };
  esp_task_wdt_reconfigure(&config);
}

const char *reset_reason_name(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "power-on";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "interrupt watchdog";
    case ESP_RST_TASK_WDT: return "task watchdog";
    case ESP_RST_WDT: return "other watchdog";
    case ESP_RST_BROWNOUT: return "brownout";
    default: return "other";
  }
}

// So a self-recovery is visible after the fact — on serial/net-log
// and on the STATUS screen's error line — instead of silent.
void log_reset_reason() {
  esp_reset_reason_t reason = esp_reset_reason();
  Serial.printf("[boot] reset reason: %s (%d)\n", reset_reason_name(reason), (int)reason);
  if (reason == ESP_RST_TASK_WDT || reason == ESP_RST_INT_WDT || reason == ESP_RST_WDT ||
      reason == ESP_RST_PANIC || reason == ESP_RST_BROWNOUT) {
    char msg[64];
    snprintf(msg, sizeof(msg), "boot: recovered from %s reset", reset_reason_name(reason));
    error_log_set(msg);
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);
  watchdog_begin();
  log_reset_reason();

  ui_init();
  buttons_init();

  // Before anything WiFi-dependent — supplies wifi_manager's credentials
  // and transit_client's station ID.
  sd_config_load();

  littlefs_setup_begin();
  // Shows cached HA entities immediately, even before MQTT connects.
  button_config_store_load_from_disk();
  // Same reasoning for the header's pending-events indicator.
  calendar_store_load_from_disk();
  // Same reasoning again — an important message that arrived just
  // before a reboot/power loss shouldn't be silently lost.
  broadcast_store_load_from_disk();

  wifi_manager_begin();
  // Owns all HTTPS fetching from here on, so loop() never blocks on it.
  fetch_worker_begin();
  ui_render_current_screen(true);
  // Subscribes the loop task to the watchdog (the core then feeds it
  // once per loop() iteration). Last in setup() — setup's own blocking
  // steps (the boot WiFi connect alone waits up to 15s) aren't covered.
  enableLoopWDT();
  Serial.println("Entrance HMI ready.");
}

void loop() {
  // Periodic free-heap/WiFi-state log — the M7 soak-test checkpoint
  // for watching heap trend and reconnect resilience over days of
  // uptime, not just debug scaffolding.
  static unsigned long last_heartbeat_ms = 0;
  if (millis() - last_heartbeat_ms >= 60UL * 1000) {
    last_heartbeat_ms = millis();
    Serial.printf("[heartbeat] up=%lums wifi=%d rssi=%d heap=%u\n", millis(), wifi_is_connected(),
                  wifi_get_rssi(), (unsigned)ESP.getFreeHeap());
  }

  wifi_manager_tick();
  ota_manager_tick();
  time_sync_tick();
  net_log_tick();

  // Header clock: redraw (partial) whenever the local minute changes.
  // Per-second is off the table (full-buffer repaint + e-paper refresh
  // latency), once/minute is the realistic ceiling. The -1 sentinel
  // also naturally covers "NTP just synced for the first time" (any
  // real tm_min 0-59 differs from it), replacing the header's "--:--"
  // placeholder as soon as time becomes available.
  static int last_clock_minute = -1;
  struct tm clock_tm;
  if (time_sync_get_local(clock_tm) && clock_tm.tm_min != last_clock_minute) {
    last_clock_minute = clock_tm.tm_min;
    ui_render_current_screen(false);
  }

  // Ghosting hygiene: fires even during long idle periods with no
  // other render trigger (ui_common's own check only fires piggybacked
  // on a render that's already happening for some other reason).
  if (app_state_needs_forced_full_refresh()) {
    ui_render_current_screen(true);
  }

  // mqtt.loop() needs a timely, regular call to send its own keepalive
  // pings, or the broker disconnects it for "exceeded timeout" even
  // though the connection itself is healthy. HTTPS fetches no longer
  // run on this loop (see fetch_worker.h), so nothing here blocks long
  // enough to starve it.
  MqttTickResult mqtt_tick = mqtt_client_tick();
  bool ha_updated_visible = mqtt_tick.ha_changed && app_state.current_screen == Screen::HA_CONTROL;
  // Calendar changes redraw regardless of visible screen — the header's
  // pending-events indicator is present on every screen, unlike
  // HA_CONTROL's list which only matters while that screen is shown.
  if (ha_updated_visible || mqtt_tick.calendar_changed) {
    ui_render_current_screen(false);
  }
  // A broadcast message is a full-canvas modal — always force a FULL
  // refresh so its border/text render cleanly regardless of whatever
  // partial-refresh state the panel was left in.
  if (mqtt_tick.broadcast_changed) {
    ui_render_current_screen(true);
  }

  // Weather/forecast/transit results fetched in the background since
  // the last iteration get published here. Same redraw rules as when
  // these fetches ran inline: only the screen that shows the data
  // redraws now (HOME picks them up on its next per-minute redraw).
  bool transit_on_screen = app_state.current_screen == Screen::TRANSIT;
  fetch_worker_set_transit_active(transit_on_screen);
  FetchUpdates fetched = fetch_worker_collect();
  bool weather_on_screen = app_state.current_screen == Screen::WEATHER;
  if ((weather_on_screen && (fetched.weather || fetched.forecast)) ||
      (transit_on_screen && fetched.transit)) {
    ui_render_current_screen(false);
  }

  // A glance at WEATHER/TRANSIT/HA_CONTROL/STATUS settles back on HOME
  // on its own once idle — see app_state_should_return_home() for the
  // exact rule. HOME then just sits there updating via the normal data
  // ticks below until the next button press.
  if (app_state_should_return_home()) {
    app_state_enter_screen(Screen::HOME);
    ui_render_current_screen(true);
  }

  ButtonEvent event = buttons_poll();
  if (event.button != Button::NONE) {
    app_state_mark_manual_interaction();
  }

  // An unacknowledged broadcast message is a modal — it absorbs every
  // button until OK dismisses it, so none of the normal per-screen OK
  // handling or nav_handle_event() below ever sees these events while
  // it's showing.
  if (broadcast_store_has_pending()) {
    if (event.button == Button::OK) {
      mqtt_client_acknowledge_broadcast();
      ui_render_current_screen(true);
    }
    return;
  }

  if (event.button == Button::OK) {
    // Refresh requests are non-blocking: the fetch runs on the worker
    // and the screen redraws when fetch_worker_collect() above
    // publishes the result (user-requested weather always redraws,
    // even if the values didn't change).
    if (app_state.current_screen == Screen::WEATHER) {
      fetch_worker_request_weather();
      return;
    }
    if (app_state.current_screen == Screen::TRANSIT) {
      fetch_worker_request_transit();
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
    fetch_worker_request_transit();
  }

  if (redraw != RedrawKind::NONE) {
    ui_render_current_screen(redraw == RedrawKind::FULL);
  }
}
