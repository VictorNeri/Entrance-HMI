#include "net_log.h"
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "../../config.h"
#include "wifi_manager.h"

// Deliberately does NOT include net_log_shadow.h — this file needs the
// real hardware Serial (NetLogPrint::write below forwards to it), not
// the shadowed one every other file gets.

NetLogPrint net_log;

namespace {
WiFiServer server(NET_LOG_PORT);
WiFiClient client;
bool started = false;

// Logging happens from both loop() and the fetch worker task, while
// net_log_tick() (loop) can replace or stop `client` at any time — so
// every use of `client` holds this. It also keeps one printf()'s output
// in one piece instead of interleaving with the other task's.
// Created in begin(), which setup() calls before anything else logs.
SemaphoreHandle_t lock = nullptr;

void take() {
  if (lock != nullptr) xSemaphoreTake(lock, portMAX_DELAY);
}
void give() {
  if (lock != nullptr) xSemaphoreGive(lock);
}
}  // namespace

size_t NetLogPrint::write(uint8_t c) {
  return write(&c, 1);
}

size_t NetLogPrint::write(const uint8_t *buffer, size_t size) {
  take();
  size_t n = Serial.write(buffer, size);
  if (client && client.connected()) client.write(buffer, size);
  give();
  return n;
}

void NetLogPrint::begin(unsigned long baud) {
  if (lock == nullptr) lock = xSemaphoreCreateMutex();
  Serial.begin(baud);
}

void net_log_tick() {
  if (!wifi_is_connected()) return;

  if (!started) {
    server.begin();
    server.setNoDelay(true);
    started = true;
    Serial.printf("[net-log] listening on :%d\n", NET_LOG_PORT);
  }

  take();
  if (server.hasClient()) {
    // Single client, last one wins — a stray session left open
    // elsewhere (a forgotten terminal, a dropped connection the far
    // end never noticed) shouldn't block picking up a new one.
    if (client) client.stop();
    client = server.accept();  // available() still works but is deprecated in this core
    Serial.printf("[net-log] client connected: %s\n", client.remoteIP().toString().c_str());
    // Deliberately not checking client.connected() below on this same
    // tick — a freshly-accepted WiFiClient can read back as not-yet-
    // connected for a moment, and doing both checks unconditionally
    // closed every connection within ~100ms of accepting it, before a
    // single byte ever went out (caught live against real hardware:
    // the debug port accepted every connection and then silently
    // killed it near-instantly, every time). Stale-client cleanup
    // below only runs on ticks where nothing was just accepted.
  } else if (client && !client.connected()) {
    client.stop();
  }
  give();
}
