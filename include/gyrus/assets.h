#ifndef GYRUS_ASSETS_H
#define GYRUS_ASSETS_H

#include <stdint.h>

/* Original authored indexed pixel art. Palette is ordinary host-endian RGB565.
 * Index zero is transparent when blitting sprites, black when clearing a scene. */
typedef struct {
    uint8_t width;
    uint8_t height;
    const uint8_t *pixels;
} gyrus_asset;

enum gyrus_palette_index {
    GYRUS_COLOR_BLACK = 0, GYRUS_COLOR_WHITE, GYRUS_COLOR_IVORY,
    GYRUS_COLOR_CYAN, GYRUS_COLOR_BLUE, GYRUS_COLOR_DEEP_BLUE,
    GYRUS_COLOR_AMBER, GYRUS_COLOR_ORANGE, GYRUS_COLOR_RED,
    GYRUS_COLOR_MAGENTA, GYRUS_COLOR_PINK, GYRUS_COLOR_VIOLET,
    GYRUS_COLOR_GREEN, GYRUS_COLOR_LIME, GYRUS_COLOR_TEAL,
    GYRUS_COLOR_STEEL, GYRUS_COLOR_DARK_STEEL, GYRUS_COLOR_BROWN,
    GYRUS_COLOR_SAND, GYRUS_COLOR_YELLOW, GYRUS_COLOR_PALE_BLUE,
    GYRUS_COLOR_DARK_RED, GYRUS_COLOR_DARK_GREEN, GYRUS_COLOR_PURPLE,
    GYRUS_COLOR_COUNT
};

typedef enum {
    GYRUS_SPRITE_PLAYER = 0,       /* 32 headings; 0 points up, clockwise. */
    GYRUS_SPRITE_SCOUT,            /* 16 headings; red swept wings. */
    GYRUS_SPRITE_FIGHTER,          /* 16 headings; amber delta wings. */
    GYRUS_SPRITE_MOTH,             /* 16 headings; magenta forked wings. */
    GYRUS_SPRITE_SATELLITE,        /* 8 animation frames. */
    GYRUS_SPRITE_ASTEROID,         /* 8 rotation frames. */
    GYRUS_SPRITE_LASER_EMITTER,    /* 8 animation frames. */
    GYRUS_SPRITE_EXPLOSION,        /* 6 frames, increasing age. */
    GYRUS_SPRITE_PLANET_EARTH,
    GYRUS_SPRITE_PLANET_MARS,
    GYRUS_SPRITE_PLANET_JUPITER,
    GYRUS_SPRITE_PLANET_SATURN,
    GYRUS_SPRITE_PLANET_URANUS,
    GYRUS_SPRITE_PLANET_NEPTUNE,
    GYRUS_SPRITE_LIFE,
    GYRUS_SPRITE_PLAYER_BULLET,
    GYRUS_SPRITE_ENEMY_BULLET,
    GYRUS_SPRITE_POWERUP,
    GYRUS_SPRITE_COUNT
} gyrus_sprite_id;

extern const uint16_t gyrus_palette[256];
/* frame_or_heading wraps modulo the sprite's frame count. Invalid IDs return 0. */
const gyrus_asset *gyrus_sprite(gyrus_sprite_id id, unsigned frame_or_heading);
unsigned gyrus_sprite_frame_count(gyrus_sprite_id id);
/* Seven rows, five bits per row, bit 4 is the leftmost pixel. ASCII case-insensitive.
 * Unsupported characters return a blank glyph. Returned storage is immutable. */
const uint8_t *gyrus_glyph(char character);

#endif
