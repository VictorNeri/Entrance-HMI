#pragma once
#include <Arduino.h>

struct SdConfig {
  String wifi_ssid;
  String wifi_password;
  String sl_site_id;
  String mqtt_host;
  int mqtt_port = 1883;
  String mqtt_username;
  String mqtt_password;
  String mqtt_topic_prefix = "entrance-hmi";
  String owm_api_key;
  String owm_lat;
  String owm_lon;
  // Minutes to walk from home to the station — departures sooner than
  // this are hidden on TRANSIT/HOME since they can't be caught. 0 (the
  // default, and what a missing/invalid SD card falls back to) means
  // the filter is off and every fetched departure is shown.
  unsigned long walk_time_min = 0;
  unsigned long weather_poll_interval_ms = 10UL * 60 * 1000;   // defaults match the
  unsigned long transit_poll_active_ms = 90UL * 1000;          // hardcoded constants
  unsigned long transit_poll_background_ms = 300UL * 1000;     // used verbatim if the
  unsigned long screen_rotation_interval_ms = 300UL * 1000;    // SD card/file is missing/invalid
  bool loaded_from_sd = false;
};

extern SdConfig sd_config;

// Mounts the SD card and reads /config.json. Call once, early in
// setup(), before wifi_manager_begin()/weather/MQTT (all their
// credentials now come from here). If the card fails to mount, the
// file is missing, or the JSON is invalid, sd_config keeps its
// built-in interval defaults with empty credentials — the device still
// boots and simply stays disconnected rather than failing to start.
void sd_config_load();
