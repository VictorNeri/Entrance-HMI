#include "http_client_helper.h"
#include <HTTPClient.h>
#include <esp_task_wdt.h>
#include <stdio.h>
#include <WiFiClientSecure.h>
#include "../app/error_log.h"
#include "net_log_shadow.h"

bool http_get_json(const char *url, JsonDocument &doc, uint16_t timeout_ms,
                    const JsonDocument *filter, const char *context, int *http_status) {
  if (http_status != nullptr) *http_status = 0;

  // Only ever called from the fetch worker task, which is subscribed
  // to the task watchdog. One attempt here can take up to ~3x
  // timeout_ms (connect + TLS handshake + response read), and transit
  // makes two attempts back to back — feeding here keeps each attempt
  // on its own watchdog budget.
  esp_task_wdt_reset();

  WiFiClientSecure client;
  client.setInsecure();
  // The handshake timeout is separate from the connect/read timeouts
  // below and defaults to 120s — longer than the watchdog allows.
  client.setHandshakeTimeout((timeout_ms + 999) / 1000);

  HTTPClient http;
  http.setConnectTimeout(timeout_ms);
  http.setTimeout(timeout_ms);
  if (!http.begin(client, url)) {
    return false;
  }

  int status = http.GET();
  if (http_status != nullptr) *http_status = status;
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
