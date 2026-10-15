#include "engine/session.h"
#include <assert.h>
#include <string.h>
static void selection_and_records(void) {
    engine_state s, before;
    engine_init(&s, 42);
    assert(s.hub && s.selected == ENGINE_GAME_GYRUS);
    before = s;
    assert(!engine_select_game(&s, (engine_game_id)ENGINE_GAME_COUNT, 7));
    assert(!memcmp(&s, &before, sizeof s));
    assert(engine_select_game(&s, ENGINE_GAME_GYRUS, 42));
    assert(!s.hub && engine_current_view(&s).title);
    assert(engine_set(&s, "high_score", 12345));
    assert(engine_set(&s, "fire_rate", 8));
    assert(engine_set(&s, "tilt_sensitivity", 150));
    assert(engine_select_game(&s, ENGINE_GAME_TEMPEST, 42));
    assert(engine_current_view(&s).settings.fire_rate == 5);
    assert(engine_current_view(&s).settings.tilt_sensitivity_percent == 150);
    assert(engine_current_view(&s).high_score == 0);
    assert(engine_set(&s, "high_score", 99));
    assert(engine_select_game(&s, ENGINE_GAME_GYRUS, 42));
    assert(engine_current_view(&s).high_score == 12345);
    assert(engine_current_view(&s).settings.fire_rate == 8);
    engine_home(&s);
    assert(s.hub && engine_controls_screen(&s) == CONTROLS_HUB);
    engine_step(&s, &(engine_input){.navigate = 1});
    assert(s.hub && s.selected == ENGINE_GAME_TEMPEST && s.slide_pixels != 0);
    uint32_t hash = engine_current_game(&s)->hash(&s.game);
    engine_step(&s, &(engine_input){.menu_action = GAME_MENU_EFFECTS});
    assert(s.records[s.selected].neon);
    assert(engine_current_game(&s)->hash(&s.game) == hash);
    engine_step(&s, &(engine_input){.start = true});
    assert(!s.hub && !engine_current_view(&s).title);
    assert(!engine_set(&s, "effects", 0));
}
static void retained_and_frozen(void) {
    engine_state s;
    engine_init(&s, 42);
    assert(engine_select_game(&s, ENGINE_GAME_GYRUS, 42));
    s.input = (engine_input){.start = true, .angle_delta = 100, .turn = 1, .fire = true};
    engine_tick(&s);
    assert(!s.input.start && !s.input.angle_delta && s.input.turn == 1 && s.input.fire);
    s.input = (engine_input){.home = true};
    engine_tick(&s);
    assert(s.hub && !s.input.home);
    s.frozen = true;
    uint32_t ticks = s.ticks;
    (void)engine_update(&s, (controls_sample){.now_ms = 1}, NULL);
    assert(s.ticks == ticks);
    assert(engine_set(&s, "control_mode", CONTROLS_TILT));
    for (uint32_t now = 16; now <= 640; now += 16) {
        controls_sample sample = {.now_ms = now,
                                  .motion_valid = true,
                                  .motion = {.sampled_ms = now, .accel_mg = {0, 0, 1000}}};
        (void)engine_update(&s, sample, NULL);
    }
    assert(s.ui.control_mode == CONTROLS_TILT && s.ui.tilt_ready);
    assert(s.ticks == ticks);
    engine_tick(&s);
    assert(s.ticks == ticks + 1);
}
static void catalogue_and_settings(void) {
    size_t count;
    const engine_game_ops *const *games = engine_catalogue(&count);
    assert(count == ENGINE_GAME_COUNT && games[0]->id == 0 && games[1]->id == 1);
    game_settings settings = {5, 100, 100};
    assert(engine_settings_set(&settings, GAME_SETTING_FIRE_RATE, 5));
    assert(!engine_settings_set(&settings, GAME_SETTING_ENEMY_SPEED, 26));
    assert(!engine_settings_set(&settings, GAME_SETTING_TILT_SENSITIVITY, 25));
    assert(engine_settings_menu(&settings, GAME_MENU_FIRE_UP) && settings.fire_rate == 6);
}
static void gyrus_dispatch_and_pure_state(void) {
    engine_state s;
    game_state legacy;
    engine_init(&s, 77);
    assert(engine_select_game(&s, ENGINE_GAME_GYRUS, 77));
    game_init(&legacy, 77);
    for (unsigned i = 0; i < 240; ++i) {
        engine_input in = {.start = i == 0,
                           .turn = (int8_t)(i < 120 ? 1 : -1),
                           .fire = true,
                           .pause = i == 80 || i == 90};
        engine_step(&s, &in);
        game_step(&legacy, &in);
        assert(engine_current_game(&s)->hash(&s.game) == game_state_hash(&legacy));
    }
    engine_state before = s;
    char json[1536], short_json[2];
    assert(engine_state_json(&s, json, sizeof json));
    assert(!engine_state_json(&s, short_json, sizeof short_json));
    assert(!memcmp(&s, &before, sizeof s));
}
static void held_boot_and_physical_home(void) {
    engine_state s;
    engine_init(&s, 1);
    assert(engine_select_game(&s, ENGINE_GAME_GYRUS, 1));
    (void)engine_update(&s, (controls_sample){.now_ms = 0, .fire_pressed = true}, NULL);
    (void)engine_update(&s, (controls_sample){.now_ms = 30, .fire_pressed = true}, NULL);
    assert(engine_current_view(&s).title);
    assert(engine_select_game(&s, ENGINE_GAME_TEMPEST, 1));
    (void)engine_update(&s, (controls_sample){.now_ms = 60, .fire_pressed = true}, NULL);
    (void)engine_update(&s, (controls_sample){.now_ms = 90, .fire_pressed = true}, NULL);
    assert(engine_current_view(&s).title);
    (void)engine_update(&s, (controls_sample){.now_ms = 120}, NULL);
    (void)engine_update(&s, (controls_sample){.now_ms = 150}, NULL);
    (void)engine_update(&s, (controls_sample){.now_ms = 180, .fire_pressed = true}, NULL);
    (void)engine_update(&s, (controls_sample){.now_ms = 210, .fire_pressed = true}, NULL);
    assert(!engine_current_view(&s).title);
    engine_step(&s, &(engine_input){.pause = true});
    assert(engine_current_view(&s).paused);
    (void)engine_update(&s, (controls_sample){.now_ms = 240}, NULL);
    (void)engine_update(
        &s, (controls_sample){.now_ms = 270, .touch_pressed = true, .touch_x = 120, .touch_y = 160},
        NULL);
    (void)engine_update(
        &s, (controls_sample){.now_ms = 300, .touch_pressed = true, .touch_x = 120, .touch_y = 160},
        NULL);
    (void)engine_update(&s, (controls_sample){.now_ms = 330}, NULL);
    assert(s.hub);
    assert(engine_select_game(&s, ENGINE_GAME_GYRUS, 1));
    engine_step(&s, &(engine_input){.start = true});
    engine_step(&s, &(engine_input){.pause = true});
    engine_step(&s, &(engine_input){.quit = true});
    assert(!s.hub && engine_current_view(&s).title);
}
static void invalid_pointer_contact_does_not_launch(void) {
    engine_state s;
    engine_init(&s, 1);
    (void)engine_update(&s, (controls_sample){.now_ms = 0}, NULL);
    (void)engine_update(
        &s, (controls_sample){.now_ms = 30, .touch_pressed = true, .touch_x = 120, .touch_y = 160},
        NULL);
    (void)engine_update(
        &s, (controls_sample){.now_ms = 60, .touch_pressed = true, .touch_x = 120, .touch_y = 160},
        NULL);
    /* Desktop drag outside its viewport, then a real mouse release. */
    (void)engine_update(
        &s, (controls_sample){.now_ms = 90, .touch_pressed = true, .touch_invalid = true}, NULL);
    (void)engine_update(&s, (controls_sample){.now_ms = 120}, NULL);
    assert(s.hub);
    (void)engine_update(
        &s, (controls_sample){.now_ms = 150, .touch_pressed = true, .touch_x = 120, .touch_y = 160},
        NULL);
    (void)engine_update(
        &s, (controls_sample){.now_ms = 180, .touch_pressed = true, .touch_x = 120, .touch_y = 160},
        NULL);
    (void)engine_update(&s, (controls_sample){.now_ms = 210}, NULL);
    assert(!s.hub);
}
int main(void) {
    catalogue_and_settings();
    selection_and_records();
    retained_and_frozen();
    gyrus_dispatch_and_pure_state();
    held_boot_and_physical_home();
    invalid_pointer_contact_does_not_launch();
    return 0;
}
