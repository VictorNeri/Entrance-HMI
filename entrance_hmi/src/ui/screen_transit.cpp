#include "screen_transit.h"
#include <stdio.h>
#include "../app/app_state.h"
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"
#include "../net/transit_client.h"
#include "ui_text_unicode.h"

void screen_transit_render() {
  EPD_ShowString(20, 20, "TRANSIT", 48, BLACK);

  if (!departure_list.valid || departure_list.count == 0) {
    EPD_ShowString(20, 90, "No departures yet - waiting for first fetch", 16, BLACK);
  } else {
    // A background fetch can shrink the list between nav's clamp (against
    // the previous count) and this render, so clamp again defensively.
    uint8_t index = app_state.list_cursor < departure_list.count ? app_state.list_cursor
                                                                   : departure_list.count - 1;
    const Departure &dep = departure_list.items[index];

    char header[48];
    snprintf(header, sizeof(header), "Line %s (%s)", dep.line.c_str(), dep.transport_mode.c_str());
    EPD_ShowString(20, 90, header, 24, BLACK);

    // Destination names (e.g. "Hässelby strand") may contain Swedish
    // characters the factory font renderer can't handle raw.
    draw_utf8_string(20, 130, dep.destination.c_str(), 24, BLACK);

    char footer[48];
    snprintf(footer, sizeof(footer), "%s   (%d / %d)", dep.display.c_str(),
             app_state.list_cursor + 1, departure_list.count);
    EPD_ShowString(20, 170, footer, 16, BLACK);
  }

  EPD_ShowString(20, 230, "PRV/NEXT: scroll   OK: refresh   EXIT: home", 12, BLACK);
}
