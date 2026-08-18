#pragma once
#include <Arduino.h>

// UTF-8-safe wrapper around EPD_ShowString. The factory font renderer
// indexes its bitmap tables with no bounds checking (chr - ' '), so
// passing raw UTF-8 continuation bytes straight through reads garbage
// out-of-table data and renders broken glyphs. This decodes UTF-8 and
// falls back to an unaccented ASCII transliteration for the Swedish
// å/ä/ö/Å/Ä/Ö characters (SL destination names contain these); any
// other non-ASCII byte is skipped rather than risking an
// out-of-bounds font-table read. Proper accented glyphs are a
// possible future visual-polish addition, not attempted here.
void draw_utf8_string(uint16_t x, uint16_t y, const char *utf8, uint16_t size, uint16_t color);
