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
