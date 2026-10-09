#pragma once
#include <Arduino.h>

// Draws a geometric weather pictogram centered at (cx,cy), selected
// from an OpenWeatherMap icon code (e.g. "01d", "10n" — see
// https://openweathermap.org/weather-conditions). No bitmaps, same
// line/rectangle/circle primitives as the rest of the chrome. Unknown
// codes draw nothing.
//
// `scale` multiplies every offset from center — 1.0 is the original
// ~28x28 size (WEATHER screen's "NOW" column), larger values scale the
// same shapes up (e.g. HOME's much bigger glance icon) without a
// second hand-drawn shape set. Integer offsets are rounded via scale's
// multiplication, not redrawn at a different level of detail, so very
// large scales will look a little blocky — fine for this panel's
// resolution at the sizes actually used (up to ~3x).
void draw_weather_icon(uint16_t cx, uint16_t cy, const String &owm_icon_code, float scale = 1.0f);

// Same idea, ~14x14, scale fixed at 1.0 — for dense contexts like an
// hourly forecast list where the full-size icon wouldn't fit. A
// separate, simpler shape set rather than a scaled-down
// draw_weather_icon, since scaling the full-size icon's absolute pixel
// offsets down would blur distinctions (e.g. the rain/thunder tails)
// at this size anyway.
void draw_weather_icon_mini(uint16_t cx, uint16_t cy, const String &owm_icon_code);
