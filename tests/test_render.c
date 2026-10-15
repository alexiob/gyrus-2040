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

static void player_stays_visible(void) {
    static uint8_t pixels[GAME_WIDTH * GAME_HEIGHT];
    game_state g;
    game_init(&g, 1);
    g.phase = GAME_PLAY;
    g.invulnerable = 0;
    for (unsigned heading = 0; heading < 32; ++heading) {
        g.ship_angle = (uint16_t)(heading * 2048u);
        int16_t cx, cy;
        game_polar(g.ship_angle, 104 * 256, &cx, &cy);
        const gyrus_asset *a =
            gyrus_sprite(GYRUS_SPRITE_PLAYER, ((unsigned)g.ship_angle + 32768u + 1024u) >> 11u);
        game_render(&g, pixels);
        for (unsigned y = 0; y < a->height; ++y)
            for (unsigned x = 0; x < a->width; ++x) {
                uint8_t color = a->pixels[y * a->width + x];
                if (!color)
                    continue;
                int px = cx - (int)a->width / 2 + (int)x;
                int py = cy - (int)a->height / 2 + (int)y;
                assert(px >= 0 && px < 240 && py >= 0 && py < 240);
                assert(pixels[(unsigned)py * GAME_WIDTH + (unsigned)px] == color);
            }
    }
}
static void settings_pages(void) {
    static const unsigned rectangles[][4] = {
        {CONTROLS_FIRE_DOWN_X0, CONTROLS_FIRE_DOWN_Y0, CONTROLS_FIRE_DOWN_X1,
         CONTROLS_FIRE_DOWN_Y1},
        {CONTROLS_FIRE_UP_X0, CONTROLS_FIRE_UP_Y0, CONTROLS_FIRE_UP_X1, CONTROLS_FIRE_UP_Y1},
        {CONTROLS_ENEMY_DOWN_X0, CONTROLS_ENEMY_DOWN_Y0, CONTROLS_ENEMY_DOWN_X1,
         CONTROLS_ENEMY_DOWN_Y1},
        {CONTROLS_ENEMY_UP_X0, CONTROLS_ENEMY_UP_Y0, CONTROLS_ENEMY_UP_X1, CONTROLS_ENEMY_UP_Y1},
        {CONTROLS_TILT_DOWN_X0, CONTROLS_TILT_DOWN_Y0, CONTROLS_TILT_DOWN_X1,
         CONTROLS_TILT_DOWN_Y1},
        {CONTROLS_TILT_UP_X0, CONTROLS_TILT_UP_Y0, CONTROLS_TILT_UP_X1, CONTROLS_TILT_UP_Y1},
        {CONTROLS_BACK_X0, CONTROLS_BACK_Y0, CONTROLS_BACK_X1, CONTROLS_BACK_Y1},
    };
    static uint8_t pixels[GAME_WIDTH * GAME_HEIGHT];
    game_state g;
    game_init(&g, 2040);
    g.phase = GAME_PAUSED;
    g.pause_settings = true;
    for (unsigned rate = 1; rate <= 10u; ++rate) {
        g.settings.fire_rate = (uint8_t)rate;
        g.settings.enemy_speed_percent = 100;
        g.settings.tilt_sensitivity_percent = 100;
        check_frame(&g);
    }
    for (unsigned speed = 25; speed <= 200u; speed += 25u) {
        g.settings.enemy_speed_percent = (uint16_t)speed;
        check_frame(&g);
    }
    for (unsigned sensitivity = 50; sensitivity <= 200u; sensitivity += 25u) {
        g.settings.tilt_sensitivity_percent = (uint16_t)sensitivity;
        check_frame(&g);
    }
    game_render(&g, pixels);
    /* Every edge of each visible control agrees with its complete touch rectangle,
       including all four corners, without being lost to the circular aperture. */
    for (unsigned i = 0; i < sizeof rectangles / sizeof rectangles[0]; ++i) {
        const unsigned *r = rectangles[i];
        for (unsigned x = r[0]; x < r[2]; ++x) {
            assert(pixels[r[1] * GAME_WIDTH + x] != 0u);
            assert(pixels[(r[3] - 1u) * GAME_WIDTH + x] != 0u);
        }
        for (unsigned y = r[1]; y < r[3]; ++y) {
            assert(pixels[y * GAME_WIDTH + r[0]] != 0u);
            assert(pixels[y * GAME_WIDTH + r[2] - 1u] != 0u);
        }
    }
    g.settings = (game_settings){1, 25, 50};
    game_render(&g, pixels);
    assert(pixels[CONTROLS_FIRE_DOWN_Y0 * GAME_WIDTH + CONTROLS_FIRE_DOWN_X0] == GYRUS_COLOR_STEEL);
    assert(pixels[CONTROLS_ENEMY_DOWN_Y0 * GAME_WIDTH + CONTROLS_ENEMY_DOWN_X0] ==
           GYRUS_COLOR_STEEL);
    assert(pixels[CONTROLS_TILT_DOWN_Y0 * GAME_WIDTH + CONTROLS_TILT_DOWN_X0] == GYRUS_COLOR_STEEL);
    g.settings = (game_settings){10, 200, 200};
    game_render(&g, pixels);
    assert(pixels[CONTROLS_FIRE_UP_Y0 * GAME_WIDTH + CONTROLS_FIRE_UP_X0] == GYRUS_COLOR_STEEL);
    assert(pixels[CONTROLS_ENEMY_UP_Y0 * GAME_WIDTH + CONTROLS_ENEMY_UP_X0] == GYRUS_COLOR_STEEL);
    assert(pixels[CONTROLS_TILT_UP_Y0 * GAME_WIDTH + CONTROLS_TILT_UP_X0] == GYRUS_COLOR_STEEL);
}
int main(void) {
    player_stays_visible();
    settings_pages();
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
