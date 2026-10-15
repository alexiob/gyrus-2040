#include "engine/game_api.h"
static const engine_game_ops *const games[] = {&engine_gyrus_game, &engine_tempest_game};
const engine_game_ops *const *engine_catalogue(size_t *count) {
    if (count)
        *count = sizeof games / sizeof games[0];
    return games;
}
