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

// --- Network debug log (TCP "serial bridge") ---
// Mirrors everything the firmware writes via Serial.print/println/
// printf out over a plain TCP socket too, so you can watch debug
// output without physical USB access once the panel is mounted.
// Connect with `nc <device-ip> <NET_LOG_PORT>` or any telnet client.
// Not authenticated — anyone who can reach the device on the LAN can
// connect and read it. It's read-only (unlike OTA_PASSWORD, which
// gates writing new firmware), so the risk is exposure of debug
// output, not device control — change the port, or don't expose this
// device's network to anyone you don't trust, if that's a concern.
#define NET_LOG_PORT 23
