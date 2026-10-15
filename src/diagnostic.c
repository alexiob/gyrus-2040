#include "gyrus/diagnostic.h"
#include <stddef.h>

bool diag_window_encode(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t out[8]) {
    if (out == NULL || w == 0u || h == 0u || x >= DIAG_WIDTH || y >= DIAG_HEIGHT ||
        (uint32_t)x + w > DIAG_WIDTH || (uint32_t)y + h > DIAG_HEIGHT) return false;
    const uint16_t endpoints[4] = {x, (uint16_t)(x + w - 1u), y, (uint16_t)(y + h - 1u)};
    for (unsigned i = 0; i < 4u; ++i) {
        out[2u*i] = (uint8_t)(endpoints[i] >> 8u);
        out[2u*i+1u] = (uint8_t)endpoints[i];
    }
    return true;
}

bool diag_touch_decode(const uint8_t bytes[5], diag_touch *out) {
    if (out == NULL) return false;
    *out = (diag_touch){0};
    if (bytes == NULL || bytes[0] > 1u) return false;
    if (bytes[0] == 0u) return true;
    const uint16_t x = (uint16_t)(((uint16_t)(bytes[1] & 15u) << 8u) | bytes[2]);
    const uint16_t y = (uint16_t)(((uint16_t)(bytes[3] & 15u) << 8u) | bytes[4]);
    if (x >= DIAG_WIDTH || y >= DIAG_HEIGHT) return false;
    *out = (diag_touch){.pressed = true, .x = x, .y = y};
    return true;
}

uint16_t diag_pattern_pixel(unsigned pattern, uint16_t x, uint16_t y) {
    static const uint16_t bars[6] = {0xf800,0x07e0,0x001f,0xffe0,0x07ff,0xf81f};
    if (x >= DIAG_WIDTH || y >= DIAG_HEIGHT) return 0u;
    if (pattern == 0u) return bars[x / 40u];
    if (pattern == 1u) return ((x / 8u + y / 8u) & 1u) == 0u ? 0xffffu : 0u;
    if (x % 20u == 0u || y % 20u == 0u) return 0xffffu;
    return (uint16_t)(((x / 8u) << 11u) | ((y / 4u) << 5u));
}

/* Discard an invalid line in full; never pass embedded NUL to strcmp. */
diag_line_result diag_line_feed(diag_line *line, uint8_t byte) {
    if (byte == '\n' || byte == '\r') {
        diag_line_result result=line->rejected;
        if (result==DIAG_LINE_NONE && line->used>0u) {
            line->text[line->used]='\0';result=DIAG_LINE_READY;
        }
        line->used=0;line->rejected=DIAG_LINE_NONE;
        return result;
    }
    if (byte<32u || byte>126u) {
        line->rejected=DIAG_LINE_INVALID;return DIAG_LINE_NONE;
    }
    if (line->rejected==DIAG_LINE_NONE) {
        if ((size_t)line->used+1u<sizeof line->text) line->text[line->used++]=(char)byte;
        else line->rejected=DIAG_LINE_LONG;
    }
    return DIAG_LINE_NONE;
}
