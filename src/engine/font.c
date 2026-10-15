#include "engine/draw.h"
#include "font_data.inc"

const uint8_t *engine_font_glyph(char character) {
    unsigned code = (unsigned char)character;
    return font[code >= 32 && code < 128 ? code - 32 : 0];
}
