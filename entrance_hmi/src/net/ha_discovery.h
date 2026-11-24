#pragma once
#include <Arduino.h>

// Home Assistant MQTT discovery: the panel shows up in HA as a device
// ("Entrance HMI") with diagnostic sensors — Wi-Fi signal, free heap,
// uptime, IP, firmware, night mode, last error, and how old the weather
// and transit data on screen is, plus a "Data stale" problem sensor to
// alert on. Availability follows the existing <prefix>/status LWT.
//
// One retained device-discovery message describes every entity; the
// values come from one JSON telemetry message that mqtt_client
// publishes on connect and every HA_TELEMETRY_INTERVAL_MS.

constexpr unsigned long HA_TELEMETRY_INTERVAL_MS = 60UL * 1000;

// <discovery prefix>/device/<MQTT_CLIENT_ID>/config
String ha_discovery_config_topic();

// The retained device-discovery payload (about 2.2 KB, close to the
// MQTT client's 3 KB buffer, so it's sent with the streaming publish
// and adding entities can't silently push it over).
String ha_discovery_config_payload(const String &telemetry_topic, const String &status_topic);

// Current telemetry as JSON. Main thread only (reads the UI's data
// globals and error_log_get()).
String ha_telemetry_payload();
