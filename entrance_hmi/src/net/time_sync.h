#pragma once
#include <Arduino.h>
#include <time.h>

// Call every loop() iteration. No-op while WiFi is down. Retries every
// 10s until the first sync succeeds, then resyncs every ~12h.
void time_sync_tick();

bool time_sync_is_synced();

// Non-blocking query of the current local time. Returns false if the
// clock doesn't hold a real date yet. True can come before the first
// sync after a soft reset, when the ESP32 kept its clock running —
// that time is real, SNTP corrects any drift shortly after.
bool time_sync_get_local(struct tm &out);
