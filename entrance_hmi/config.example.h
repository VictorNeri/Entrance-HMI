#pragma once

// Copy this file to config.h (gitignored, never committed) and fill in
// real values. config.h holds flash-time secrets/tunables only — the
// Home Assistant button/entity list is intentionally NOT here; it's
// runtime-editable over MQTT starting at M5.

// --- WiFi ---
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"

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

// --- Transit (SL Transport API, free/keyless) ---
// Find your own stop's site ID: fetch
// https://transport.integration.sl.se/v1/sites and search the JSON by
// stop name (or use trafiklab.se's stop lookup). 9001 = T-Centralen,
// a placeholder for schema testing — replace with your actual nearest
// stop before relying on this.
#define SL_SITE_ID "9001"

// --- MQTT (Home Assistant integration) ---
#define MQTT_HOST "your-mqtt-broker-ip-or-hostname"
#define MQTT_PORT 1883
#define MQTT_USERNAME ""  // leave empty for anonymous/no-auth brokers
#define MQTT_PASSWORD ""
#define MQTT_CLIENT_ID "entrance-hmi"
#define MQTT_TOPIC_PREFIX "entrance-hmi"
