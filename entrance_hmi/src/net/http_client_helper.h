#pragma once
#include <ArduinoJson.h>
#include <Arduino.h>

// HTTPS GET, parsed straight into `doc`. Returns true only on HTTP 200
// plus valid JSON — on failure `doc` may be partially populated, so
// check the return value rather than doc's contents.
//
// `root_ca_pem` is the PEM root(s) to trust for this host (see
// root_certs.h) — required: the server's chain and hostname are
// verified against it, and there's deliberately no insecure fallback.
// A verification failure returns false with a "TLS ..." error_log
// entry. Only one such call should be in
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
//
// `http_status`, if given, receives the HTTP status code (negative for
// an HTTPClient transport error, 0 if the request never started) — so
// callers can react to specific codes, e.g. back off on 429.
bool http_get_json(const char *url, const char *root_ca_pem, JsonDocument &doc,
                    uint16_t timeout_ms = 8000,
                    const JsonDocument *filter = nullptr, const char *context = "http",
                    int *http_status = nullptr);
constexpr int HTTP_STATUS_TOO_MANY_REQUESTS = 429;
