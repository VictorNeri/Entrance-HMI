#include "app_state.h"
#include "../net/time_sync.h"
#include "../storage/button_config_store.h"
#include "../storage/sd_config.h"

AppState app_state;

const ScreenInfo SCREEN_TABLE[] = {
    {Screen::HOME, "HOME", true, true},
    {Screen::WEATHER, "WEATHER", true, true},
    {Screen::TRANSIT, "TRANSIT", true, true},
    {Screen::HA_CONTROL, "HA CTRL", true, false},
    {Screen::STATUS, "STATUS", false, false},
};
const size_t SCREEN_TABLE_LEN = sizeof(SCREEN_TABLE) / sizeof(SCREEN_TABLE[0]);

namespace {
constexpr unsigned long FULL_REFRESH_INTERVAL_MS = 30UL * 60 * 1000;  // 30 min
constexpr int QUIET_HOUR = 4;  // local hour for the daily forced refresh

// The once-a-minute clock trigger (entrance_hmi.ino) means a partial
// render now fires roughly every 60s during otherwise-idle periods,
// versus only on user interaction/data ticks before. This 30-minute
// forced-FULL timer is the primary ghosting defense against that much
// higher partial-push rate — don't shorten it without checking actual
// on-hardware ghosting first.
}  // namespace

bool screen_has_list(Screen screen) {
  return screen == Screen::HA_CONTROL;
}

int8_t screen_list_length(Screen screen) {
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

namespace {
bool is_in_rotation_ring(Screen screen) {
  for (size_t i = 0; i < SCREEN_TABLE_LEN; i++) {
    if (SCREEN_TABLE[i].screen == screen) return SCREEN_TABLE[i].in_rotation_ring;
  }
  return false;
}
}  // namespace

bool app_state_should_auto_rotate() {
  if (!is_in_rotation_ring(app_state.current_screen)) return false;

  unsigned long now = millis();
  if (now - app_state.last_manual_interaction_ms < sd_config.screen_rotation_interval_ms) return false;
  if (now - app_state.last_auto_rotation_ms < sd_config.screen_rotation_interval_ms) return false;
  return true;
}

Screen app_state_next_rotation_screen() {
  // Index within the filtered rotation-ring subset of SCREEN_TABLE.
  size_t rotation_indices[SCREEN_TABLE_LEN];
  size_t rotation_len = 0;
  size_t current_idx = 0;
  for (size_t i = 0; i < SCREEN_TABLE_LEN; i++) {
    if (!SCREEN_TABLE[i].in_rotation_ring) continue;
    if (SCREEN_TABLE[i].screen == app_state.current_screen) current_idx = rotation_len;
    rotation_indices[rotation_len++] = i;
  }
  if (rotation_len == 0) return app_state.current_screen;
  return SCREEN_TABLE[rotation_indices[(current_idx + 1) % rotation_len]].screen;
}

void app_state_mark_auto_rotated() {
  app_state.last_auto_rotation_ms = millis();
}
