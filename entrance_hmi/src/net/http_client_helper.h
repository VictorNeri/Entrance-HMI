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
bool http_get_json(const char *url, JsonDocument &doc, uint16_t timeout_ms = 8000);
