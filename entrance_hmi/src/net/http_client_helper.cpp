#include "http_client_helper.h"
#include <HTTPClient.h>
#include <stdio.h>
#include <WiFiClientSecure.h>
#include "../app/error_log.h"

bool http_get_json(const char *url, JsonDocument &doc, uint16_t timeout_ms,
                    const JsonDocument *filter, const char *context) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setConnectTimeout(timeout_ms);
  http.setTimeout(timeout_ms);
  if (!http.begin(client, url)) {
    return false;
  }

  int status = http.GET();
  bool ok = false;
  if (status == HTTP_CODE_OK) {
    // Deserialize straight from the response stream rather than
    // buffering the whole body into a String first — smaller peak heap.
    DeserializationError err =
        filter != nullptr ? deserializeJson(doc, http.getStream(),
                                             DeserializationOption::Filter(*filter))
                           : deserializeJson(doc, http.getStream());
    ok = err == DeserializationError::Ok;
    if (!ok) {
      Serial.printf("[http] GET %s: HTTP 200 but JSON parse failed: %s\n", url, err.c_str());
      char msg[64];
      snprintf(msg, sizeof(msg), "%s: JSON parse failed (%s)", context, err.c_str());
      error_log_set(msg);
    }
  } else {
    Serial.printf("[http] GET %s: HTTP status %d\n", url, status);
    char msg[64];
    snprintf(msg, sizeof(msg), "%s: HTTP status %d", context, status);
    error_log_set(msg);
  }

  http.end();
  return ok;
}
