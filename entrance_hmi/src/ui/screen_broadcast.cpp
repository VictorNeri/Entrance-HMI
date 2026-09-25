#include "screen_broadcast.h"
#include <string.h>
#include "../epd_driver/EPD.h"
#include "../epd_driver/EPD_Init.h"
#include "../storage/broadcast_store.h"
#include "ui_text_unicode.h"

namespace {

constexpr uint16_t CANVAS_RIGHT = 792, CANVAS_BOTTOM = 272;

// Double outline for a bold, attention-grabbing frame — the only way
// to convey "important" on a 1-bit panel without shading.
constexpr uint16_t BOX_X0 = 6, BOX_Y0 = 6, BOX_X1 = 786, BOX_Y1 = 266;
constexpr uint16_t INNER_X0 = 10, INNER_Y0 = 10, INNER_X1 = 782, INNER_Y1 = 262;

constexpr uint16_t TITLE_SIZE = 48, TITLE_Y = 18;
constexpr uint16_t DIVIDER_Y = 76;

constexpr uint16_t BODY_SIZE = 24, BODY_X0 = 40, BODY_X1 = 752, BODY_Y0 = 90, LINE_H = 28;
constexpr uint8_t MAX_LINES = 5;
constexpr uint16_t MAX_CHARS_PER_LINE = (BODY_X1 - BODY_X0) / (BODY_SIZE / 2);

constexpr uint16_t HINT_SIZE = 16, HINT_Y = 240;

const char *title_for_level(const char *level) {
  if (strcmp(level, "urgent") == 0) return "URGENT";
  if (strcmp(level, "warning") == 0) return "WARNING";
  return "MESSAGE";
}

void draw_centered(uint16_t y, const char *text, uint16_t size) {
  uint16_t width = strlen(text) * (size / 2);
  uint16_t x = width < CANVAS_RIGHT ? (CANVAS_RIGHT - width) / 2 : 0;
  EPD_ShowString(x, y, text, size, BLACK);
}

// Greedy word-wrap into up to MAX_LINES fixed-width lines, honoring an
// explicit '\n' as a forced break. Byte-counted rather than decoded-
// UTF-8-char-counted, so a line with accented characters (2 bytes each
// via draw_utf8_string's decoding, see ui_text_unicode.h) wraps a
// little earlier than the pixel width strictly requires — conservative
// in the safe direction, never overflows the box. Text beyond MAX_LINES
// is dropped, with the last line's tail replaced by "..." so it's
// visibly truncated rather than silently cut off.
uint8_t wrap_lines(const char *text, char lines[][BROADCAST_TEXT_MAX + 1]) {
  uint8_t line_count = 0;
  char *line = lines[0];
  size_t line_len = 0;
  line[0] = '\0';

  const char *p = text;
  bool truncated = false;

  auto push_line = [&]() {
    line[line_len] = '\0';
    line_count++;
    if (line_count < MAX_LINES) {
      line = lines[line_count];
      line_len = 0;
      line[0] = '\0';
    }
  };

  while (*p != '\0' && line_count < MAX_LINES) {
    if (*p == '\n') {
      push_line();
      p++;
      continue;
    }

    // Next word: [word_start, word_end).
    const char *word_start = p;
    while (*p != '\0' && *p != ' ' && *p != '\n') p++;
    size_t word_len = p - word_start;
    while (*p == ' ') p++;  // collapse the run of spaces after it

    if (word_len == 0) continue;

    // Word alone is longer than a whole line: hard-break it.
    if (word_len > MAX_CHARS_PER_LINE) {
      size_t offset = 0;
      while (offset < word_len && line_count < MAX_LINES) {
        size_t space_left = MAX_CHARS_PER_LINE - line_len;
        if (space_left == 0) {
          push_line();
          continue;
        }
        size_t chunk = word_len - offset < space_left ? word_len - offset : space_left;
        memcpy(line + line_len, word_start + offset, chunk);
        line_len += chunk;
        offset += chunk;
      }
      continue;
    }

    size_t needed = line_len == 0 ? word_len : line_len + 1 + word_len;
    if (needed > MAX_CHARS_PER_LINE) {
      push_line();
      if (line_count >= MAX_LINES) {
        truncated = true;  // this word never got written to any line
        break;
      }
    }
    if (line_len > 0) line[line_len++] = ' ';
    memcpy(line + line_len, word_start, word_len);
    line_len += word_len;
  }

  if (line_count < MAX_LINES) {
    line[line_len] = '\0';
    line_count++;
  }
  if (*p != '\0') truncated = true;  // more text existed than MAX_LINES could hold

  if (truncated) {
    char *last = lines[MAX_LINES - 1];
    size_t len = strlen(last);
    size_t cut = len + 3 > MAX_CHARS_PER_LINE ? MAX_CHARS_PER_LINE - 3 : len;
    strcpy(last + cut, "...");
  }

  return line_count;
}

}  // namespace

void screen_broadcast_render_overlay() {
  // EPD_ClearWindows is declared in EPD.h but was never actually
  // implemented anywhere (not here, not in the vendor source this
  // driver was copied from) — a filled white rectangle via the
  // already-working EPD_DrawRectangle does the same job. -1 on both
  // ends: Paint_SetPixel's rotate-180 coordinate math wraps to a huge
  // out-of-bounds index at exactly the visible width/height (matches
  // the pattern every other CANVAS_RIGHT/UI_FOOTER_Y1 use in
  // ui_chrome.cpp already avoids the same way — this bit the project
  // before, see the Aug 31 framebuffer-overrun commit).
  EPD_DrawRectangle(0, 0, CANVAS_RIGHT - 1, CANVAS_BOTTOM - 1, WHITE, 1);

  EPD_DrawRectangle(BOX_X0, BOX_Y0, BOX_X1, BOX_Y1, BLACK, 0);
  EPD_DrawRectangle(INNER_X0, INNER_Y0, INNER_X1, INNER_Y1, BLACK, 0);

  draw_centered(TITLE_Y, title_for_level(broadcast_message.level), TITLE_SIZE);
  EPD_DrawLine(INNER_X0 + 4, DIVIDER_Y, INNER_X1 - 4, DIVIDER_Y, BLACK);

  static char lines[MAX_LINES][BROADCAST_TEXT_MAX + 1];
  uint8_t line_count = wrap_lines(broadcast_message.text, lines);
  for (uint8_t i = 0; i < line_count; i++) {
    draw_utf8_string(BODY_X0, BODY_Y0 + i * LINE_H, lines[i], BODY_SIZE, BLACK);
  }

  draw_centered(HINT_Y, "OK: Acknowledge", HINT_SIZE);
}
