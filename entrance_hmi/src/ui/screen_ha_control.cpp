#include "screen_ha_control.h"
#include <stdio.h>
#include "../app/app_state.h"
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"
#include "../storage/button_config_store.h"
#include "ui_text_unicode.h"

void screen_ha_control_render() {
  EPD_ShowString(20, 20, "HOME ASSISTANT", 48, BLACK);

  if (!ha_entity_list.valid || ha_entity_list.count == 0) {
    EPD_ShowString(20, 90, "No entities configured yet - waiting for MQTT config", 16, BLACK);
  } else {
    // A background config/state update can shrink the list between
    // nav's clamp (against the previous count) and this render.
    uint8_t index = app_state.list_cursor < ha_entity_list.count ? app_state.list_cursor
                                                                    : ha_entity_list.count - 1;
    const HaEntity &entity = ha_entity_list.items[index];

    draw_utf8_string(20, 90, entity.label.c_str(), 24, BLACK);

    const char *state_text = !entity.state_known ? "state: unknown"
                              : entity.state_on   ? "state: ON"
                                                   : "state: OFF";
    EPD_ShowString(20, 130, state_text, 16, BLACK);

    char footer[32];
    snprintf(footer, sizeof(footer), "(%d / %d)", index + 1, ha_entity_list.count);
    EPD_ShowString(20, 170, footer, 16, BLACK);
  }

  EPD_ShowString(20, 230, "PRV/NEXT: scroll   OK: toggle   EXIT: home", 12, BLACK);
}
