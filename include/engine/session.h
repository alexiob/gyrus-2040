#ifndef ENGINE_SESSION_H
#define ENGINE_SESSION_H
#include "engine/game_api.h"
#include "gyrus/game.h"
#include "tempest/game.h"
typedef struct {
    uint32_t high_score;
    game_settings settings;
    bool neon;
} engine_game_record;
typedef struct engine_state {
    engine_game_id selected;
    bool hub, hub_settings, frozen;
    uint32_t seed, hub_tick, ticks;
    int16_t slide_pixels;
    controls_state controls;
    engine_input input; /* Retained transport controls; pulses are consumed once. */
    engine_ui ui;
    uint16_t tilt_sensitivity;
    engine_game_record records[ENGINE_GAME_COUNT];
    union {
        game_state gyrus;
        tempest_state tempest;
    } game;
} engine_state;
void engine_init(engine_state *, uint32_t seed);
bool engine_select_game(engine_state *, engine_game_id, uint32_t seed);
void engine_home(engine_state *);
void engine_step(engine_state *, const engine_input *);
void engine_tick(engine_state *);
bool engine_update(engine_state *, controls_sample, const engine_input *keyboard);
controls_screen engine_controls_screen(const engine_state *);
engine_game_view engine_current_view(const engine_state *);
const engine_game_ops *engine_current_game(const engine_state *);
const uint16_t *engine_render(const engine_state *, uint8_t *pixels);
bool engine_set(engine_state *, const char *, int32_t);
bool engine_level(engine_state *, unsigned);
bool engine_state_json(const engine_state *, char *, size_t);
#endif
