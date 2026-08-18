#include "app_state.h"
#include "../net/time_sync.h"
#include "../net/transit_client.h"
#include "../storage/button_config_store.h"

AppState app_state;

namespace {
constexpr unsigned long FULL_REFRESH_INTERVAL_MS = 30UL * 60 * 1000;  // 30 min
constexpr int QUIET_HOUR = 4;  // local hour for the daily forced refresh
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
