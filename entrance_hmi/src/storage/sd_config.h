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
  // MQTT over TLS (mqtts). Off by default. When on, the broker's
  // certificate must chain to the PEM CA in /mqtt_ca.pem on the SD card
  // and match mqtt_host (a hostname, or an IP listed as an IP SAN); with
  // no usable CA file the client refuses to connect rather than falling
  // back to plaintext or an unverified connection.
  bool mqtt_tls = false;
  String mqtt_ca_pem;
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
  // How long idle (no button press) on any screen other than HOME
  // before auto-returning to HOME. Kept the same field name/JSON key
  // across the behavior change below — it's still "how long idle
  // before something automatic happens," just home-return instead of
  // cycling through screens.
  unsigned long screen_rotation_interval_ms = 12UL * 1000;      // SD card/file is missing/invalid
  // Night mode window, local hours 0-23 (may wrap midnight) — see
  // app/night_mode.h. Equal values turn night mode off; an out-of-range
  // value in config.json falls back to the default.
  uint8_t night_start_hour = 23;
  uint8_t night_end_hour = 6;
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
