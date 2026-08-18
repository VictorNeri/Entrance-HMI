#include "app_state.h"
#include "../net/transit_client.h"
#include "../storage/button_config_store.h"

AppState app_state;

bool screen_has_list(Screen screen) {
  return screen == Screen::TRANSIT || screen == Screen::HA_CONTROL;
}

int8_t screen_list_length(Screen screen) {
  if (screen == Screen::TRANSIT) return static_cast<int8_t>(departure_list.count);
  if (screen == Screen::HA_CONTROL) return static_cast<int8_t>(ha_entity_list.count);
  return 0;
}

void app_state_enter_screen(Screen screen) {
  app_state.current_screen = screen;
  app_state.list_cursor = 0;
}
