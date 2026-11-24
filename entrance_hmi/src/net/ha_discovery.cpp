#include "ha_discovery.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_timer.h>
#include "../../config.h"
#include "../app/error_log.h"
#include "../app/fw_version.h"
#include "../app/night_mode.h"
#include "../storage/sd_config.h"
#include "../ui/ui_stale.h"
#include "transit_client.h"
#include "weather_client.h"
#include "wifi_manager.h"

namespace {

// Minutes since a dataset was last fetched, or -1 if never.
long age_min(bool valid, unsigned long fetched_at_ms) {
  if (!valid) return -1;
  return (long)((millis() - fetched_at_ms) / 60000UL);
}

// Same thresholds as the on-screen "25m ago" badges. Before the first
// fetch, the age counts from boot, so a fresh boot isn't reported stale
// for the first poll interval or two.
bool overdue(bool valid, unsigned long fetched_at_ms, unsigned long stale_after_ms) {
  unsigned long since = valid ? fetched_at_ms : 0;
  return millis() - since > stale_after_ms;
}

void add_common(JsonObject c, const char *key, const char *platform, const char *name) {
  c["p"] = platform;
  c["name"] = name;
  c["uniq_id"] = String(MQTT_CLIENT_ID) + "_" + key;
  c["ent_cat"] = "diagnostic";
}

}  // namespace

String ha_discovery_config_topic() {
  return sd_config.ha_discovery_prefix + "/device/" + MQTT_CLIENT_ID + "/config";
}

String ha_discovery_config_payload(const String &telemetry_topic, const String &status_topic) {
  JsonDocument doc;

  JsonObject dev = doc["dev"].to<JsonObject>();
  dev["ids"].to<JsonArray>().add(MQTT_CLIENT_ID);
  dev["name"] = "Entrance HMI";
  dev["mf"] = "Elecrow";
  dev["mdl"] = "CrowPanel ESP32-S3 5.79\" e-paper";
  dev["sw"] = FW_VERSION;

  JsonObject origin = doc["o"].to<JsonObject>();
  origin["name"] = "Entrance-HMI firmware";
  origin["sw"] = FW_VERSION;
  origin["url"] = "https://github.com/VictorNeri/Entrance-HMI";

  doc["stat_t"] = telemetry_topic;
  doc["avty_t"] = status_topic;  // "online"/"offline" — HA's defaults

  JsonObject cmps = doc["cmps"].to<JsonObject>();
  JsonObject c;

  c = cmps["rssi"].to<JsonObject>();
  add_common(c, "rssi", "sensor", "Wi-Fi signal");
  c["dev_cla"] = "signal_strength";
  c["unit_of_meas"] = "dBm";
  c["stat_cla"] = "measurement";
  c["val_tpl"] = "{{ value_json.rssi }}";

  c = cmps["heap"].to<JsonObject>();
  add_common(c, "heap", "sensor", "Free heap");
  c["dev_cla"] = "data_size";
  c["unit_of_meas"] = "B";
  c["stat_cla"] = "measurement";
  c["val_tpl"] = "{{ value_json.heap }}";

  c = cmps["uptime"].to<JsonObject>();
  add_common(c, "uptime", "sensor", "Uptime");
  c["dev_cla"] = "duration";
  c["unit_of_meas"] = "s";
  c["stat_cla"] = "total_increasing";
  c["val_tpl"] = "{{ value_json.uptime_s }}";

  c = cmps["ip"].to<JsonObject>();
  add_common(c, "ip", "sensor", "IP address");
  c["val_tpl"] = "{{ value_json.ip }}";

  c = cmps["fw"].to<JsonObject>();
  add_common(c, "fw", "sensor", "Firmware");
  c["val_tpl"] = "{{ value_json.fw }}";

  c = cmps["last_error"].to<JsonObject>();
  add_common(c, "last_error", "sensor", "Last error");
  c["val_tpl"] = "{{ value_json.last_error }}";

  c = cmps["night"].to<JsonObject>();
  add_common(c, "night", "binary_sensor", "Night mode");
  c["val_tpl"] = "{{ 'ON' if value_json.night else 'OFF' }}";

  // Not diagnostic: this is the one to build an alert on.
  c = cmps["stale"].to<JsonObject>();
  add_common(c, "stale", "binary_sensor", "Data stale");
  c.remove("ent_cat");
  c["dev_cla"] = "problem";
  c["val_tpl"] = "{{ 'ON' if value_json.stale else 'OFF' }}";

  // -1 means "never fetched" and renders as unknown in HA.
  c = cmps["weather_age"].to<JsonObject>();
  add_common(c, "weather_age", "sensor", "Weather data age");
  c["dev_cla"] = "duration";
  c["unit_of_meas"] = "min";
  c["stat_cla"] = "measurement";
  c["val_tpl"] = "{{ value_json.weather_age_min if value_json.weather_age_min >= 0 else None }}";

  c = cmps["transit_age"].to<JsonObject>();
  add_common(c, "transit_age", "sensor", "Transit data age");
  c["dev_cla"] = "duration";
  c["unit_of_meas"] = "min";
  c["stat_cla"] = "measurement";
  c["val_tpl"] = "{{ value_json.transit_age_min if value_json.transit_age_min >= 0 else None }}";

  String out;
  serializeJson(doc, out);
  return out;
}

String ha_telemetry_payload() {
  JsonDocument doc;
  doc["rssi"] = wifi_get_rssi();
  doc["heap"] = ESP.getFreeHeap();
  // esp_timer is 64-bit microseconds, so this doesn't wrap at 49.7 days
  // like millis() does.
  doc["uptime_s"] = (uint32_t)(esp_timer_get_time() / 1000000LL);
  doc["ip"] = WiFi.localIP().toString();
  doc["fw"] = FW_VERSION;
  doc["last_error"] = error_log_get_ms() == 0 ? "none" : error_log_get();
  doc["night"] = night_mode_active();

  doc["weather_age_min"] = age_min(weather_data.valid, weather_data.fetched_at_ms);
  doc["transit_age_min"] = age_min(departure_list.valid, departure_list.fetched_at_ms);

  // Only sources that are configured count. Transit doesn't during night
  // hours: background SL polling pauses then, so old departures are
  // expected, not a fault.
  bool stale = false;
  if (sd_config.owm_api_key.length() > 0 &&
      overdue(weather_data.valid, weather_data.fetched_at_ms, ui_weather_stale_after_ms())) {
    stale = true;
  }
  if (sd_config.sl_site_id.length() > 0 && !night_mode_in_hours() &&
      overdue(departure_list.valid, departure_list.fetched_at_ms, ui_transit_stale_after_ms())) {
    stale = true;
  }
  doc["stale"] = stale;

  String out;
  serializeJson(doc, out);
  return out;
}
