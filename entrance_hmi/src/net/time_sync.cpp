#include "time_sync.h"
#include "../../config.h"
#include "wifi_manager.h"

namespace {

constexpr unsigned long RESYNC_INTERVAL_MS = 12UL * 60 * 60 * 1000;  // ~12h once synced
constexpr unsigned long RETRY_INTERVAL_MS = 10UL * 1000;             // before first sync

bool synced = false;
unsigned long last_sync_attempt_ms = 0;

}  // namespace

void time_sync_tick() {
  if (!wifi_is_connected()) return;

  unsigned long now = millis();
  unsigned long interval = synced ? RESYNC_INTERVAL_MS : RETRY_INTERVAL_MS;
  if (now - last_sync_attempt_ms < interval) return;

  configTzTime(TZ_STRING, NTP_SERVER1, NTP_SERVER2);
  last_sync_attempt_ms = now;

  struct tm timeinfo;
  synced = getLocalTime(&timeinfo, synced ? 0 : 3000);
}

bool time_sync_is_synced() {
  return synced;
}

bool time_sync_get_local(struct tm &out) {
  if (!getLocalTime(&out, 0)) {
    synced = false;
    return false;
  }
  synced = true;
  return true;
}
