#include "mqtt_client.h"
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <stdio.h>
#include "../../config.h"
#include "../app/error_log.h"
#include "../storage/button_config_store.h"
#include "../storage/calendar_store.h"
#include "wifi_manager.h"

namespace {

WiFiClient net_client;
PubSubClient mqtt(net_client);

String topic_config;
String topic_calendar;
String topic_status;
bool topics_initialized = false;

unsigned long last_reconnect_attempt_ms = 0;
constexpr unsigned long RECONNECT_INTERVAL_MS = 5000;

bool ha_changed_flag = false;
bool calendar_changed_flag = false;

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
      ha_changed_flag = true;
    }
    return;
  }

  if (topic_str == topic_calendar) {
    if (calendar_store_apply_payload(reinterpret_cast<const char *>(payload), length)) {
      calendar_changed_flag = true;
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
      ha_changed_flag = true;
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
    mqtt.subscribe(topic_calendar.c_str());
    resubscribe_state_topics();
    Serial.println("[mqtt] connected");
  } else {
    // PubSubClient's state() is a small negative/positive error code
    // (see PubSubClient.h) — not a human string, but enough to
    // distinguish "broker unreachable" from "auth rejected" etc. on
    // the STATUS screen without needing a lookup table here.
    char msg[64];
    snprintf(msg, sizeof(msg), "mqtt: connect failed (state %d)", mqtt.state());
    error_log_set(msg);
  }
  return ok;
}

}  // namespace

MqttTickResult mqtt_client_tick() {
  if (!wifi_is_connected()) return {};

  if (!topics_initialized) {
    topic_config = String(MQTT_TOPIC_PREFIX) + "/config/buttons";
    topic_calendar = String(MQTT_TOPIC_PREFIX) + "/config/calendar";
    topic_status = String(MQTT_TOPIC_PREFIX) + "/status";
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    // Default 256B is far too small — button-config already needed
    // 1024B, and calendar payloads (several events, each with a title
    // string) push that further. One shared buffer sized for the
    // worst case across both payload types.
    mqtt.setBufferSize(3072);
    mqtt.setCallback(on_message);
    topics_initialized = true;
  }

  if (!mqtt.connected()) {
    unsigned long now = millis();
    if (now - last_reconnect_attempt_ms < RECONNECT_INTERVAL_MS) return {};
    last_reconnect_attempt_ms = now;
    connect();
    return {};
  }

  ha_changed_flag = false;
  calendar_changed_flag = false;
  mqtt.loop();
  return {ha_changed_flag, calendar_changed_flag};
}

void mqtt_client_publish_toggle(const String &entity_id) {
  if (!mqtt.connected()) return;
  mqtt.publish(topic_cmd(entity_id).c_str(), "toggle");
}

bool mqtt_is_connected() {
  return mqtt.connected();
}
