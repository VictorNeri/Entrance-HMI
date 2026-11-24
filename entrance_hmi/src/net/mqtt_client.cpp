#include "mqtt_client.h"
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <stdio.h>
#include "../../config.h"
#include "../app/error_log.h"
#include "../storage/broadcast_store.h"
#include "../storage/button_config_store.h"
#include "../storage/calendar_store.h"
#include "../storage/sd_config.h"
#include "ha_discovery.h"
#include "time_sync.h"
#include "wifi_manager.h"
#include "net_log_shadow.h"

namespace {

WiFiClient plain_client;
WiFiClientSecure tls_client;
PubSubClient mqtt;

// Set once at init: TLS on but no usable CA. Never connects then — the
// alternatives (plaintext, or TLS without verifying the broker) would
// silently give up exactly what mqtt_tls was turned on for.
bool tls_misconfigured = false;

// Bounds the TLS handshake, which otherwise defaults to 120s — this
// runs on loop(), under a 60s task watchdog.
constexpr uint32_t TLS_HANDSHAKE_TIMEOUT_S = 10;

String topic_config;
String topic_calendar;
String topic_broadcast;
String topic_broadcast_ack;
String topic_status;
String topic_telemetry;
bool topics_initialized = false;

unsigned long last_reconnect_attempt_ms = 0;
unsigned long last_telemetry_ms = 0;
constexpr unsigned long RECONNECT_INTERVAL_MS = 5000;

bool ha_changed_flag = false;
bool calendar_changed_flag = false;
bool broadcast_changed_flag = false;

String topic_cmd(const String &entity_id) {
  return sd_config.mqtt_topic_prefix + "/cmd/" + entity_id;
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

  if (topic_str == topic_broadcast) {
    if (broadcast_store_apply_payload(reinterpret_cast<const char *>(payload), length)) {
      broadcast_changed_flag = true;
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

// Retained, so HA picks the device up whenever it (re)starts. Sent with
// the streaming publish, which isn't bound by the client buffer.
// With discovery turned off, an empty retained payload removes a device
// an earlier run may have created.
void publish_ha_discovery() {
  String topic = ha_discovery_config_topic();
  String payload = sd_config.ha_discovery
                       ? ha_discovery_config_payload(topic_telemetry, topic_status)
                       : String();
  bool ok = mqtt.beginPublish(topic.c_str(), payload.length(), true) &&
            mqtt.write(reinterpret_cast<const uint8_t *>(payload.c_str()), payload.length()) ==
                payload.length() &&
            mqtt.endPublish();
  if (!ok) {
    Serial.println("[mqtt] HA discovery publish failed");
  } else if (sd_config.ha_discovery) {
    Serial.printf("[mqtt] HA discovery published (%u bytes)\n", (unsigned)payload.length());
  }
}

void publish_telemetry() {
  last_telemetry_ms = millis();
  if (!sd_config.ha_discovery) return;
  String payload = ha_telemetry_payload();
  mqtt.publish(topic_telemetry.c_str(), payload.c_str(), true);
}

bool connect() {
  bool ok = sd_config.mqtt_username.length() > 0
                ? mqtt.connect(MQTT_CLIENT_ID, sd_config.mqtt_username.c_str(),
                               sd_config.mqtt_password.c_str(), topic_status.c_str(), 0, true,
                               "offline")
                : mqtt.connect(MQTT_CLIENT_ID, topic_status.c_str(), 0, true, "offline");

  if (ok) {
    mqtt.publish(topic_status.c_str(), "online", true);
    mqtt.subscribe(topic_config.c_str());
    mqtt.subscribe(topic_calendar.c_str());
    mqtt.subscribe(topic_broadcast.c_str());
    resubscribe_state_topics();
    Serial.println(sd_config.mqtt_tls ? "[mqtt] connected (TLS)" : "[mqtt] connected");
    publish_ha_discovery();
    publish_telemetry();
  } else {
    // PubSubClient's state() is a small negative/positive error code
    // (see PubSubClient.h) — not a human string, but enough to
    // distinguish "broker unreachable" from "auth rejected" etc. on
    // the STATUS screen without needing a lookup table here.
    char msg[64];
    if (sd_config.mqtt_tls && mqtt.state() == MQTT_CONNECT_FAILED) {
      // TLS failures (untrusted or mismatched broker certificate,
      // handshake timeout) all surface as PubSubClient state -2
      // "connect failed"; mbedTLS's own error says which.
      char tls_err[96];
      int code = tls_client.lastError(tls_err, sizeof(tls_err));
      Serial.printf("[mqtt] TLS connect failed: -0x%04x %s\n", -code, tls_err);
      snprintf(msg, sizeof(msg), "mqtt: TLS failed (-0x%04x)", -code);
    } else {
      snprintf(msg, sizeof(msg), "mqtt: connect failed (state %d)", mqtt.state());
    }
    error_log_set(msg);
  }
  return ok;
}

}  // namespace

MqttTickResult mqtt_client_tick() {
  if (!wifi_is_connected()) return {};

  if (!topics_initialized) {
    topic_config = sd_config.mqtt_topic_prefix + "/config/buttons";
    topic_calendar = sd_config.mqtt_topic_prefix + "/config/calendar";
    topic_broadcast = sd_config.mqtt_topic_prefix + "/broadcast/message";
    topic_broadcast_ack = sd_config.mqtt_topic_prefix + "/broadcast/ack";
    topic_status = sd_config.mqtt_topic_prefix + "/status";
    topic_telemetry = sd_config.mqtt_topic_prefix + "/telemetry";
    if (sd_config.mqtt_tls) {
      if (sd_config.mqtt_ca_pem.length() == 0) {
        tls_misconfigured = true;
        error_log_set("mqtt: TLS on, no CA in /mqtt_ca.pem");
      }
      // setCACert keeps the pointer, not a copy — sd_config.mqtt_ca_pem
      // lives (unchanged) for the life of the program.
      tls_client.setCACert(sd_config.mqtt_ca_pem.c_str());
      tls_client.setHandshakeTimeout(TLS_HANDSHAKE_TIMEOUT_S);
      mqtt.setClient(tls_client);
    } else {
      mqtt.setClient(plain_client);
    }
    mqtt.setServer(sd_config.mqtt_host.c_str(), sd_config.mqtt_port);
    // Default 256B is far too small — button-config already needed
    // 1024B, and calendar payloads (several events, each with a title
    // string) push that further. One shared buffer sized for the
    // worst case across all incoming payload types (broadcast messages
    // are capped well under this at BROADCAST_TEXT_MAX).
    mqtt.setBufferSize(3072);
    // PubSubClient's default (15s) is tight for this loop: weather,
    // forecast, and transit fetches are each bounded at 8s and run
    // ahead of nothing now (mqtt_client_tick() is serviced first in
    // entrance_hmi.ino), but an EPD full refresh or a slow fetch that
    // still lands mid-cycle can eat into a 15s budget fast. Confirmed
    // via the broker's own log: real disconnects logged as "exceeded
    // timeout" despite the client otherwise being healthy.
    mqtt.setKeepAlive(60);
    mqtt.setCallback(on_message);
    topics_initialized = true;
  }

  if (tls_misconfigured) return {};

  if (!mqtt.connected()) {
    unsigned long now = millis();
    if (now - last_reconnect_attempt_ms < RECONNECT_INTERVAL_MS) return {};
    last_reconnect_attempt_ms = now;
    connect();
    return {};
  }

  ha_changed_flag = false;
  calendar_changed_flag = false;
  broadcast_changed_flag = false;
  mqtt.loop();
  if (mqtt.connected() && millis() - last_telemetry_ms >= HA_TELEMETRY_INTERVAL_MS) {
    publish_telemetry();
  }
  return {ha_changed_flag, calendar_changed_flag, broadcast_changed_flag};
}

bool mqtt_client_publish_toggle(const String &entity_id) {
  if (!mqtt.connected()) return false;
  return mqtt.publish(topic_cmd(entity_id).c_str(), "toggle");
}

bool mqtt_is_connected() {
  return mqtt.connected();
}

void mqtt_client_acknowledge_broadcast() {
  String id = broadcast_message.id;  // capture before broadcast_store_acknowledge() clears it
  broadcast_store_acknowledge();

  if (!mqtt.connected()) return;

  long acked_at = 0;
  struct tm t;
  if (time_sync_get_local(t)) acked_at = (long)mktime(&t);

  // snprintf itself can't overflow this (it truncates), but sized with
  // real headroom: BROADCAST_ID_MAX (47) + MQTT_CLIENT_ID + the JSON
  // punctuation comfortably fits, so a max-length id is never cut off.
  char payload[160];
  snprintf(payload, sizeof(payload), "{\"id\":\"%s\",\"client_id\":\"%s\",\"acked_at\":%ld}",
           id.c_str(), MQTT_CLIENT_ID, acked_at);
  mqtt.publish(topic_broadcast_ack.c_str(), payload);
}
