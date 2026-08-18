#include "mqtt_client.h"
#include <PubSubClient.h>
#include <WiFiClient.h>
#include "../../config.h"
#include "../storage/button_config_store.h"
#include "wifi_manager.h"

namespace {

WiFiClient net_client;
PubSubClient mqtt(net_client);

String topic_config;
String topic_status;
bool topics_initialized = false;

unsigned long last_reconnect_attempt_ms = 0;
constexpr unsigned long RECONNECT_INTERVAL_MS = 5000;

bool state_changed_flag = false;

String topic_cmd(const String &entity_id) {
  return String(MQTT_TOPIC_PREFIX) + "/cmd/" + entity_id;
}

// NOTE: doesn't unsubscribe topics belonging to entities removed by a
// later config update — a config change is a rare, operator-driven
// event, not a hot path, so a few stale subscriptions lingering for
// the rest of this MQTT session is an acceptable simplification here.
void resubscribe_state_topics() {
  for (uint8_t i = 0; i < ha_entity_list.count; i++) {
    const String &t = ha_entity_list.items[i].state_topic;
    if (t.length() > 0) {
      mqtt.subscribe(t.c_str());
    }
  }
}

void on_message(char *topic, uint8_t *payload, unsigned int length) {
  String topic_str(topic);

  if (topic_str == topic_config) {
    if (button_config_store_apply_payload(reinterpret_cast<const char *>(payload), length)) {
      resubscribe_state_topics();
      state_changed_flag = true;
    }
    return;
  }

  // HA's mqtt_statestream integration typically publishes the raw
  // state string ("on"/"off") as the payload body.
  for (uint8_t i = 0; i < ha_entity_list.count; i++) {
    if (ha_entity_list.items[i].state_topic != topic_str) continue;

    String value;
    value.reserve(length);
    for (unsigned int j = 0; j < length; j++) value += static_cast<char>(payload[j]);
    value.trim();
    value.toLowerCase();

    bool new_on = value == "on" || value == "true" || value == "1";
    if (!ha_entity_list.items[i].state_known || ha_entity_list.items[i].state_on != new_on) {
      state_changed_flag = true;
    }
    ha_entity_list.items[i].state_known = true;
    ha_entity_list.items[i].state_on = new_on;
    break;
  }
}

bool connect() {
  bool ok = strlen(MQTT_USERNAME) > 0
                ? mqtt.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD, topic_status.c_str(),
                               0, true, "offline")
                : mqtt.connect(MQTT_CLIENT_ID, topic_status.c_str(), 0, true, "offline");

  if (ok) {
    mqtt.publish(topic_status.c_str(), "online", true);
    mqtt.subscribe(topic_config.c_str());
    resubscribe_state_topics();
    Serial.println("[mqtt] connected");
  }
  return ok;
}

}  // namespace

bool mqtt_client_tick() {
  if (!wifi_is_connected()) return false;

  if (!topics_initialized) {
    topic_config = String(MQTT_TOPIC_PREFIX) + "/config/buttons";
    topic_status = String(MQTT_TOPIC_PREFIX) + "/status";
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setBufferSize(1024);  // default 256B is too small for the config payload
    mqtt.setCallback(on_message);
    topics_initialized = true;
  }

  if (!mqtt.connected()) {
    unsigned long now = millis();
    if (now - last_reconnect_attempt_ms < RECONNECT_INTERVAL_MS) return false;
    last_reconnect_attempt_ms = now;
    connect();
    return false;
  }

  state_changed_flag = false;
  mqtt.loop();
  return state_changed_flag;
}

void mqtt_client_publish_toggle(const String &entity_id) {
  if (!mqtt.connected()) return;
  mqtt.publish(topic_cmd(entity_id).c_str(), "toggle");
}
