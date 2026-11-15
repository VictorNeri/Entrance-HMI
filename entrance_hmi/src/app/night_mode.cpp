#include "night_mode.h"
#include "../net/time_sync.h"
#include "../storage/broadcast_store.h"
#include "../storage/sd_config.h"
#include "app_state.h"

bool night_mode_in_hours() {
  uint8_t start = sd_config.night_start_hour;
  uint8_t end = sd_config.night_end_hour;
  if (start == end) return false;

  struct tm now_tm;
  if (!time_sync_get_local(now_tm)) return false;
  int h = now_tm.tm_hour;
  return start < end ? (h >= start && h < end) : (h >= start || h < end);
}

bool night_mode_active() {
  if (app_state.current_screen != Screen::HOME) return false;
  if (broadcast_store_has_pending()) return false;
  if (millis() - app_state.last_manual_interaction_ms < NIGHT_WAKE_MS) return false;
  return night_mode_in_hours();
}
