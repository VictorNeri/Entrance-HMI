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
  // A new broadcast message arrived (or superseded an unacknowledged
  // one) — caller should force a FULL redraw regardless of current
  // screen, since the modal covers the whole canvas.
  bool broadcast_changed = false;
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

// Called by the main loop when the user acknowledges the currently
// pending broadcast message (OK button while screen_broadcast's modal
// is showing). Always clears broadcast_store's pending state locally
// first — regardless of connection state, so a WiFi hiccup can't trap
// someone in front of the panel behind an unacknowledgeable modal —
// then, only if currently connected, publishes an ack to
// <prefix>/broadcast/ack so whoever sent it knows it was seen.
void mqtt_client_acknowledge_broadcast();
