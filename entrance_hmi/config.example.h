#pragma once

// Copy this file to config.h (gitignored, never committed) and fill in
// real values. config.h holds flash-time secrets only — WiFi
// credentials, the SL station ID, and poll/rotation intervals live on
// the SD card instead (see README.md for the /config.json format), and
// the Home Assistant button/entity list arrives at runtime over MQTT.

// --- Time / NTP ---
// POSIX TZ string for Europe/Stockholm — encodes CET/CEST DST rules
// automatically, unlike a fixed UTC offset.
#define TZ_STRING "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "se.pool.ntp.org"

// --- Weather (OpenWeatherMap, free v2.5 "current weather" endpoint) ---
// lat/lon preferred over city-name lookup — unambiguous and stable.
#define OWM_API_KEY "your-openweathermap-api-key"
#define OWM_LAT "59.3293"
#define OWM_LON "18.0686"

// --- MQTT (Home Assistant integration) ---
#define MQTT_HOST "your-mqtt-broker-ip-or-hostname"
#define MQTT_PORT 1883
#define MQTT_USERNAME ""  // leave empty for anonymous/no-auth brokers
#define MQTT_PASSWORD ""
#define MQTT_CLIENT_ID "entrance-hmi"
#define MQTT_TOPIC_PREFIX "entrance-hmi"

// --- OTA (over-the-air firmware updates over WiFi) ---
// Required, not optional — ArduinoOTA with an empty password lets
// anyone on the LAN push arbitrary firmware to the device. Pick your
// own strong value; treat it like the credentials above.
#define OTA_PASSWORD "your-ota-password"
#define OTA_HOSTNAME "entrance-hmi"
