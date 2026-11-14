#include "error_log.h"
#include <string.h>
#include <freertos/FreeRTOS.h>

namespace {
constexpr size_t MAX_LEN = 63;
// Written from both loop() (MQTT, OTA) and the fetch worker task
// (HTTP failures), so every access goes through this spinlock — the
// critical sections are a bounded 64-byte copy, short enough for a
// portMUX.
portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
char last_message[MAX_LEN + 1] = "";
unsigned long last_ms = 0;
// Stable copy handed out by error_log_get(), so the caller never reads
// last_message while another task is overwriting it.
char snapshot[MAX_LEN + 1] = "";
}  // namespace

void error_log_set(const char *message) {
  portENTER_CRITICAL(&lock);
  strncpy(last_message, message, MAX_LEN);
  last_message[MAX_LEN] = '\0';
  last_ms = millis();
  portEXIT_CRITICAL(&lock);
}

const char *error_log_get() {
  portENTER_CRITICAL(&lock);
  memcpy(snapshot, last_message, sizeof(snapshot));
  portEXIT_CRITICAL(&lock);
  return snapshot;
}

unsigned long error_log_get_ms() {
  portENTER_CRITICAL(&lock);
  unsigned long ms = last_ms;
  portEXIT_CRITICAL(&lock);
  return ms;
}
