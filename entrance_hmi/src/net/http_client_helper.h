#pragma once
#include <ArduinoJson.h>
#include <Arduino.h>

// HTTPS GET, parsed straight into `doc`. Returns true only on HTTP 200
// plus valid JSON — on failure `doc` may be partially populated, so
// check the return value rather than doc's contents.
//
// Uses WiFiClientSecure::setInsecure() (no cert pinning) — a deliberate
// tradeoff for read-only public APIs on a home LAN device; see the
// project plan for the rationale. Only one such call should be in
// flight at a time: the WiFiClientSecure instance is the single
// biggest transient memory consumer in this firmware.
// `filter`, if given, is passed to ArduinoJson's DeserializationOption::Filter
// so only the fields it names are actually parsed/stored — needed for
// endpoints that return more data than we want to hold in memory at
// once (e.g. SL's departures response, tens of KB with fields we
// don't use).
//
// `context`, if given, tags the caller (e.g. "weather", "transit") in
// both the serial diagnostic and the error_log entry shown on the
// STATUS screen, so a failure there is traceable to which fetch it was.
bool http_get_json(const char *url, JsonDocument &doc, uint16_t timeout_ms = 8000,
                    const JsonDocument *filter = nullptr, const char *context = "http");
