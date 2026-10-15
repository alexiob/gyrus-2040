#include "engine/debug_session.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
static void write_bytes(void *ctx, const void *bytes, size_t length) {
    engine_debug_session *a = ctx;
    if (a->write)
        a->write(a->write_context, bytes, length);
}
static bool state(void *ctx, char *out, size_t cap) {
    return engine_state_json(((engine_debug_session *)ctx)->engine, out, cap);
}
static bool set(void *ctx, const char *key, int32_t value) {
    return engine_set(((engine_debug_session *)ctx)->engine, key, value);
}
static bool action(void *ctx, gyrus_debug_action command, uint32_t value) {
    engine_state *s = ((engine_debug_session *)ctx)->engine;
    switch (command) {
    case GYRUS_DEBUG_STEP:
        if (!value || value > 3600)
            return false;
        for (uint32_t i = 0; i < value; ++i)
            engine_tick(s);
        return true;
    case GYRUS_DEBUG_PAUSE:
        s->frozen = true;
        return true;
    case GYRUS_DEBUG_RESUME:
        s->frozen = false;
        return true;
    case GYRUS_DEBUG_LEVEL:
        return engine_level(s, value);
    case GYRUS_DEBUG_RESET:
        s->records[s->selected] = (engine_game_record){.settings = {5, 100, 100}};
        s->seed = value;
        s->tilt_sensitivity = 100;
        s->input = (engine_input){0};
        s->ui = (engine_ui){0};
        controls_init(&s->controls);
        s->ticks = s->hub_tick = 0;
        s->slide_pixels = 0;
        s->hub = s->hub_settings = false;
        memset(&s->game, 0, sizeof s->game);
        engine_current_game(s)->init(&s->game, value);
        return true;
    case GYRUS_DEBUG_GAME:
        return engine_select_game(s, (engine_game_id)value, s->seed);
    case GYRUS_DEBUG_HOME:
        engine_home(s);
        return true;
    }
    return false;
}
static bool input(void *ctx, const gyrus_debug_input *in) {
    engine_input *out = &((engine_debug_session *)ctx)->engine->input;
    if (in->mask & GYRUS_INPUT_TURN)
        out->turn = in->turn;
    if (in->mask & GYRUS_INPUT_ANGLE)
        out->angle_delta = in->angle_delta;
    if (in->mask & GYRUS_INPUT_FIRE)
        out->fire = in->fire;
    if (in->mask & GYRUS_INPUT_START)
        out->start = in->start;
    if (in->mask & GYRUS_INPUT_PAUSE)
        out->pause = in->pause;
    if (in->mask & GYRUS_INPUT_QUIT)
        out->quit = in->quit;
    if (in->mask & GYRUS_INPUT_MENU)
        out->menu_action = (game_menu_action)in->menu_action;
    if (in->mask & GYRUS_INPUT_SPECIAL)
        out->special = in->special;
    if (in->mask & GYRUS_INPUT_HOME)
        out->home = in->home;
    if (in->mask & GYRUS_INPUT_NAVIGATE)
        out->navigate = in->navigate;
    return true;
}
static bool frame(void *ctx, gyrus_debug_frame *out) {
    engine_debug_session *a = ctx;
    if (!a->framebuffer)
        return false;
    const uint16_t *palette = engine_render(a->engine, a->framebuffer);
    *out = (gyrus_debug_frame){ENGINE_WIDTH, ENGINE_HEIGHT,  256,
                               palette,      a->framebuffer, ENGINE_WIDTH * ENGINE_HEIGHT};
    return true;
}
static bool catalogue(void *ctx, char *out, size_t cap) {
    const engine_state *s = ((engine_debug_session *)ctx)->engine;
    size_t count;
    const engine_game_ops *const *games = engine_catalogue(&count);
    int n =
        snprintf(out, cap, "{\"selected\":%u,\"hub\":%s,\"frozen\":%s,\"games\":[",
                 (unsigned)s->selected, s->hub ? "true" : "false", s->frozen ? "true" : "false");
    if (n < 0 || (size_t)n >= cap)
        return false;
    size_t used = (size_t)n;
    for (size_t i = 0; i < count; ++i) {
        const engine_game_ops *ops = games[i];
        uint32_t high =
            i == (size_t)s->selected ? engine_current_view(s).high_score : s->records[i].high_score;
        n = snprintf(out + used, cap - used,
                     "%s{\"id\":%u,\"slug\":\"%s\",\"title\":\"%s\",\"year\":%u,"
                     "\"capabilities\":%u,\"high_score\":%" PRIu32 ",\"effects\":%s}",
                     i ? "," : "", (unsigned)ops->id, ops->slug, ops->title, (unsigned)ops->year,
                     ops->capabilities, high, s->records[i].neon ? "true" : "false");
        if (n < 0 || (size_t)n >= cap - used)
            return false;
        used += (size_t)n;
    }
    n = snprintf(out + used, cap - used, "]}");
    return n > 0 && (size_t)n < cap - used;
}
void engine_debug_session_init(engine_debug_session *a, engine_state *s, uint8_t *pixels,
                               void (*writer)(void *, const void *, size_t), void *ctx) {
    *a = (engine_debug_session){s, pixels, writer, ctx};
}
gyrus_debug_callbacks engine_debug_session_callbacks(engine_debug_session *a) {
    return (gyrus_debug_callbacks){.context = a,
                                   .write = write_bytes,
                                   .state = state,
                                   .set = set,
                                   .action = action,
                                   .input = input,
                                   .frame = frame,
                                   .catalogue = catalogue};
}
