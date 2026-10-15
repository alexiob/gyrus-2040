#include "engine/draw.h"
#include "tempest/game.h"
#include <stdio.h>
#include <string.h>
static void init(void *s, uint32_t seed) {
    tempest_init(s, seed);
}
static void step(void *s, const engine_input *in) {
    tempest_step(s, in);
}
static void render(const void *s, uint8_t *p) {
    tempest_render(s, p);
}
static uint32_t hash(const void *s) {
    return tempest_hash(s);
}
static bool level(void *s, unsigned n) {
    return tempest_set_stage(s, n);
}
static void sync_ui(void *state, const engine_ui *ui) {
    tempest_state *s = state;
    s->ui = *ui;
}
static engine_game_view view(const void *state) {
    const tempest_state *s = state;
    controls_screen screen = CONTROLS_TEMPEST_PLAY;
    if (s->settings_open)
        screen = CONTROLS_SETTINGS;
    else if (s->phase == TEMPEST_PAUSED)
        screen = CONTROLS_PAUSE;
    else if (s->phase == TEMPEST_TITLE)
        screen = CONTROLS_TITLE;
    else if (s->phase == TEMPEST_OVER)
        screen = CONTROLS_GAME_OVER;
    return (engine_game_view){.screen = screen,
                              .settings = s->settings,
                              .score = s->score,
                              .high_score = s->high_score,
                              .level = s->stage,
                              .lives = s->lives,
                              .title = s->phase == TEMPEST_TITLE,
                              .over = s->phase == TEMPEST_OVER,
                              .paused = s->phase == TEMPEST_PAUSED,
                              .settings_open = s->settings_open};
}
static bool set(void *state, const char *key, int32_t n) {
    tempest_state *s = state;
    if (!key)
        return false;
    if (!strcmp(key, "fire_rate")) {
        bool changed = n != s->settings.fire_rate;
        bool valid = engine_settings_set(&s->settings, GAME_SETTING_FIRE_RATE, n);
        if (valid && changed) {
            s->fire_cooldown = 0;
            s->fire_remainder = 0;
        }
        return valid;
    }
    if (!strcmp(key, "enemy_speed"))
        return engine_settings_set(&s->settings, GAME_SETTING_ENEMY_SPEED, n);
    if (!strcmp(key, "tilt_sensitivity"))
        return engine_settings_set(&s->settings, GAME_SETTING_TILT_SENSITIVITY, n);
    if (!strcmp(key, "high_score") && n >= 0 && n <= 99999999) {
        s->high_score = (uint32_t)n;
        return true;
    }
    if (!strcmp(key, "score") && n >= 0 && n <= 99999999) {
        s->score = (uint32_t)n;
        s->next_bonus = ((uint32_t)n / 20000u + 1u) * 20000u;
        return true;
    }
    if (!strcmp(key, "lives") && n >= 0 && n <= 99) {
        s->lives = (uint8_t)n;
        return true;
    }
    if (n != 0 && n != 1)
        return false;
    if (!strcmp(key, "effects")) {
        s->effects = n != 0;
        return true;
    }
    if (!strcmp(key, "autofire")) {
        s->autofire = n != 0;
        return true;
    }
    if (!strcmp(key, "invincible")) {
        s->invincible = n != 0;
        return true;
    }
    return false;
}
static bool state_json(const void *state, bool frozen, char *out, size_t cap) {
    const tempest_state *s = state;
    if (!out || !cap)
        return false;
    unsigned enemies = 0, shots = 0, hostile = 0, spikes = 0, fx = 0;
    for (unsigned i = 0; i < TEMPEST_ENEMIES; i++)
        enemies += s->enemies[i].active != 0;
    for (unsigned i = 0; i < TEMPEST_SHOTS; i++)
        shots += s->shots[i].active != 0;
    for (unsigned i = 0; i < TEMPEST_HOSTILE; i++)
        hostile += s->hostile[i].active != 0;
    for (unsigned i = 0; i < TEMPEST_RAILS; i++)
        spikes += s->spikes[i] > 0;
    for (unsigned i = 0; i < TEMPEST_VISUAL_EVENTS; i++)
        fx += s->visual[i].life != 0;
    int n = snprintf(
        out, cap,
        "{\"game\":\"tempest\",\"phase\":\"%s\",\"tick\":%lu,\"stage\":%u,\"web\":\"%s\","
        "\"closed\":%s,\"lanes\":%u,\"score\":%lu,\"high_score\":%lu,\"lives\":%u,\"zaps\":%u,"
        "\"enemies\":%u,\"shots\":%u,\"hostile\":%u,\"spikes\":%u,\"claw_q8\":%ld,\"hash\":%lu,"
        "\"frozen\":%s,\"effects\":%s,\"pause_settings\":%s,\"fire_rate\":%u,\"enemy_speed\":%u,"
        "\"tilt_sensitivity\":%u,\"phase_ticks\":%u,\"dive_depth\":%ld,\"fx\":%u,\"zap_flash\":%u}",
        tempest_phase_name(s->phase), (unsigned long)s->tick, s->stage,
        tempest_current_web(s)->name, tempest_current_web(s)->closed ? "true" : "false",
        tempest_current_web(s)->lane_count, (unsigned long)s->score, (unsigned long)s->high_score,
        s->lives, s->zaps, enemies, shots, hostile, spikes, (long)s->claw_q8,
        (unsigned long)tempest_hash(s), frozen ? "true" : "false", s->effects ? "true" : "false",
        s->settings_open ? "true" : "false", s->settings.fire_rate, s->settings.enemy_speed_percent,
        s->settings.tilt_sensitivity_percent, s->phase_ticks, (long)s->dive_depth, fx,
        s->zap_flash);
    return n >= 0 && (size_t)n < cap;
}
const engine_game_ops engine_tempest_game = {.id = ENGINE_GAME_TEMPEST,
                                             .slug = "tempest",
                                             .title = "Tempest",
                                             .year = 1981,
                                             .capabilities =
                                                 ENGINE_CAP_SPECIAL | ENGINE_CAP_EFFECTS,
                                             .state_size = sizeof(tempest_state),
                                             .palette = engine_palette,
                                             .init = init,
                                             .step = step,
                                             .render = render,
                                             .draw_title = engine_title_tempest,
                                             .view = view,
                                             .sync_ui = sync_ui,
                                             .set = set,
                                             .level = level,
                                             .hash = hash,
                                             .state_json = state_json};
