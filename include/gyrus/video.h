#ifndef GYRUS_VIDEO_H
#define GYRUS_VIDEO_H
#include "gyrus/game.h"
#define VIDEO_STRIP_ROWS 16u
#define VIDEO_STRIP_PIXELS (GAME_WIDTH * VIDEO_STRIP_ROWS)
#define VIDEO_STRIPS (GAME_HEIGHT / VIDEO_STRIP_ROWS)
typedef struct {
    uint16_t x, width;
} video_span;
/* A full first presentation seeds every GRAM pixel; later spans cover the
 * circular aperture plus a two-pixel margin, including all black edge pixels. */
video_span video_strip_span(unsigned strip, bool full);
size_t video_expand_strip(const uint8_t *frame, unsigned strip, bool full, uint16_t *output,
                          size_t capacity);
#endif
