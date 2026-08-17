#include "screen_home.h"
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"

void screen_home_render() {
  EPD_ShowString(20, 20, "HOME", 48, BLACK);
  EPD_ShowString(20, 90, "M1 skeleton - real data starts at M2", 16, BLACK);
  EPD_ShowString(20, 230, "PRV/NEXT: cycle screens   HOME(hold): status", 12, BLACK);
}
