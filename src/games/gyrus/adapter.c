#include "engine/game_api.h"
#include "gyrus/assets.h"
#include "gyrus/debug_game.h"
static void init(void *s, uint32_t seed) {
    game_init(s, seed);
}
static void step(void *s, const engine_input *in) {
    game_step(s, in);
}
static void render(const void *s, uint8_t *pixels) {
    game_render(s, pixels);
}
static uint32_t hash(const void *s) {
    return game_state_hash(s);
}
static engine_game_view view(const void *state) {
    const game_state *s = state;
    controls_screen screen = CONTROLS_PLAY;
    if (s->pause_settings)
        screen = CONTROLS_SETTINGS;
    else if (s->phase == GAME_TITLE)
        screen = CONTROLS_TITLE;
    else if (s->phase == GAME_OVER)
        screen = CONTROLS_GAME_OVER;
    else if (s->phase == GAME_PAUSED)
        screen = CONTROLS_PAUSE;
    return (engine_game_view){.screen = screen,
                              .settings = s->settings,
                              .score = s->score,
                              .high_score = s->high_score,
                              .level = s->stage,
                              .lives = s->lives,
                              .title = s->phase == GAME_TITLE,
                              .over = s->phase == GAME_OVER,
                              .paused = s->phase == GAME_PAUSED,
                              .settings_open = s->pause_settings};
}
static void sync_ui(void *state, const engine_ui *ui) {
    game_state *s = state;
    s->control_mode = ui->control_mode;
    s->reboot_progress = ui->reboot_progress;
    s->tilt_ready = ui->tilt_ready;
    s->tilt_progress = ui->tilt_progress;
    s->tilt_preview = ui->tilt_preview;
}
static bool set(void *state, const char *key, int32_t value) {
    gyrus_debug_game adapter;
    gyrus_debug_game_init(&adapter, state, NULL, NULL, NULL);
    gyrus_debug_callbacks cb = gyrus_debug_game_callbacks(&adapter);
    return cb.set(cb.context, key, value);
}
static bool level(void *s, unsigned value) {
    return game_set_stage(s, value);
}
static bool state_json(const void *state, bool frozen, char *out, size_t cap) {
    gyrus_debug_game adapter;
    /* The legacy callback is read-only; its interface predates const game states. */
    gyrus_debug_game_init(&adapter, (game_state *)state, NULL, NULL, NULL);
    adapter.frozen = frozen;
    gyrus_debug_callbacks cb = gyrus_debug_game_callbacks(&adapter);
    return cb.state(cb.context, out, cap);
}
const engine_game_ops engine_gyrus_game = {.id = ENGINE_GAME_GYRUS,
                                           .slug = "gyrus",
                                           .title = "Gyrus",
                                           .year = 1983,
                                           .state_size = sizeof(game_state),
                                           .palette = gyrus_palette,
                                           .init = init,
                                           .step = step,
                                           .render = render,
                                           .draw_title = engine_title_gyrus,
                                           .view = view,
                                           .sync_ui = sync_ui,
                                           .set = set,
                                           .level = level,
                                           .hash = hash,
                                           .state_json = state_json};
