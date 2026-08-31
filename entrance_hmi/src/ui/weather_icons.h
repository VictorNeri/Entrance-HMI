#pragma once
#include <Arduino.h>

// Draws a small (~28x28) geometric weather pictogram centered at (cx,cy),
// selected from an OpenWeatherMap icon code (e.g. "01d", "10n" — see
// https://openweathermap.org/weather-conditions). No bitmaps, same
// line/rectangle/circle primitives as the rest of the chrome. Unknown
// codes draw nothing.
void draw_weather_icon(uint16_t cx, uint16_t cy, const String &owm_icon_code);

// Same idea, ~14x14 — for dense contexts like an hourly forecast list
// where the full-size icon wouldn't fit. A separate, simpler shape set
// rather than a scaled-down draw_weather_icon, since scaling the
// full-size icon's absolute pixel offsets down would blur distinctions
// (e.g. the rain/thunder tails) at this size anyway.
void draw_weather_icon_mini(uint16_t cx, uint16_t cy, const String &owm_icon_code);
