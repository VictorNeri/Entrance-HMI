#pragma once
#include <Arduino.h>
#include <time.h>

// Call every loop() iteration. No-op while WiFi is down. Retries every
// 10s until the first sync succeeds, then resyncs every ~12h.
void time_sync_tick();

bool time_sync_is_synced();

// Non-blocking query of the current local time. Returns false (and
// leaves out untouched by the caller's own logic) if not yet synced.
bool time_sync_get_local(struct tm &out);
