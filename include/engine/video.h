#ifndef ENGINE_VIDEO_H
#define ENGINE_VIDEO_H
#include "engine/display.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define VIDEO_STRIP_ROWS 16u
#define VIDEO_STRIP_PIXELS (ENGINE_WIDTH * VIDEO_STRIP_ROWS)
#define VIDEO_STRIPS (ENGINE_HEIGHT / VIDEO_STRIP_ROWS)
typedef struct {
    uint16_t x, width;
} video_span;
/* A full first presentation seeds every GRAM pixel; later spans cover the
 * circular aperture plus a two-pixel margin, including all black edge pixels. */
video_span video_strip_span(unsigned strip, bool full);
size_t video_expand_strip_palette(const uint8_t *frame, const uint16_t *palette, unsigned strip,
                                  bool full, uint16_t *output, size_t capacity);
#endif
