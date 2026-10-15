#include "engine/debug_session.h"
#include "gyrus/debug_game.h"
#include <assert.h>
#include <string.h>
static char output[4096];
static size_t used;
static void write_output(void *ctx, const void *bytes, size_t length) {
    (void)ctx;
    assert(used + length < sizeof output);
    memcpy(output + used, bytes, length);
    used += length;
    output[used] = 0;
}
int main(void) {
    engine_state s;
    engine_debug_session adapter;
    static uint8_t pixels[ENGINE_WIDTH * ENGINE_HEIGHT];
    engine_init(&s, 41);
    engine_debug_session_init(&adapter, &s, pixels, write_output, NULL);
    gyrus_debug_callbacks cb = engine_debug_session_callbacks(&adapter);
    assert(cb.action(cb.context, GYRUS_DEBUG_GAME, 0));
    assert(!s.hub && engine_current_view(&s).title);
    char actual[GYRUS_DEBUG_STATE_MAX], expected[GYRUS_DEBUG_STATE_MAX];
    assert(cb.state(cb.context, actual, sizeof actual));
    gyrus_debug_game legacy;
    gyrus_debug_game_init(&legacy, &s.game.gyrus, pixels, NULL, NULL);
    gyrus_debug_callbacks old = gyrus_debug_game_callbacks(&legacy);
    assert(old.state(old.context, expected, sizeof expected));
    assert(!strcmp(actual, expected));
    assert(cb.action(cb.context, GYRUS_DEBUG_PAUSE, 0));
    assert(s.frozen);
    gyrus_debug_input in = {
        .mask = GYRUS_INPUT_START | GYRUS_INPUT_SPECIAL, .start = true, .special = true};
    assert(cb.input(cb.context, &in));
    assert(cb.action(cb.context, GYRUS_DEBUG_STEP, 2));
    assert(!s.input.start && !s.input.special && !engine_current_view(&s).title);
    assert(cb.action(cb.context, GYRUS_DEBUG_HOME, 0) && s.hub);
    assert(cb.catalogue(cb.context, actual, sizeof actual));
    assert(strstr(actual, "gyrus") && strstr(actual, "tempest"));
    gyrus_debug protocol;
    gyrus_debug_init(&protocol, &cb);
    gyrus_debug_feed(&protocol, "G1 1 GAMES\n", 11);
    assert(strstr(output, "gyrus") && strstr(output, "tempest"));
    assert(cb.action(cb.context, GYRUS_DEBUG_GAME, 1));
    assert(cb.set(cb.context, "fire_rate", 9));
    assert(cb.action(cb.context, GYRUS_DEBUG_RESET, 9));
    assert(engine_current_view(&s).settings.fire_rate == 5 && s.frozen && !s.hub);
    /* Two separately injected pulses must each fire, even on adjacent ticks. */
    assert(cb.action(cb.context, GYRUS_DEBUG_LEVEL, 1));
    in = (gyrus_debug_input){.mask = GYRUS_INPUT_SPECIAL, .special = true};
    assert(cb.input(cb.context, &in));
    assert(cb.action(cb.context, GYRUS_DEBUG_STEP, 1));
    assert(s.game.tempest.zaps == 1);
    assert(cb.input(cb.context, &in));
    assert(cb.action(cb.context, GYRUS_DEBUG_STEP, 1));
    assert(s.game.tempest.zaps == 0);
    return 0;
}
