#include "button_config_store.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

HaEntityList ha_entity_list;

namespace {

constexpr const char *CONFIG_PATH = "/config/buttons.json";

bool parse_into(const char *payload, size_t length, HaEntityList &out) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err != DeserializationError::Ok) return false;

  JsonArray entities = doc["entities"].as<JsonArray>();
  if (entities.isNull()) return false;

  HaEntityList next;
  for (JsonObject e : entities) {
    if (next.count >= MAX_HA_ENTITIES) break;
    const char *id = e["id"] | "";
    if (id[0] == '\0') continue;  // id is required — skip malformed entries

    HaEntity &item = next.items[next.count];
    item.id = id;
    item.label = (const char *)(e["label"] | id);
    item.icon = (const char *)(e["icon"] | "generic");
    item.action = (const char *)(e["action"] | "toggle");
    item.state_topic = (const char *)(e["state_topic"] | "");
    next.count++;
  }

  if (next.count == 0) return false;
  next.valid = true;
  out = next;
  return true;
}

}  // namespace

void button_config_store_load_from_disk() {
  if (!LittleFS.exists(CONFIG_PATH)) return;

  File f = LittleFS.open(CONFIG_PATH, "r");
  if (!f) return;

  String contents = f.readString();
  f.close();

  HaEntityList loaded;
  if (parse_into(contents.c_str(), contents.length(), loaded)) {
    ha_entity_list = loaded;
    Serial.printf("[ha-config] loaded %u entities from disk cache\n", (unsigned)loaded.count);
  }
}

bool button_config_store_apply_payload(const char *payload, size_t length) {
  HaEntityList parsed;
  if (!parse_into(payload, length, parsed)) {
    Serial.println("[ha-config] rejected invalid config payload, keeping previous config");
    return false;
  }

  ha_entity_list = parsed;

  // Re-serialize rather than writing the raw payload back, so the
  // on-disk cache always matches what was actually parsed/accepted.
  File f = LittleFS.open(CONFIG_PATH, "w");
  if (f) {
    JsonDocument doc;
    doc["version"] = 1;
    JsonArray arr = doc["entities"].to<JsonArray>();
    for (uint8_t i = 0; i < parsed.count; i++) {
      JsonObject o = arr.add<JsonObject>();
      o["id"] = parsed.items[i].id;
      o["label"] = parsed.items[i].label;
      o["icon"] = parsed.items[i].icon;
      o["action"] = parsed.items[i].action;
      o["state_topic"] = parsed.items[i].state_topic;
    }
    serializeJson(doc, f);
    f.close();
  }

  Serial.printf("[ha-config] applied %u entities, free heap: %u bytes\n", (unsigned)parsed.count,
                (unsigned)ESP.getFreeHeap());
  return true;
}
