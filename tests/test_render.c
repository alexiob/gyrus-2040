#include "gyrus/assets.h"
#include "gyrus/controls.h"
#include "gyrus/game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint8_t before[16];
    uint8_t pixels[GAME_WIDTH * GAME_HEIGHT];
    uint8_t after[16];
} guarded_frame;

static void check_frame(const game_state *state) {
    static guarded_frame frame;
    static uint8_t repeated[GAME_WIDTH * GAME_HEIGHT];
    const game_state snapshot = *state;
    memset(&frame, 0xa5, sizeof frame);
    game_render(state, frame.pixels);
    assert(memcmp(&snapshot, state, sizeof snapshot) == 0);
    for (unsigned i = 0; i < 16u; ++i) {
        assert(frame.before[i] == 0xa5u);
        assert(frame.after[i] == 0xa5u);
    }
    unsigned lit = 0;
    for (unsigned y = 0; y < GAME_HEIGHT; ++y) {
        for (unsigned x = 0; x < GAME_WIDTH; ++x) {
            const uint8_t color = frame.pixels[y * GAME_WIDTH + x];
            assert(color < GYRUS_COLOR_COUNT);
            const int dx = (int)x - 120, dy = (int)y - 120;
            if (dx * dx + dy * dy > 120 * 120)
                assert(color == 0u);
            if (color != 0u)
                ++lit;
        }
    }
    assert(lit > 100u);
    game_render(state, repeated);
    assert(memcmp(repeated, frame.pixels, sizeof repeated) == 0);
    assert(memcmp(&snapshot, state, sizeof snapshot) == 0);
}

int main(void) {
    for (uint32_t seed = 1; seed <= 32u; ++seed) {
        game_state base;
        game_init(&base, seed);
        game_start(&base);
        base.invincible = true;
        base.high_score = 100000u;
        const game_input input = {.fire = true, .turn = 1};
        for (unsigned tick = 0; tick < 180u + seed; ++tick)
            game_step(&base, &input);
        /* Exercise every enemy visual role, a live laser pair and both bullet pools. */
        for (unsigned kind = 0; kind < 7u; ++kind) {
            base.enemies[kind] = (game_enemy){.radius = (int32_t)(40u + kind * 7u) * 256,
                                              .angle = (uint16_t)(kind * 8192u),
                                              .kind = (uint8_t)kind,
                                              .active = 1,
                                              .mode = (uint8_t)(seed % 5u),
                                              .age = (uint16_t)(seed * 5u),
                                              .angular_speed = 260};
        }
        base.enemies[7] = (game_enemy){.radius = 72 * 256, .angle = 18000, .kind = 5, .active = 1};
        base.shots[0] = (game_shot){.radius = 70 * 256, .angle = 40000, .active = 1};
        base.hostile[0] = (game_shot){.radius = 80 * 256, .angle = 50000, .active = 1};
        base.planet = (uint8_t)(seed % 6u);
        base.control_mode = (uint8_t)(seed % CONTROLS_MODE_COUNT);
        base.tilt_ready = (seed % 4u) != 0u;
        base.reboot_progress = (uint16_t)(seed * 40u);
        for (unsigned phase = 0; phase < 7u; ++phase) {
            base.phase = (game_phase)phase;
            base.resume_phase = GAME_PLAY;
            check_frame(&base);
        }
    }
    puts(
        "Renderer: all phases, circular bounds, palette, canaries, determinism and purity passed.");
    return 0;
}
