#pragma once
#include <Arduino.h>

struct SdConfig {
  String wifi_ssid;
  String wifi_password;
  String sl_site_id;
  unsigned long weather_poll_interval_ms = 10UL * 60 * 1000;   // defaults match the
  unsigned long transit_poll_active_ms = 90UL * 1000;          // hardcoded constants
  unsigned long transit_poll_background_ms = 300UL * 1000;     // used verbatim if the
  unsigned long screen_rotation_interval_ms = 300UL * 1000;    // SD card/file is missing/invalid
  bool loaded_from_sd = false;
};

extern SdConfig sd_config;

// Mounts the SD card and reads /config.json. Call once, early in
// setup(), before wifi_manager_begin() (WiFi credentials now come from
// here). If the card fails to mount, the file is missing, or the JSON
// is invalid, sd_config keeps its built-in interval defaults with
// empty wifi_ssid/wifi_password/sl_site_id — the device still boots
// and simply stays WiFi-disconnected rather than failing to start.
void sd_config_load();
