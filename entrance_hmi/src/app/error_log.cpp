#include "error_log.h"
#include <string.h>

namespace {
constexpr size_t MAX_LEN = 63;
char last_message[MAX_LEN + 1] = "";
unsigned long last_ms = 0;
}  // namespace

void error_log_set(const char *message) {
  strncpy(last_message, message, MAX_LEN);
  last_message[MAX_LEN] = '\0';
  last_ms = millis();
}

const char *error_log_get() {
  return last_message;
}

unsigned long error_log_get_ms() {
  return last_ms;
}
