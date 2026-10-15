#include "gyrus/debug_game.h"
#include "gyrus/assets.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
static void write_bytes(void *ctx, const void *p, size_t n) {
    gyrus_debug_game *a = ctx;
    if (a->write)
        a->write(a->write_context, p, n);
}
static bool state(void *ctx, char *out, size_t cap) {
    gyrus_debug_game *a = ctx;
    const game_state *s = a->game;
    unsigned enemies = 0, shots = 0, hostile = 0, modes[6] = {0};
    for (unsigned i = 0; i < GAME_ENEMIES; i++) {
        const game_enemy *e = &s->enemies[i];
        enemies += e->active != 0;
        if (e->active && e->kind <= 2u && e->mode < 6u)
            ++modes[e->mode];
    }
    for (unsigned i = 0; i < GAME_SHOTS; i++)
        shots += s->shots[i].active != 0;
    for (unsigned i = 0; i < GAME_HOSTILE_SHOTS; i++)
        hostile += s->hostile[i].active != 0;
    int n = snprintf(
        out, cap,
        "{\"tick\":%" PRIu32 ",\"rng\":%" PRIu32 ",\"score\":%" PRIu32 ",\"high_score\":%" PRIu32
        ",\"lives\":%u,\"stage\":%u,\"planet\":%u,\"warp\":%u,\"double_fire\":%u,\"phase\":\"%s\","
        "\"ship_angle\":%u,\"phase_ticks\":%u,"
        "\"invincible\":%s,\"autofire\":%s,\"frozen\":%s,\"control_mode\":%u,\"reboot_progress\":%"
        "u,\"tilt_ready\":%s,\"enemies\":%u,\"shots\":%u,\"hostile_shots\":%u,\"hash\":%" PRIu32
        ",\"pause_settings\":%s,\"fire_rate\":%u,\"enemy_speed\":%u,\"tilt_sensitivity\":%u,\"tilt_"
        "progress\":%u,\"tilt_preview\":%d,"
        "\"spawned\":%u,\"destroyed\":%u,\"escaped\":%u,\"entrance_complete\":%s,"
        "\"enemy_modes\":[%u,%u,%u,%u,%u,%u],\"stage_target\":%u,"
        "\"entrance_kills\":[%u,%u,%u,%u,%u]}",
        s->tick, s->rng, s->score, s->high_score, (unsigned)s->lives, (unsigned)s->stage,
        (unsigned)s->planet, (unsigned)s->warp, (unsigned)s->double_fire, game_phase_name(s->phase),
        (unsigned)s->ship_angle, (unsigned)s->phase_ticks, s->invincible ? "true" : "false",
        s->autofire ? "true" : "false", a->frozen ? "true" : "false", (unsigned)s->control_mode,
        (unsigned)s->reboot_progress, s->tilt_ready ? "true" : "false", enemies, shots, hostile,
        game_state_hash(s), s->pause_settings ? "true" : "false", (unsigned)s->settings.fire_rate,
        (unsigned)s->settings.enemy_speed_percent, (unsigned)s->settings.tilt_sensitivity_percent,
        (unsigned)s->tilt_progress, (int)s->tilt_preview, (unsigned)s->spawned,
        (unsigned)s->destroyed, (unsigned)s->escaped, s->entrance_complete ? "true" : "false",
        modes[0], modes[1], modes[2], modes[3], modes[4], modes[5], (unsigned)s->stage_target,
        (unsigned)s->entrance_kills[0], (unsigned)s->entrance_kills[1],
        (unsigned)s->entrance_kills[2], (unsigned)s->entrance_kills[3],
        (unsigned)s->entrance_kills[4]);
    return n > 0 && (size_t)n < cap;
}
static bool set(void *ctx, const char *key, int32_t n) {
    gyrus_debug_game *a = ctx;
    game_state *s = a->game;
    if (!strcmp(key, "clear_stage"))
        return n == 1 && game_debug_complete_stage(s);
    if (!strcmp(key, "fire_rate"))
        return game_set_setting(s, GAME_SETTING_FIRE_RATE, n);
    if (!strcmp(key, "enemy_speed"))
        return game_set_setting(s, GAME_SETTING_ENEMY_SPEED, n);
    if (!strcmp(key, "tilt_sensitivity"))
        return game_set_setting(s, GAME_SETTING_TILT_SENSITIVITY, n);
    if (!strcmp(key, "score") && n >= 0 && n <= 99999999)
        s->score = (uint32_t)n;
    else if (!strcmp(key, "high_score") && n >= 0 && n <= 99999999)
        s->high_score = (uint32_t)n;
    else if (!strcmp(key, "lives") && n >= 0 && n <= 99)
        s->lives = (uint8_t)n;
    else if (!strcmp(key, "ship_angle") && n >= 0 && n <= 65535)
        s->ship_angle = (uint16_t)n;
    else if (!strcmp(key, "invincible") && (n == 0 || n == 1))
        s->invincible = n != 0;
    else if (!strcmp(key, "autofire") && (n == 0 || n == 1))
        s->autofire = n != 0;
    else if (!strcmp(key, "double_fire") && (n == 0 || n == 1))
        s->double_fire = (uint8_t)n;
    else if (!strcmp(key, "control_mode") && n >= 0 && n < CONTROLS_MODE_COUNT) {
        s->control_mode = (uint8_t)n;
        if (a->controls)
            controls_set_mode(a->controls, (controls_mode)n);
    } else
        return false;
    return true;
}
void gyrus_debug_game_tick(gyrus_debug_game *a) {
    game_step(a->game, &a->input);
    a->input.angle_delta = 0;
    a->input.start = false;
    a->input.pause = false;
    a->input.quit = false;
    a->input.menu_action = GAME_MENU_NONE;
}
static bool action(void *ctx, gyrus_debug_action action, uint32_t n) {
    gyrus_debug_game *a = ctx;
    switch (action) {
    case GYRUS_DEBUG_STEP:
        for (uint32_t i = 0; i < n; i++)
            gyrus_debug_game_tick(a);
        return true;
    case GYRUS_DEBUG_PAUSE:
        a->frozen = true;
        return true;
    case GYRUS_DEBUG_RESUME:
        a->frozen = false;
        return true;
    case GYRUS_DEBUG_LEVEL:
        return game_set_stage(a->game, n);
    case GYRUS_DEBUG_GAME:
    case GYRUS_DEBUG_HOME:
        return false;
    case GYRUS_DEBUG_RESET:
        game_init(a->game, n);
        a->input = (game_input){0};
        return true;
    }
    return false;
}
static bool input(void *ctx, const gyrus_debug_input *i) {
    game_input *out = &((gyrus_debug_game *)ctx)->input;
    _Static_assert(GAME_MENU_TILT_UP == 8, "G1 menu codes must match game menu actions");
    if (i->mask & GYRUS_INPUT_MENU)
        out->menu_action = (game_menu_action)i->menu_action;
    if (i->mask & GYRUS_INPUT_TURN)
        out->turn = i->turn;
    if (i->mask & GYRUS_INPUT_ANGLE)
        out->angle_delta = i->angle_delta;
    if (i->mask & GYRUS_INPUT_FIRE)
        out->fire = i->fire;
    if (i->mask & GYRUS_INPUT_START)
        out->start = i->start;
    if (i->mask & GYRUS_INPUT_PAUSE)
        out->pause = i->pause;
    if (i->mask & GYRUS_INPUT_QUIT)
        out->quit = i->quit;
    return true;
}
static bool frame(void *ctx, gyrus_debug_frame *out) {
    gyrus_debug_game *a = ctx;
    if (!a->framebuffer)
        return false;
    game_render(a->game, a->framebuffer);
    *out = (gyrus_debug_frame){GAME_WIDTH,    GAME_HEIGHT,    256,
                               gyrus_palette, a->framebuffer, GAME_WIDTH * GAME_HEIGHT};
    return true;
}
void gyrus_debug_game_init(gyrus_debug_game *a, game_state *s, uint8_t *fb,
                           void (*write)(void *, const void *, size_t), void *ctx) {
    *a = (gyrus_debug_game){.game = s, .framebuffer = fb, .write = write, .write_context = ctx};
}
gyrus_debug_callbacks gyrus_debug_game_callbacks(gyrus_debug_game *a) {
    return (gyrus_debug_callbacks){a, write_bytes, state, set, action, input, frame, NULL};
}
