#include "screen_status.h"
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"

void screen_status_render() {
  EPD_ShowString(20, 20, "STATUS", 48, BLACK);
  EPD_ShowString(20, 90, "Placeholder - WiFi/MQTT diagnostics start at M2", 16, BLACK);
  EPD_ShowString(20, 230, "HOME: back", 12, BLACK);
}
