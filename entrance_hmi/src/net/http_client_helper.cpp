#include "http_client_helper.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

bool http_get_json(const char *url, JsonDocument &doc, uint16_t timeout_ms,
                    const JsonDocument *filter) {
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
  }

  http.end();
  return ok;
}
