#include "http_client_helper.h"
#include <HTTPClient.h>
#include <esp_task_wdt.h>
#include <stdio.h>
#include <string.h>
#include <WiFiClientSecure.h>
#include "../app/error_log.h"
#include "net_log_shadow.h"

namespace {

// The URL as it's safe to log: scheme/host/path only. Query strings
// here carry the OpenWeatherMap API key (appid=) and home coordinates,
// and logs go out over net_log's unauthenticated TCP port as well as
// USB serial — so the whole query is dropped rather than picking out
// known-sensitive parameters one by one.
String loggable_url(const char *url) {
  const char *query = strchr(url, '?');
  if (query == nullptr) return String(url);
  String out;
  out.reserve(query - url + 4);
  out.concat(url, query - url);
  out += "?...";
  return out;
}

}  // namespace

bool http_get_json(const char *url, const char *root_ca_pem, JsonDocument &doc, uint16_t timeout_ms,
                    const JsonDocument *filter, const char *context, int *http_status) {
  if (http_status != nullptr) *http_status = 0;

  // Only ever called from the fetch worker task, which is subscribed
  // to the task watchdog. One attempt here can take up to ~3x
  // timeout_ms (connect + TLS handshake + response read), and transit
  // makes two attempts back to back — feeding here keeps each attempt
  // on its own watchdog budget.
  esp_task_wdt_reset();

  WiFiClientSecure client;
  client.setCACert(root_ca_pem);
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
      Serial.printf("[http] GET %s: HTTP 200 but JSON parse failed: %s\n",
                    loggable_url(url).c_str(), err.c_str());
      char msg[64];
      snprintf(msg, sizeof(msg), "%s: JSON parse failed (%s)", context, err.c_str());
      error_log_set(msg);
    }
  } else if (status < 0) {
    // Transport-level failure (HTTPClient's negative codes). With CA
    // pinning, a certificate the pinned roots don't vouch for lands
    // here too — so pull mbedtls's own reason, which tells "TLS:
    // certificate verify failed" apart from a plain connect/timeout.
    char tls_err[48] = "";
    client.lastError(tls_err, sizeof(tls_err));
    Serial.printf("[http] GET %s: %s (%d), TLS: %s\n", loggable_url(url).c_str(),
                  HTTPClient::errorToString(status).c_str(),
                  status, tls_err[0] != '\0' ? tls_err : "no error");
    char msg[64];
    if (tls_err[0] != '\0') {
      snprintf(msg, sizeof(msg), "%s: TLS %s", context, tls_err);
    } else {
      snprintf(msg, sizeof(msg), "%s: HTTP status %d", context, status);
    }
    error_log_set(msg);
  } else {
    Serial.printf("[http] GET %s: HTTP status %d\n", loggable_url(url).c_str(), status);
    char msg[64];
    snprintf(msg, sizeof(msg), "%s: HTTP status %d", context, status);
    error_log_set(msg);
  }

  http.end();
  return ok;
}
