#include "ui_text_unicode.h"
#include "../epd_driver/EPD.h"

namespace {

// Swedish å/ä/ö/Å/Ä/Ö are all 2-byte UTF-8 sequences with lead byte
// 0xC3. Returns the ASCII fallback for a recognized second byte, or 0
// if this isn't one of the six characters we handle.
char latin1_fallback(uint8_t second_byte) {
  switch (second_byte) {
    case 0xA5:
      return 'a';  // å
    case 0xA4:
      return 'a';  // ä
    case 0xB6:
      return 'o';  // ö
    case 0x85:
      return 'A';  // Å
    case 0x84:
      return 'A';  // Ä
    case 0x96:
      return 'O';  // Ö
    default:
      return 0;
  }
}

}  // namespace

void draw_utf8_string(uint16_t x, uint16_t y, const char *utf8, uint16_t size, uint16_t color) {
  const uint8_t *p = reinterpret_cast<const uint8_t *>(utf8);

  while (*p != '\0') {
    uint8_t b = *p;

    if (b < 0x80) {
      EPD_ShowChar(x, y, b, size, color);
      x += size / 2;
      p++;
      continue;
    }

    if (b == 0xC3 && p[1] != '\0') {
      char fallback = latin1_fallback(p[1]);
      if (fallback != 0) {
        EPD_ShowChar(x, y, fallback, size, color);
        x += size / 2;
      }
      p += 2;
      continue;
    }

    // Any other multi-byte/unrecognized sequence: skip exactly one raw
    // byte without drawing. This never risks reading past the string's
    // actual length, unlike advancing by a presumed sequence length.
    p++;
  }
}
