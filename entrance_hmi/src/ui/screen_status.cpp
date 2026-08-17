#include "screen_status.h"
#include <WiFi.h>
#include <stdio.h>
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"
#include "../net/time_sync.h"
#include "../net/wifi_manager.h"

void screen_status_render() {
  EPD_ShowString(20, 20, "STATUS", 48, BLACK);

  char line[64];
  if (wifi_is_connected()) {
    IPAddress ip = WiFi.localIP();
    snprintf(line, sizeof(line), "WiFi: connected  RSSI %ddBm", WiFi.RSSI());
    EPD_ShowString(20, 90, line, 16, BLACK);
    snprintf(line, sizeof(line), "IP: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    EPD_ShowString(20, 120, line, 16, BLACK);
  } else {
    EPD_ShowString(20, 90, "WiFi: disconnected, retrying...", 16, BLACK);
  }

  struct tm timeinfo;
  if (time_sync_get_local(timeinfo)) {
    snprintf(line, sizeof(line), "Time: %04d-%02d-%02d %02d:%02d:%02d",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    EPD_ShowString(20, 160, line, 16, BLACK);
  } else {
    EPD_ShowString(20, 160, "Time: not synced yet", 16, BLACK);
  }

  EPD_ShowString(20, 230, "HOME: back", 12, BLACK);
}
