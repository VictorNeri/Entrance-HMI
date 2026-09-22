#include "broadcast_store.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <string.h>
#include "../net/net_log_shadow.h"

BroadcastMessage broadcast_message;

namespace {

constexpr const char *CONFIG_PATH = "/config/broadcast.json";

bool is_known_level(const char *level) {
  return strcmp(level, "info") == 0 || strcmp(level, "warning") == 0 ||
         strcmp(level, "urgent") == 0;
}

bool parse_into(const char *payload, size_t length, BroadcastMessage &out) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err != DeserializationError::Ok) return false;

  const char *id = doc["id"] | "";
  const char *text = doc["text"] | "";
  if (id[0] == '\0' || text[0] == '\0') return false;  // both required

  BroadcastMessage next;
  next.pending = true;
  strncpy(next.id, id, BROADCAST_ID_MAX);
  next.id[BROADCAST_ID_MAX] = '\0';
  strncpy(next.text, text, BROADCAST_TEXT_MAX);
  next.text[BROADCAST_TEXT_MAX] = '\0';

  const char *level = doc["level"] | "info";
  if (!is_known_level(level)) level = "info";  // unrecognized value, not a malformed payload
  strncpy(next.level, level, BROADCAST_LEVEL_MAX);
  next.level[BROADCAST_LEVEL_MAX] = '\0';

  out = next;
  return true;
}

void persist(const BroadcastMessage &msg) {
  File f = LittleFS.open(CONFIG_PATH, "w");
  if (!f) return;
  JsonDocument doc;
  doc["id"] = msg.id;
  doc["text"] = msg.text;
  doc["level"] = msg.level;
  serializeJson(doc, f);
  f.close();
}

}  // namespace

void broadcast_store_load_from_disk() {
  if (!LittleFS.exists(CONFIG_PATH)) return;

  File f = LittleFS.open(CONFIG_PATH, "r");
  if (!f) return;

  String contents = f.readString();
  f.close();

  BroadcastMessage loaded;
  if (parse_into(contents.c_str(), contents.length(), loaded)) {
    broadcast_message = loaded;
    Serial.printf("[broadcast] loaded pending message from disk cache: %s\n", loaded.id);
  }
}

bool broadcast_store_apply_payload(const char *payload, size_t length) {
  BroadcastMessage parsed;
  if (!parse_into(payload, length, parsed)) {
    Serial.println("[broadcast] rejected invalid broadcast payload, keeping previous state");
    return false;
  }

  broadcast_message = parsed;
  persist(parsed);
  Serial.printf("[broadcast] applied message id=%s level=%s\n", parsed.id, parsed.level);
  return true;
}

bool broadcast_store_has_pending() {
  return broadcast_message.pending;
}

void broadcast_store_acknowledge() {
  broadcast_message = BroadcastMessage{};  // pending=false, cleared id/text/level
  LittleFS.remove(CONFIG_PATH);
}
