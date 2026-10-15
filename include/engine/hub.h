#ifndef ENGINE_HUB_H
#define ENGINE_HUB_H
#include "engine/game_api.h"
typedef struct {
    const engine_game_ops *const *catalogue;
    size_t count;
    unsigned selected;
    int16_t slide_pixels; /* Current card visual X offset; neighbors +/-220px. */
    uint32_t tick, high_score;
    bool neon, settings_open;
    game_settings settings;
    engine_ui ui;
} engine_hub_view;
void engine_hub_render(const engine_hub_view *, uint8_t *pixels);
#endif
