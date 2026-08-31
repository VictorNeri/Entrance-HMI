#pragma once
#include <Arduino.h>

// Shared chrome geometry — a big time/date "hero" block (replaces both
// the old thin header bar and every screen's big title text, freeing
// that space for it), a button-legend rail (right side, matching the
// physical buttons), and a footer screen dock. Screens lay out their
// own content inside the remaining rect these constants describe.
constexpr uint16_t UI_FOOTER_Y0 = 252, UI_FOOTER_Y1 = 272;
// Button-legend rail lives on the right, matching the physical buttons'
// side of the panel. Spans the full height now that there's no header.
constexpr uint16_t UI_RAIL_X0 = 752, UI_RAIL_X1 = 792;

constexpr uint16_t UI_CONTENT_LEFT = 20;
constexpr uint16_t UI_CONTENT_RIGHT = 740;

// Hero time/date: time at size 48, date at size 24 to its right, a
// pending-events dot further right, then a divider separating this
// block from body content below.
constexpr uint16_t UI_HERO_TIME_Y = 4;
constexpr uint16_t UI_HERO_DATE_X = 160;
constexpr uint16_t UI_HERO_DATE_Y = 16;
constexpr uint16_t UI_HERO_DIVIDER_Y = 58;

constexpr uint16_t UI_CONTENT_BODY_TOP = 66;  // first body line
constexpr uint16_t UI_CONTENT_HINT_Y = 228;   // screen-specific OK/EXIT hint, size 16
constexpr uint16_t UI_CONTENT_BOTTOM = 244;   // nothing should paint below this

// Generic 3-column content split — shared so HOME's weather/event/train
// summary and WEATHER's current/hourly/alerts breakdown line up
// visually instead of each screen computing slightly different column
// bounds.
constexpr uint16_t UI_COL_W = (UI_CONTENT_RIGHT - UI_CONTENT_LEFT) / 3;
constexpr uint16_t UI_COL1_X = UI_CONTENT_LEFT;
constexpr uint16_t UI_COL_DIVIDER1_X = UI_CONTENT_LEFT + UI_COL_W;
constexpr uint16_t UI_COL2_X = UI_COL_DIVIDER1_X + 16;
constexpr uint16_t UI_COL_DIVIDER2_X = UI_CONTENT_LEFT + 2 * UI_COL_W;
constexpr uint16_t UI_COL3_X = UI_COL_DIVIDER2_X + 16;

// Draws the hero time/date block, button-legend rail, and footer screen
// dock in one call. Screens should not paint into these regions
// themselves, and no longer draw their own title (the hero block
// replaces it).
void ui_chrome_render();
