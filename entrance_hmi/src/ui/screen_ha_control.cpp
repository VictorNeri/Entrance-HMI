#include "screen_ha_control.h"
#include <stdio.h>
#include "../app/app_state.h"
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"

void screen_ha_control_render() {
  EPD_ShowString(20, 20, "HOME ASSISTANT", 48, BLACK);
  EPD_ShowString(20, 90, "Placeholder list - live MQTT entities start at M5", 16, BLACK);

  char line[48];
  snprintf(line, sizeof(line), "Entity %d / %d", app_state.list_cursor + 1,
           screen_list_length(Screen::HA_CONTROL));
  EPD_ShowString(20, 130, line, 24, BLACK);

  EPD_ShowString(20, 230, "PRV/NEXT: scroll   OK: toggle   EXIT: home", 12, BLACK);
}
