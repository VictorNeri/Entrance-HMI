#include "screen_ha_control.h"
#include <stdio.h>
#include "../app/app_state.h"
#include "../app/ha_pending.h"
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"
#include "../storage/button_config_store.h"
#include "ui_chrome.h"
#include "ui_text_unicode.h"

void screen_ha_control_render() {
  if (!ha_entity_list.valid || ha_entity_list.count == 0) {
    EPD_ShowString(UI_CONTENT_LEFT, UI_CONTENT_BODY_TOP,
                   "No entities configured yet - waiting for MQTT config", 24, BLACK);
  } else {
    // A background config/state update can shrink the list between
    // nav's clamp (against the previous count) and this render.
    uint8_t index = app_state.list_cursor < ha_entity_list.count ? app_state.list_cursor
                                                                    : ha_entity_list.count - 1;
    const HaEntity &entity = ha_entity_list.items[index];

    // MQTT-supplied labels are unbounded in length; at size 48 (24px/
    // char) anything past ~26 chars runs past UI_CONTENT_RIGHT toward
    // the rail, and EPD_ShowChar has no bounds checking of its own —
    // truncate defensively rather than trust the input.
    char label[27];
    snprintf(label, sizeof(label), "%s", entity.label.c_str());
    draw_utf8_string(UI_CONTENT_LEFT, UI_CONTENT_BODY_TOP, label, 48, BLACK);

    const char *state_text = !entity.state_known ? "state: unknown"
                              : entity.state_on   ? "state: ON"
                                                   : "state: OFF";
    // Pending-toggle note after the (still HA-confirmed) state.
    const char *note = nullptr;
    switch (ha_pending_status(entity)) {
      case HaPendingStatus::WAITING:
        note = "  - sent, waiting for HA...";
        break;
      case HaPendingStatus::TIMED_OUT:
        note = "  - no response from HA";
        break;
      case HaPendingStatus::NOT_SENT:
        note = "  - not sent (MQTT offline)";
        break;
      case HaPendingStatus::NONE:
        break;
    }
    char state_line[64];
    snprintf(state_line, sizeof(state_line), "%s%s", state_text, note != nullptr ? note : "");
    EPD_ShowString(UI_CONTENT_LEFT, UI_CONTENT_BODY_TOP + 50, state_line, 24, BLACK);

    char footer[32];
    snprintf(footer, sizeof(footer), "(%d / %d)", index + 1, ha_entity_list.count);
    EPD_ShowString(UI_CONTENT_LEFT, UI_CONTENT_BODY_TOP + 82, footer, 24, BLACK);
  }

  EPD_ShowString(UI_CONTENT_LEFT, UI_CONTENT_HINT_Y, "OK: toggle", 16, BLACK);
}
