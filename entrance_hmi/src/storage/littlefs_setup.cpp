#include "littlefs_setup.h"
#include <Arduino.h>
#include <LittleFS.h>

bool littlefs_setup_begin() {
  if (LittleFS.begin(false)) return true;

  Serial.println("[storage] LittleFS mount failed, formatting...");
  if (!LittleFS.begin(true)) {
    Serial.println("[storage] LittleFS format failed");
    return false;
  }
  return true;
}
