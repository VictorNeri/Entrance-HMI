#include "app_state.h"

AppState app_state;

namespace {
constexpr int8_t PLACEHOLDER_LIST_LENGTH = 5;
}

bool screen_has_list(Screen screen) {
  return screen == Screen::TRANSIT || screen == Screen::HA_CONTROL;
}

int8_t screen_list_length(Screen screen) {
  return screen_has_list(screen) ? PLACEHOLDER_LIST_LENGTH : 0;
}

void app_state_enter_screen(Screen screen) {
  app_state.current_screen = screen;
  app_state.list_cursor = 0;
}
