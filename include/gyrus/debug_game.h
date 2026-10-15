#ifndef GYRUS_DEBUG_GAME_H
#define GYRUS_DEBUG_GAME_H
#include "gyrus/controls.h"
#include "gyrus/debug.h"
#include "gyrus/game.h"
typedef struct {
    game_state *game;
    controls_state *controls; /* Optional physical/host controls to change mode. */
    uint8_t *framebuffer;
    game_input input;
    bool frozen;
    void (*write)(void *, const void *, size_t);
    void *write_context;
} gyrus_debug_game;
void gyrus_debug_game_init(gyrus_debug_game *, game_state *, uint8_t *,
                           void (*write)(void *, const void *, size_t), void *);
gyrus_debug_callbacks gyrus_debug_game_callbacks(gyrus_debug_game *);
/* One simulation tick; consumes angle/start/pause/quit pulses, retains turn/fire. */
void gyrus_debug_game_tick(gyrus_debug_game *);
#endif
