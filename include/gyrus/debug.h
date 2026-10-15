#ifndef GYRUS_DEBUG_H
#define GYRUS_DEBUG_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define GYRUS_DEBUG_LINE_MAX 256u
#define GYRUS_DEBUG_STATE_MAX 1536u
#define GYRUS_DEBUG_FRAME_MAX 57600u

typedef enum {
    GYRUS_DEBUG_STEP,
    GYRUS_DEBUG_PAUSE,
    GYRUS_DEBUG_RESUME,
    GYRUS_DEBUG_LEVEL,
    GYRUS_DEBUG_RESET
} gyrus_debug_action;
enum {
    GYRUS_INPUT_TURN = 1u,
    GYRUS_INPUT_ANGLE = 2u,
    GYRUS_INPUT_FIRE = 4u,
    GYRUS_INPUT_START = 8u,
    GYRUS_INPUT_PAUSE = 16u,
    GYRUS_INPUT_QUIT = 32u,
    GYRUS_INPUT_MENU = 64u
};
typedef struct {
    uint8_t mask;
    int8_t turn;
    int16_t angle_delta;
    uint8_t menu_action; /*0..8, one game menu pulse. */
    bool fire, start, pause, quit;
} gyrus_debug_input;
typedef struct {
    uint16_t width, height, palette_count;
    const uint16_t *palette; /* RGB565 words, transmitted little endian. */
    const uint8_t *pixels;
    size_t pixels_len;
} gyrus_debug_frame;
typedef struct {
    void *context;
    void (*write)(void *, const void *, size_t);
    bool (*state)(void *, char *, size_t); /* One bounded JSON object. */
    bool (*set)(void *, const char *, int32_t);
    bool (*action)(void *, gyrus_debug_action, uint32_t);
    bool (*input)(void *, const gyrus_debug_input *);
    bool (*frame)(void *, gyrus_debug_frame *); /* Stable until feed returns. */
} gyrus_debug_callbacks;
typedef struct {
    gyrus_debug_callbacks callbacks;
    char line[GYRUS_DEBUG_LINE_MAX];
    size_t used;
    uint8_t dropping; /* 0 accepting, 1 overlong, 2 invalid ASCII. */
} gyrus_debug;
void gyrus_debug_init(gyrus_debug *, const gyrus_debug_callbacks *);
void gyrus_debug_feed(gyrus_debug *, const void *, size_t);
uint32_t gyrus_debug_crc32(const void *, size_t);
#endif
