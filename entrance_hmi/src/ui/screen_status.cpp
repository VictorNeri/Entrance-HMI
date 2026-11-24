#include "screen_status.h"
#include <WiFi.h>
#include <stdio.h>
#include "../app/error_log.h"
#include "../app/fw_version.h"
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"
#include "../net/time_sync.h"
#include "../net/wifi_manager.h"
#include "ui_chrome.h"

namespace {
// 32px per size-24 line: five lines (66..218) stay above the hint row
// at UI_CONTENT_HINT_Y (228).
constexpr uint16_t LINE_H = 32;
}  // namespace

void screen_status_render() {
  // At size 24 (12px/char), the un-truncated "Error: " + 63-char
  // message + "(Ns ago)" (~80 chars) would run to ~960px — past even
  // the panel's true pixel buffer, not just the visible content area.
  // Bounded to 55 chars (~660px) to stay safely inside UI_CONTENT_RIGHT.
  char line[56];
  if (wifi_is_connected()) {
    IPAddress ip = WiFi.localIP();
    snprintf(line, sizeof(line), "WiFi: connected  RSSI %ddBm", WiFi.RSSI());
    EPD_ShowString(UI_CONTENT_LEFT, UI_CONTENT_BODY_TOP, line, 24, BLACK);
    snprintf(line, sizeof(line), "IP: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    EPD_ShowString(UI_CONTENT_LEFT, UI_CONTENT_BODY_TOP + 1 * LINE_H, line, 24, BLACK);
  } else {
    EPD_ShowString(UI_CONTENT_LEFT, UI_CONTENT_BODY_TOP, "WiFi: disconnected, retrying...", 24,
                   BLACK);
  }

  struct tm timeinfo;
  if (time_sync_get_local(timeinfo)) {
    snprintf(line, sizeof(line), "Time: %04d-%02d-%02d %02d:%02d:%02d", timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min,
             timeinfo.tm_sec);
    EPD_ShowString(UI_CONTENT_LEFT, UI_CONTENT_BODY_TOP + 2 * LINE_H, line, 24, BLACK);
  } else {
    EPD_ShowString(UI_CONTENT_LEFT, UI_CONTENT_BODY_TOP + 2 * LINE_H, "Time: not synced yet", 24, BLACK);
  }

  if (error_log_get_ms() == 0) {
    EPD_ShowString(UI_CONTENT_LEFT, UI_CONTENT_BODY_TOP + 3 * LINE_H, "No errors", 24, BLACK);
  } else {
    unsigned long ago_sec = (millis() - error_log_get_ms()) / 1000;
    snprintf(line, sizeof(line), "Error: %s (%lus ago)", error_log_get(), ago_sec);
    EPD_ShowString(UI_CONTENT_LEFT, UI_CONTENT_BODY_TOP + 3 * LINE_H, line, 24, BLACK);
  }

  snprintf(line, sizeof(line), "Firmware: %s  built %s", FW_VERSION, FW_BUILT);
  EPD_ShowString(UI_CONTENT_LEFT, UI_CONTENT_BODY_TOP + 4 * LINE_H, line, 24, BLACK);
}
