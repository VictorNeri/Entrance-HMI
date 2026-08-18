#pragma once
#include <Arduino.h>

// Call every loop() iteration. No-op while WiFi is down; otherwise
// connects/reconnects (5s retry interval) and services the client.
// Returns true only if a message this tick actually changed something
// visible (an entity's state, or the config list itself) — callers use
// this to decide whether HA_CONTROL needs a redraw.
bool mqtt_client_tick();

// Publish a toggle command for the given entity (OK button on
// HA_CONTROL). No-op if not connected. Does not update local state —
// the entity's displayed state only changes once its state_topic
// confirms the change, not optimistically.
void mqtt_client_publish_toggle(const String &entity_id);
