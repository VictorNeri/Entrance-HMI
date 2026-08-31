#pragma once
#include <Arduino.h>

struct MqttTickResult {
  // An HA entity's state or the button config list changed — caller
  // should redraw only if HA_CONTROL is the currently visible screen.
  bool ha_changed = false;
  // The calendar event list changed — caller should redraw regardless
  // of which screen is visible, since the header's pending-events
  // indicator is present on every screen.
  bool calendar_changed = false;
};

// Call every loop() iteration. No-op while WiFi is down; otherwise
// connects/reconnects (5s retry interval) and services the client.
MqttTickResult mqtt_client_tick();

// Publish a toggle command for the given entity (OK button on
// HA_CONTROL). No-op if not connected. Does not update local state —
// the entity's displayed state only changes once its state_topic
// confirms the change, not optimistically.
void mqtt_client_publish_toggle(const String &entity_id);

// For the header's connection indicator.
bool mqtt_is_connected();
