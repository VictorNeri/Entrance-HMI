#pragma once
#include <Arduino.h>

// Call every loop() iteration. No-op while WiFi is down; lazily runs
// ArduinoOTA's one-time setup (mDNS + UDP listener) on the first tick
// after WiFi connects, then services it every tick after — mirrors
// mqtt_client_tick()'s pattern so OTA still comes up correctly even if
// WiFi wasn't connected yet when this first started ticking (boot with
// bad credentials fixed later, a reconnect after an outage, etc.).
// Advertises via mDNS as "<OTA_HOSTNAME>.local" so `arduino-cli
// upload`/Arduino IDE can target the device over WiFi (network port)
// instead of USB. Once an update is actually pushed, this call blocks
// until it finishes (success, reboot, or failure) — that's ArduinoOTA's
// own behavior, not something this wrapper adds.
void ota_manager_tick();
