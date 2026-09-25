#pragma once
#include <Arduino.h>

// TCP "serial bridge": every Serial.print/println/printf call in a file
// that includes net_log_shadow.h (not this header directly) instead of
// writing straight to Serial, also streams to a plain-text TCP socket —
// so debug output is visible over the LAN (`nc <device-ip> <port>`, or
// any telnet client) without physical USB access. Useful now that the
// panel is wall-mounted. Hardware Serial keeps working exactly as
// before regardless of whether anything is connected on the TCP side.
//
// Only 3 Serial methods are used anywhere in this codebase — begin,
// println, printf (confirmed by grep) — and NetLogPrint forwards all
// of them (println/printf via the inherited Print:: implementations,
// which funnel through write() below) to the real hardware Serial in
// addition to any connected TCP client.
class NetLogPrint : public Print {
 public:
  size_t write(uint8_t c) override;
  size_t write(const uint8_t *buffer, size_t size) override;

  // Forwards to the real HardwareSerial::begin — still required so USB
  // serial keeps working even before WiFi/the TCP listener are up.
  void begin(unsigned long baud);
};

extern NetLogPrint net_log;

// Call every loop() iteration. No-op while WiFi is down; lazily starts
// the TCP listener on the first tick after WiFi connects (mirrors
// ota_manager_tick()'s pattern), then accepts new client connections
// every tick after — single client at a time, a new connection
// replaces whatever was there before rather than being refused.
void net_log_tick();
