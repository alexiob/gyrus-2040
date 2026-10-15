#include "gyrus/controls.h"
#include "gyrus/game.h"
#include <assert.h>
#include <string.h>
static void defaults_bounds_and_preservation(void) {
    game_state g;
    game_init(&g, 1);
    assert(g.settings.fire_rate == 5 && g.settings.enemy_speed_percent == 100 &&
           g.settings.tilt_sensitivity_percent == 100);
    uint32_t before = game_state_hash(&g);
    assert(!game_set_setting(&g, GAME_SETTING_FIRE_RATE, 0));
    assert(!game_set_setting(&g, GAME_SETTING_FIRE_RATE, 11));
    assert(!game_set_setting(&g, GAME_SETTING_ENEMY_SPEED, 26));
    assert(!game_set_setting(&g, GAME_SETTING_ENEMY_SPEED, 225));
    assert(!game_set_setting(&g, GAME_SETTING_TILT_SENSITIVITY, 25));
    assert(!game_set_setting(&g, (game_setting)99, 100));
    assert(game_state_hash(&g) == before);
    assert(game_set_setting(&g, GAME_SETTING_FIRE_RATE, 7));
    assert(game_set_setting(&g, GAME_SETTING_ENEMY_SPEED, 25));
    assert(game_set_setting(&g, GAME_SETTING_TILT_SENSITIVITY, 200));
    game_start(&g);
    assert(g.settings.fire_rate == 7 && g.settings.enemy_speed_percent == 25 &&
           g.settings.tilt_sensitivity_percent == 200);
    game_step(&g, &(game_input){.quit = true});
    game_step(&g, &(game_input){.start = true});
    assert(g.settings.fire_rate == 7 && g.settings.enemy_speed_percent == 25 &&
           g.settings.tilt_sensitivity_percent == 200);
}
static void pause_actions(void) {
    game_state g;
    game_init(&g, 1);
    game_start(&g);
    game_step(&g, &(game_input){.pause = true});
    uint32_t tick = g.tick;
    game_step(&g, &(game_input){.menu_action = GAME_MENU_SETTINGS});
    assert(g.pause_settings && g.phase == GAME_PAUSED && g.tick == tick);
    game_step(&g, &(game_input){.menu_action = GAME_MENU_FIRE_DOWN});
    game_step(&g, &(game_input){.menu_action = GAME_MENU_ENEMY_DOWN});
    game_step(&g, &(game_input){.menu_action = GAME_MENU_TILT_UP});
    assert(g.settings.fire_rate == 4 && g.settings.enemy_speed_percent == 75 &&
           g.settings.tilt_sensitivity_percent == 125);
    assert(g.tick == tick && g.phase == GAME_PAUSED);
    for (unsigned i = 0; i < 20; ++i)
        game_step(&g, &(game_input){.menu_action = GAME_MENU_FIRE_DOWN});
    assert(g.settings.fire_rate == 1);
    game_step(&g, &(game_input){.menu_action = GAME_MENU_BACK});
    assert(!g.pause_settings && g.phase == GAME_PAUSED);
    game_step(&g, &(game_input){.menu_action = GAME_MENU_FIRE_UP});
    assert(g.settings.fire_rate == 1); /* Adjustments only belong to Settings page. */
    game_step(&g, &(game_input){.menu_action = GAME_MENU_SETTINGS});
    game_step(&g, &(game_input){.pause = true});
    assert(!g.pause_settings && g.phase == GAME_INTRO);
}
static void title_settings(void) {
    for (unsigned over = 0; over < 2; ++over) {
        game_state g;
        game_init(&g, 42);
        g.phase = over ? GAME_OVER : GAME_TITLE;
        const game_phase parent = g.phase;
        game_step(&g, &(game_input){.menu_action = GAME_MENU_SETTINGS});
        assert(g.pause_settings && g.phase == parent);
        const uint32_t tick = g.tick;
        game_step(&g, &(game_input){.menu_action = GAME_MENU_FIRE_UP});
        game_step(&g, &(game_input){.start = true});
        assert(g.phase == parent && g.pause_settings && g.tick == tick);
        assert(g.settings.fire_rate == 6);
        game_step(&g, &(game_input){.menu_action = GAME_MENU_BACK});
        assert(!g.pause_settings && g.phase == parent);
        game_step(&g, &(game_input){.start = true});
        assert(g.phase == GAME_INTRO && g.settings.fire_rate == 6);
    }
}
static void exact_fire_cadence(void) {
    for (int32_t rate = 1; rate <= 10; ++rate) {
        game_state g;
        game_init(&g, 1);
        game_start(&g);
        g.phase = GAME_PLAY;
        g.invincible = true;
        g.stage_target = 65535;
        assert(game_set_setting(&g, GAME_SETTING_FIRE_RATE, rate));
        unsigned fired = 0;
        for (unsigned tick = 0; tick < 600; ++tick) {
            memset(g.enemies, 0, sizeof g.enemies);
            game_step(&g, NULL);
            for (unsigned shot = 0; shot < GAME_SHOTS; ++shot)
                if (g.shots[shot].active && g.shots[shot].age == 1)
                    ++fired;
            memset(g.shots, 0, sizeof g.shots);
        }
        assert(fired == (unsigned)rate * 10u);
    }
}
static void live_enemy_speed(void) {
    game_state g;
    game_init(&g, 1);
    game_start(&g);
    g.phase = GAME_PLAY;
    g.autofire = false;
    g.phase_ticks = 1;
    g.ship_angle = 0;
    g.enemies[0] = (game_enemy){.active = 1, .kind = 0, .mode = 2, .radius = 60 * 256, .hp = 1};
    g.hostile[0] = (game_shot){.active = 1, .radius = 20 * 256, .angle = 16384};
    assert(game_set_setting(&g, GAME_SETTING_ENEMY_SPEED, 25));
    game_step(&g, NULL);
    assert(g.enemies[0].radius == 60 * 256 && g.hostile[0].radius == 20 * 256);
    for (unsigned i = 0; i < 3; ++i)
        game_step(&g, NULL);
    assert(g.enemy_ticks == 1);
    assert(g.enemies[0].radius == 60 * 256 + 256 && g.hostile[0].radius == 20 * 256 + 256);
    assert(game_set_setting(&g, GAME_SETTING_ENEMY_SPEED, 200));
    game_step(&g, NULL);
    assert(g.enemies[0].radius == 60 * 256 + 256 + 512 &&
           g.hostile[0].radius == 20 * 256 + 256 + 512);
}
static void touch_pause_flow(void) {
    game_state g;
    game_init(&g, 1);
    game_start(&g);
    game_step(&g, &(game_input){.pause = true});
    controls_state controls;
    controls_init(&controls);
    controls_sample sample = {0};
    (void)controls_update(&controls, sample, CONTROLS_PAUSE);
    sample = (controls_sample){.now_ms = 10, .touch_pressed = true, .touch_x = 120, .touch_y = 127};
    (void)controls_update(&controls, sample, CONTROLS_PAUSE);
    sample.now_ms = 30;
    sample.touch_pressed = false;
    controls_output out = controls_update(&controls, sample, CONTROLS_PAUSE);
    game_step(&g, &out.input);
    assert(g.pause_settings);
    sample.now_ms = 40;
    (void)controls_update(&controls, sample, CONTROLS_SETTINGS);
    sample = (controls_sample){.now_ms = 50, .touch_pressed = true, .touch_x = 180, .touch_y = 107};
    (void)controls_update(&controls, sample, CONTROLS_SETTINGS);
    sample.now_ms = 70;
    sample.touch_pressed = false;
    out = controls_update(&controls, sample, CONTROLS_SETTINGS);
    game_step(&g, &out.input);
    assert(g.settings.fire_rate == 6 && g.phase == GAME_PAUSED);
}
static void idle_enemy_tick_still_collides(void) {
    game_state g;
    game_init(&g, 1);
    game_start(&g);
    g.phase = GAME_PLAY;
    g.invulnerable = 0;
    g.ship_angle = 64936;
    g.hostile[0] = (game_shot){.active = 1, .radius = 104 * 256, .angle = 0};
    assert(game_set_setting(&g, GAME_SETTING_ENEMY_SPEED, 25));
    game_step(&g, &(game_input){.turn = 1});
    assert(g.lives == 2);
}
int main(void) {
    defaults_bounds_and_preservation();
    pause_actions();
    title_settings();
    exact_fire_cadence();
    live_enemy_speed();
    touch_pause_flow();
    idle_enemy_tick_still_collides();
    return 0;
}
