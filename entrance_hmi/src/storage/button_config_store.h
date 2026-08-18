#pragma once
#include <Arduino.h>

constexpr uint8_t MAX_HA_ENTITIES = 10;

struct HaEntity {
  String id;             // e.g. "light.entryway" — used to build the command topic
  String label;           // display text
  String icon;             // icon key (bulb/lock/fan/generic/...) — rendering deferred to later polish
  String action;           // currently only "toggle" is handled
  String state_topic;      // MQTT topic to subscribe for this entity's state

  bool state_known = false;
  bool state_on = false;
};

struct HaEntityList {
  bool valid = false;
  uint8_t count = 0;
  HaEntity items[MAX_HA_ENTITIES];
};

extern HaEntityList ha_entity_list;

// Loads the last-persisted config from LittleFS into ha_entity_list.
// Safe to call even if nothing was ever saved (leaves the list empty).
// Call this at boot, before WiFi/MQTT connect, so HA_CONTROL has
// something to show immediately even if the broker is unreachable.
void button_config_store_load_from_disk();

// Parses an MQTT config payload (see the JSON schema in the project
// plan). On success, updates ha_entity_list in memory AND persists it
// to LittleFS. On parse failure, the existing (possibly stale but
// last-known-good) ha_entity_list and on-disk cache are left
// untouched. Returns true on success.
bool button_config_store_apply_payload(const char *payload, size_t length);
