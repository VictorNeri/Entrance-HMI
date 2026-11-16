#include "net_log.h"
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <lwip/sockets.h>
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

// Set when a client is found disconnected (closed, reset, or dead);
// reported to the next client on
// connect, since the one it happened to can't be told.
unsigned long last_drop_up_ms = 0;
bool have_drop = false;

// NetworkClient's operator bool IS connected(), so `client` can't be
// used to ask "is there a socket to clean up" — once connected() has
// seen an error it reads false while the socket is still open. fd()
// is the real "holds a socket" test.
bool has_socket() {
  return client.fd() >= 0;
}

// Without keepalive a peer that vanished (WiFi drop, laptop asleep)
// holds the single slot until a new client replaces it. Probe after
// 30s idle, every 10s, give up after 3 misses.
void enable_keepalive(int fd) {
  int on = 1, idle = 30, intvl = 10, cnt = 3;
  setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
  setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
  setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));
  setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));
}

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
  if (has_socket() && client.connected()) client.write(buffer, size);
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
    if (has_socket()) client.stop();
    client = server.accept();  // available() still works but is deprecated in this core
    if (has_socket()) enable_keepalive(client.fd());
    Serial.printf("[net-log] client connected: %s\n", client.remoteIP().toString().c_str());
    char banner[96];
    int n = snprintf(banner, sizeof(banner), "[net-log] connected, up=%lums\n", millis());
    if (have_drop) {
      n += snprintf(banner + n, sizeof(banner) - n, "[net-log] previous client disconnected at up=%lums\n",
                    last_drop_up_ms);
      have_drop = false;
    }
    client.write(reinterpret_cast<const uint8_t *>(banner), strlen(banner));
    // Deliberately not checking client.connected() below on this same
    // tick — a freshly-accepted WiFiClient can read back as not-yet-
    // connected for a moment, and doing both checks unconditionally
    // closed every connection within ~100ms of accepting it, before a
    // single byte ever went out (caught live against real hardware:
    // the debug port accepted every connection and then silently
    // killed it near-instantly, every time). Stale-client cleanup
    // below only runs on ticks where nothing was just accepted.
  } else if (has_socket() && !client.connected()) {
    // Close it for real: before this check used `client`, which can
    // never be true here (see has_socket()), so a client that hit one
    // socket error stayed open but silent forever — the far end saw a
    // healthy, idle connection and no more output.
    client.stop();
    last_drop_up_ms = millis();
    have_drop = true;
    Serial.printf("[net-log] client disconnected\n");
  }
  give();
}
