#include "wifi_manager.h"
#include <WiFi.h>
#include "../storage/sd_config.h"

namespace {

constexpr unsigned long BOOT_CONNECT_TIMEOUT_MS = 15000;
constexpr unsigned long BACKOFF_STEPS_MS[] = {0, 5000, 15000, 60000};
constexpr size_t BACKOFF_STEPS_LEN = sizeof(BACKOFF_STEPS_MS) / sizeof(BACKOFF_STEPS_MS[0]);

size_t backoff_index = 0;
unsigned long next_attempt_ms = 0;
bool was_connected = false;

}  // namespace

void wifi_manager_begin() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(sd_config.wifi_ssid.c_str(), sd_config.wifi_password.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < BOOT_CONNECT_TIMEOUT_MS) {
    delay(250);
  }

  was_connected = WiFi.status() == WL_CONNECTED;
  backoff_index = 0;
  next_attempt_ms = 0;

  if (was_connected) {
    Serial.printf("[wifi] connected, ip=%s\n", WiFi.localIP().toString().c_str());
  }
}

void wifi_manager_tick() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!was_connected) {
      backoff_index = 0;  // reset backoff after a successful (re)connect
      // IP is otherwise only visible on the physical STATUS screen —
      // useful on serial too (e.g. targeting OTA by IP when mDNS
      // doesn't reach across subnets).
      Serial.printf("[wifi] connected, ip=%s\n", WiFi.localIP().toString().c_str());
    }
    was_connected = true;
    return;
  }

  was_connected = false;
  unsigned long now = millis();
  if (now < next_attempt_ms) return;

  WiFi.disconnect();
  WiFi.reconnect();

  size_t idx = backoff_index < BACKOFF_STEPS_LEN ? backoff_index : BACKOFF_STEPS_LEN - 1;
  next_attempt_ms = now + BACKOFF_STEPS_MS[idx];
  if (backoff_index < BACKOFF_STEPS_LEN - 1) backoff_index++;
}

bool wifi_is_connected() {
  return WiFi.status() == WL_CONNECTED;
}
