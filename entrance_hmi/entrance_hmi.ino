// M1: button-driven navigation across the 5 top-level screens. Each
// screen is a placeholder — real data is wired in starting at M2.
#include "src/app/app_state.h"
#include "src/app/nav.h"
#include "src/input/buttons.h"
#include "src/ui/ui_common.h"

void setup() {
  Serial.begin(115200);
  ui_init();
  buttons_init();
  ui_render_current_screen(true);
  Serial.println("M1 navigation skeleton ready.");
}

void loop() {
  ButtonEvent event = buttons_poll();
  RedrawKind redraw = nav_handle_event(event);
  if (redraw != RedrawKind::NONE) {
    ui_render_current_screen(redraw == RedrawKind::FULL);
  }
}
