#pragma once
#include "../input/buttons.h"

enum class RedrawKind : uint8_t { NONE, FULL, PARTIAL };

RedrawKind nav_handle_event(const ButtonEvent &event);
