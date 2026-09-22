#pragma once
#include <Arduino.h>

constexpr size_t BROADCAST_ID_MAX = 47;
constexpr size_t BROADCAST_TEXT_MAX = 240;  // plenty for a word-wrapped on-screen message
constexpr size_t BROADCAST_LEVEL_MAX = 15;

struct BroadcastMessage {
  bool pending = false;
  char id[BROADCAST_ID_MAX + 1] = "";
  char text[BROADCAST_TEXT_MAX + 1] = "";
  char level[BROADCAST_LEVEL_MAX + 1] = "info";  // "info" | "warning" | "urgent"
};

extern BroadcastMessage broadcast_message;

// Loads a still-unacknowledged message persisted from a previous boot
// (see README for the JSON schema). Safe to call even if nothing was
// ever saved. Call at boot, before WiFi/MQTT connect, so an important
// message that arrived just before a reboot/power loss isn't silently
// lost — it comes back up still pending acknowledgment.
void broadcast_store_load_from_disk();

// Parses an MQTT broadcast payload. On success, REPLACES any
// currently-pending message — this device shows one broadcast at a
// time, newest wins, not a queue — and persists it to LittleFS so it
// survives a reboot until acknowledged. Returns true on success; a
// malformed payload (missing "id" or "text") leaves the existing
// pending message, if any, untouched.
bool broadcast_store_apply_payload(const char *payload, size_t length);

bool broadcast_store_has_pending();

// Clears the pending message in memory AND on disk. Callers that need
// to also notify whoever sent it (mqtt_client's ack publish) must
// capture broadcast_message.id before calling this.
void broadcast_store_acknowledge();
