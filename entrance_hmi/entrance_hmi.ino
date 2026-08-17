// M2: WiFi connect/reconnect + NTP time sync, surfaced on the STATUS
// screen. Other screens remain M1 placeholders.
#include "src/app/app_state.h"
#include "src/app/nav.h"
#include "src/input/buttons.h"
#include "src/net/time_sync.h"
#include "src/net/wifi_manager.h"
#include "src/ui/ui_common.h"

void setup() {
  Serial.begin(115200);
  ui_init();
  buttons_init();
  wifi_manager_begin();
  ui_render_current_screen(true);
  Serial.println("M2 WiFi + NTP ready.");
}

void loop() {
  wifi_manager_tick();
  time_sync_tick();

  ButtonEvent event = buttons_poll();
  RedrawKind redraw = nav_handle_event(event);
  if (redraw != RedrawKind::NONE) {
    ui_render_current_screen(redraw == RedrawKind::FULL);
  }
}
