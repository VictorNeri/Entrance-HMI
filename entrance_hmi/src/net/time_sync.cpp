#include "time_sync.h"
#include "../../config.h"
#include "wifi_manager.h"

namespace {

constexpr unsigned long RESYNC_INTERVAL_MS = 12UL * 60 * 60 * 1000;  // ~12h once synced
constexpr unsigned long RETRY_INTERVAL_MS = 10UL * 1000;             // before first sync

bool synced = false;
unsigned long last_sync_attempt_ms = 0;

// Non-blocking read of the local clock; true once it holds a real
// (NTP-set) date. Deliberately not getLocalTime(&tm, 0): the core
// implements it as `while (millis() - start <= ms) { ...check... }`,
// so with ms=0 the check never runs at all whenever millis() ticks
// over between those two reads, and it spuriously returns false — a
// few times a second when called every loop(). That flipped night mode
// on and off and could also drop `synced` into the 10s NTP retry loop.
bool read_local(struct tm &out) {
  time_t now;
  time(&now);
  localtime_r(&now, &out);
  return out.tm_year > (2016 - 1900);
}

}  // namespace

void time_sync_tick() {
  if (!wifi_is_connected()) return;

  unsigned long now = millis();
  unsigned long interval = synced ? RESYNC_INTERVAL_MS : RETRY_INTERVAL_MS;
  if (now - last_sync_attempt_ms < interval) return;

  configTzTime(TZ_STRING, NTP_SERVER1, NTP_SERVER2);
  last_sync_attempt_ms = now;

  struct tm timeinfo;
  synced = synced ? read_local(timeinfo) : getLocalTime(&timeinfo, 3000);
}

bool time_sync_is_synced() {
  return synced;
}

bool time_sync_get_local(struct tm &out) {
  if (!read_local(out)) {
    synced = false;
    return false;
  }
  synced = true;
  return true;
}
