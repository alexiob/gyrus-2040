#include "engine/draw.h"
#include <string.h>

const uint16_t engine_palette[256] = {
    0x0000, 0xffff, 0xff99, 0x5f5e, 0x343d, 0x11af, 0xfde9, 0xf3a5, 0xe9c7, 0xda39, 0xfcb9, 0x8b3d,
    0x4611, 0xa72f, 0x2473, 0x84d5, 0x324b, 0x8286, 0xdd2d, 0xff2c, 0xbf7f, 0x7946, 0x1aeb, 0x414e,
};

void draw_pixel(uint8_t *fb, int x, int y, uint8_t color) {
    if ((unsigned)x < ENGINE_WIDTH && (unsigned)y < ENGINE_HEIGHT)
        fb[(unsigned)y * ENGINE_WIDTH + (unsigned)x] = color;
}
void draw_fill(uint8_t *fb, int x, int y, int width, int height, uint8_t color) {
    if (width <= 0 || height <= 0 || x >= (int)ENGINE_WIDTH || y >= (int)ENGINE_HEIGHT)
        return;
    /* Clip before adding endpoints. Even INT_MIN origins and INT_MAX sizes
       cannot overflow; every subsequent address is inside the framebuffer. */
    if (x < 0) {
        if (x <= -width)
            return;
        width += x;
        x = 0;
    }
    if (y < 0) {
        if (y <= -height)
            return;
        height += y;
        y = 0;
    }
    if (width > (int)ENGINE_WIDTH - x)
        width = (int)ENGINE_WIDTH - x;
    if (height > (int)ENGINE_HEIGHT - y)
        height = (int)ENGINE_HEIGHT - y;
    unsigned offset = (unsigned)y * ENGINE_WIDTH + (unsigned)x;
    /* Glyphs at native scale are one pixel wide; avoid a libc call for those. */
    if (width == 1) {
        for (int yy = 0; yy < height; ++yy) {
            fb[offset] = color;
            offset += ENGINE_WIDTH;
        }
    } else if (width == (int)ENGINE_WIDTH) {
        memset(fb + offset, color, (unsigned)height * ENGINE_WIDTH);
    } else {
        for (int yy = 0; yy < height; ++yy) {
            memset(fb + offset, color, (unsigned)width);
            offset += ENGINE_WIDTH;
        }
    }
}
void draw_line(uint8_t *fb, int x0, int y0, int x1, int y1, uint8_t color) {
    const int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    const int dy = y1 > y0 ? y0 - y1 : y1 - y0;
    const int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    for (;;) {
        draw_pixel(fb, x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;
        const int twice = 2 * error;
        if (twice >= dy) {
            error += dy;
            x0 += sx;
        }
        if (twice <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}
int draw_text_width(const char *string, int scale) {
    int count = 0;
    while (*string != '\0') {
        ++count;
        ++string;
    }
    return count > 0 ? (count * 6 - 1) * scale : 0;
}
void draw_label(uint8_t *fb, const char *string, int x, int y, uint8_t color, int scale) {
    while (*string != '\0') {
        const uint8_t *glyph = engine_font_glyph(*string++);
        for (unsigned row = 0; row < 7u; ++row)
            for (unsigned col = 0; col < 5u; ++col)
                if ((glyph[row] & (1u << (4u - col))) != 0u)
                    draw_fill(fb, x + (int)col * scale, y + (int)row * scale, scale, scale, color);
        x += 6 * scale;
    }
}
void draw_centered(uint8_t *fb, const char *string, int y, uint8_t color, int scale) {
    draw_label(fb, string, 120 - draw_text_width(string, scale) / 2, y, color, scale);
}
void draw_decimal(char result[11], uint32_t number, unsigned digits) {
    char reversed[10];
    unsigned used = 0;
    do {
        reversed[used++] = (char)('0' + number % 10u);
        number /= 10u;
    } while (number != 0u && used < 10u);
    while (used < digits && used < 10u)
        reversed[used++] = '0';
    for (unsigned i = 0; i < used; ++i)
        result[i] = reversed[used - 1u - i];
    result[used] = '\0';
}

/* Exact left edge of (x-120)^2+(y-120)^2 <= 14400 for every row.
   The centre is asymmetric in a 0..239 buffer: right padding is left-1.
   This 240-byte ROM table replaces 57,600 per-frame geometry evaluations. */
static const uint8_t round_left[240] = {
    120, 105, 99, 94, 90, 86, 83, 80, 77, 75, 73, 70, 68, 66, 64, 62, 61, 59, 57, 56,  54, 53,
    51,  50,  48, 47, 46, 45, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30,  29, 28,
    28,  27,  26, 25, 24, 24, 23, 22, 22, 21, 20, 20, 19, 18, 18, 17, 17, 16, 15, 15,  14, 14,
    13,  13,  12, 12, 11, 11, 11, 10, 10, 9,  9,  8,  8,  8,  7,  7,  7,  6,  6,  6,   5,  5,
    5,   5,   4,  4,  4,  4,  3,  3,  3,  3,  3,  2,  2,  2,  2,  2,  2,  1,  1,  1,   1,  1,
    1,   1,   1,  1,  1,  1,  1,  1,  1,  1,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,   1,  1,
    1,   1,   1,  1,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  4,  4,  4,  4,  5,   5,  5,
    5,   6,   6,  6,  7,  7,  7,  8,  8,  8,  9,  9,  10, 10, 11, 11, 11, 12, 12, 13,  13, 14,
    14,  15,  15, 16, 17, 17, 18, 18, 19, 20, 20, 21, 22, 22, 23, 24, 24, 25, 26, 27,  28, 28,
    29,  30,  31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 45, 46, 47, 48, 50,  51, 53,
    54,  56,  57, 59, 61, 62, 64, 66, 68, 70, 73, 75, 77, 80, 83, 86, 90, 94, 99, 105,
};

void draw_mask_round(uint8_t *pixels) {
    for (unsigned y = 0; y < ENGINE_HEIGHT; ++y) {
        const unsigned left = round_left[y];
        uint8_t *row = pixels + y * ENGINE_WIDTH;
        if (left != 0u)
            memset(row, DRAW_BLACK, left);
        if (left > 1u)
            memset(row + (ENGINE_WIDTH + 1u - left), DRAW_BLACK, left - 1u);
    }
}
