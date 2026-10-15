#ifndef ENGINE_DRAW_H
#define ENGINE_DRAW_H
#include "engine/display.h"
#include <stdint.h>
enum {
    DRAW_BLACK,
    DRAW_WHITE,
    DRAW_IVORY,
    DRAW_CYAN,
    DRAW_BLUE,
    DRAW_DEEP_BLUE,
    DRAW_AMBER,
    DRAW_ORANGE,
    DRAW_RED,
    DRAW_MAGENTA,
    DRAW_PINK,
    DRAW_VIOLET,
    DRAW_GREEN,
    DRAW_LIME,
    DRAW_TEAL,
    DRAW_STEEL,
    DRAW_DARK,
    DRAW_BROWN,
    DRAW_SAND,
    DRAW_YELLOW,
    DRAW_PALE_BLUE,
    DRAW_DARK_RED,
    DRAW_DARK_GREEN,
    DRAW_PURPLE,
    DRAW_COLOR_COUNT
};
extern const uint16_t engine_palette[256];
const uint8_t *engine_font_glyph(char character);
void draw_pixel(uint8_t *, int x, int y, uint8_t color);
void draw_fill(uint8_t *, int x, int y, int width, int height, uint8_t color);
void draw_line(uint8_t *, int x0, int y0, int x1, int y1, uint8_t color);
int draw_text_width(const char *, int scale);
void draw_label(uint8_t *, const char *, int x, int y, uint8_t color, int scale);
void draw_centered(uint8_t *, const char *, int y, uint8_t color, int scale);
void draw_decimal(char result[11], uint32_t number, unsigned digits);
void draw_mask_round(uint8_t *);
#endif
