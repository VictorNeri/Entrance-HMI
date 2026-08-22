#include "app_state.h"
#include "../net/time_sync.h"
#include "../net/transit_client.h"
#include "../storage/button_config_store.h"
#include "../storage/sd_config.h"

AppState app_state;

namespace {
constexpr unsigned long FULL_REFRESH_INTERVAL_MS = 30UL * 60 * 1000;  // 30 min
constexpr int QUIET_HOUR = 4;  // local hour for the daily forced refresh

constexpr Screen ROTATION_RING[] = {Screen::HOME, Screen::WEATHER, Screen::TRANSIT};
constexpr size_t ROTATION_RING_LEN = sizeof(ROTATION_RING) / sizeof(ROTATION_RING[0]);
}  // namespace

bool screen_has_list(Screen screen) {
  return screen == Screen::TRANSIT || screen == Screen::HA_CONTROL;
}

int8_t screen_list_length(Screen screen) {
  if (screen == Screen::TRANSIT) return static_cast<int8_t>(departure_list.count);
  if (screen == Screen::HA_CONTROL) return static_cast<int8_t>(ha_entity_list.count);
  return 0;
}

void app_state_enter_screen(Screen screen) {
  app_state.current_screen = screen;
  app_state.list_cursor = 0;
}

bool app_state_needs_forced_full_refresh() {
  unsigned long now = millis();
  if (now - app_state.last_full_refresh_ms >= FULL_REFRESH_INTERVAL_MS) return true;

  struct tm timeinfo;
  if (time_sync_get_local(timeinfo) && timeinfo.tm_hour == QUIET_HOUR &&
      timeinfo.tm_yday != app_state.last_daily_refresh_yday) {
    return true;
  }
  return false;
}

void app_state_mark_full_refresh_done() {
  app_state.last_full_refresh_ms = millis();
  struct tm timeinfo;
  if (time_sync_get_local(timeinfo)) {
    app_state.last_daily_refresh_yday = timeinfo.tm_yday;
  }
}

void app_state_mark_manual_interaction() {
  app_state.last_manual_interaction_ms = millis();
}

bool app_state_should_auto_rotate() {
  bool on_rotation_screen = app_state.current_screen == Screen::HOME ||
                             app_state.current_screen == Screen::WEATHER ||
                             app_state.current_screen == Screen::TRANSIT;
  if (!on_rotation_screen) return false;

  unsigned long now = millis();
  if (now - app_state.last_manual_interaction_ms < sd_config.screen_rotation_interval_ms) return false;
  if (now - app_state.last_auto_rotation_ms < sd_config.screen_rotation_interval_ms) return false;
  return true;
}

Screen app_state_next_rotation_screen() {
  size_t idx = 0;
  for (size_t i = 0; i < ROTATION_RING_LEN; i++) {
    if (ROTATION_RING[i] == app_state.current_screen) {
      idx = i;
      break;
    }
  }
  return ROTATION_RING[(idx + 1) % ROTATION_RING_LEN];
}

void app_state_mark_auto_rotated() {
  app_state.last_auto_rotation_ms = millis();
}
