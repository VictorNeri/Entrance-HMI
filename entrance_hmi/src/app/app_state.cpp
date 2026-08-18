#include "app_state.h"
#include "../net/transit_client.h"

AppState app_state;

namespace {
// HA_CONTROL's entity list arrives over MQTT starting at M5; until
// then it's a fixed placeholder count purely to exercise scrolling.
constexpr int8_t HA_CONTROL_PLACEHOLDER_LENGTH = 5;
}

bool screen_has_list(Screen screen) {
  return screen == Screen::TRANSIT || screen == Screen::HA_CONTROL;
}

int8_t screen_list_length(Screen screen) {
  if (screen == Screen::TRANSIT) return static_cast<int8_t>(departure_list.count);
  if (screen == Screen::HA_CONTROL) return HA_CONTROL_PLACEHOLDER_LENGTH;
  return 0;
}

void app_state_enter_screen(Screen screen) {
  app_state.current_screen = screen;
  app_state.list_cursor = 0;
}
