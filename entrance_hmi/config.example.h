#pragma once

// Copy this file to config.h (gitignored, never committed) and fill in
// real values. config.h holds only what's NOT on the SD card: NTP/
// timezone (rarely changes) and the OTA password (deliberately kept
// off the SD card — see below). Everything else — WiFi credentials,
// the SL station ID, MQTT broker/credentials, the OpenWeatherMap key/
// coordinates, the walk-to-station filter, and poll/rotation intervals
// — lives on the SD card instead (see README.md for the /config.json
// format), and the Home Assistant button/entity list arrives at
// runtime over MQTT.

// --- Time / NTP ---
// POSIX TZ string for Europe/Stockholm — encodes CET/CEST DST rules
// automatically, unlike a fixed UTC offset.
#define TZ_STRING "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "se.pool.ntp.org"

// --- MQTT client identity (not a secret, rarely needs tuning — the
// broker/credentials/topic prefix are on the SD card) ---
#define MQTT_CLIENT_ID "entrance-hmi"

// --- OTA (over-the-air firmware updates over WiFi) ---
// Required, not optional — ArduinoOTA with an empty password lets
// anyone on the LAN push arbitrary firmware to the device. Pick your
// own strong value. Deliberately NOT on the SD card, unlike everything
// else above: this is the credential that gates who can push new
// firmware, so it stays off a medium anyone can pull out and read.
#define OTA_PASSWORD "your-ota-password"
#define OTA_HOSTNAME "entrance-hmi"
