#include "screen_transit.h"
#include <stdio.h>
#include "../app/app_state.h"
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"

void screen_transit_render() {
  EPD_ShowString(20, 20, "TRANSIT", 48, BLACK);
  EPD_ShowString(20, 90, "Placeholder list - live SL data starts at M4", 16, BLACK);

  char line[48];
  snprintf(line, sizeof(line), "Departure %d / %d", app_state.list_cursor + 1,
           screen_list_length(Screen::TRANSIT));
  EPD_ShowString(20, 130, line, 24, BLACK);

  EPD_ShowString(20, 230, "PRV/NEXT: scroll   EXIT: home", 12, BLACK);
}
