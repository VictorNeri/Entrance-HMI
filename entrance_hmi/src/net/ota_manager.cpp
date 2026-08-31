#include "ota_manager.h"
#include <ArduinoOTA.h>
#include <stdio.h>
#include "../../config.h"
#include "../app/error_log.h"
#include "wifi_manager.h"

namespace {

bool ota_started = false;

const char *error_reason(ota_error_t error) {
  switch (error) {
    case OTA_AUTH_ERROR:
      return "auth failed";
    case OTA_BEGIN_ERROR:
      return "begin failed";
    case OTA_CONNECT_ERROR:
      return "connect failed";
    case OTA_RECEIVE_ERROR:
      return "receive failed";
    case OTA_END_ERROR:
      return "end failed";
    default:
      return "unknown error";
  }
}

void start() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    const char *type = ArduinoOTA.getCommand() == U_FLASH ? "sketch" : "filesystem";
    Serial.printf("[ota] update starting (%s)\n", type);
  });
  ArduinoOTA.onEnd([]() { Serial.println("[ota] update complete, rebooting"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[ota] %u%%\n", total > 0 ? (progress * 100) / total : 0);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    const char *reason = error_reason(error);
    Serial.printf("[ota] error: %s\n", reason);
    char msg[64];
    snprintf(msg, sizeof(msg), "ota: %s", reason);
    error_log_set(msg);
  });

  ArduinoOTA.begin();
  Serial.printf("[ota] ready, hostname=%s.local\n", OTA_HOSTNAME);
}

}  // namespace

void ota_manager_tick() {
  if (!wifi_is_connected()) return;

  if (!ota_started) {
    start();
    ota_started = true;
  }

  ArduinoOTA.handle();
}
