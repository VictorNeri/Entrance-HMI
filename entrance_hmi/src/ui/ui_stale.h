#pragma once
#include <Arduino.h>

// "This data is old" badges. Fetched data stays on screen after its
// source stops updating (WiFi/API outage, 429 backoff), which on an
// e-paper panel looks exactly like fresh data. Once a dataset is
// overdue, its section gets a small inverted (white-on-black) age
// badge, e.g. "25m ago"; while data is fresh nothing is drawn, so the
// normal layout is unchanged.
//
// Ages are computed at render time, and loop() already re-renders once
// a minute for the header clock, so the badge appears and ticks up on
// its own without any extra refresh trigger.

// Overdue thresholds: missing roughly two polls in a row for weather
// and forecast. Transit uses the slow (background) poll interval as the
// base even on TRANSIT, so arriving there with background-age data
// doesn't flash a badge for the second it takes the immediate refetch
// to land.
unsigned long ui_weather_stale_after_ms();
unsigned long ui_forecast_stale_after_ms();
unsigned long ui_transit_stale_after_ms();

// If the data fetched at `fetched_at_ms` (a millis() timestamp) is
// older than `stale_after_ms`, draws the age badge with its top-left
// corner at (x, y) and returns its width in pixels; otherwise draws
// nothing and returns 0. Badge height is UI_STALE_BADGE_H.
uint16_t ui_draw_stale_badge(uint16_t x, uint16_t y, unsigned long fetched_at_ms,
                             unsigned long stale_after_ms);

// Width the badge would have (0 if not stale) — for right-aligning it.
uint16_t ui_stale_badge_width(unsigned long fetched_at_ms, unsigned long stale_after_ms);

constexpr uint16_t UI_STALE_BADGE_H = 20;
