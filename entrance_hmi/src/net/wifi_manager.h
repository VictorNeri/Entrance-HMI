#pragma once
#include <Arduino.h>

// Bounded blocking connect attempt — call once from setup(). There's
// nothing useful to show before this returns anyway, so blocking here
// is fine; everything after this point must be non-blocking.
void wifi_manager_begin();

// Call every loop() iteration. No-op while connected; otherwise runs a
// non-blocking backoff reconnect (immediate, then 5s, 15s, 60s capped).
void wifi_manager_tick();

bool wifi_is_connected();

// Signal strength in dBm (typically -30 to -90; less negative is
// stronger). Returns 0 when not connected — WiFi.RSSI()'s value is
// meaningless without an active association.
int wifi_get_rssi();
