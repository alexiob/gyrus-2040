#include "gyrus/controls.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static controls_state state;
static controls_sample sample;
static controls_output step(controls_screen screen, uint32_t ms, bool touch, unsigned x, unsigned y,
                            bool fire) {
    sample = (controls_sample){.now_ms = ms,
                               .touch_pressed = touch,
                               .touch_x = (uint16_t)x,
                               .touch_y = (uint16_t)y,
                               .fire_pressed = fire};
    return controls_update(&state, sample, screen);
}
static void init(void) {
    controls_init(&state);
    (void)step(CONTROLS_PLAY, 0, false, 0, 0, false);
}
static void split_hold_and_release(void) {
    init();
    assert(step(CONTROLS_PLAY, 17, true, 30, 120, false).input.turn == 1);
    assert(step(CONTROLS_PLAY, 34, true, 210, 120, false).input.turn == -1);
    assert(step(CONTROLS_PLAY, 51, true, 120, 45, false).input.turn == 0);
    assert(step(CONTROLS_PLAY, 68, false, 0, 0, false).input.turn == 0);
    assert(step(CONTROLS_PLAY, 85, true, 120, 120, false).input.turn == 0);
    assert(step(CONTROLS_PLAY, 102, false, 0, 0, false).input.pause);
}
static void menu_actions_and_transition_consumption(void) {
    controls_init(&state);
    (void)step(CONTROLS_TITLE, 0, false, 0, 0, false);
    (void)step(CONTROLS_TITLE, 17, true, 120, 160, false);
    controls_output opened = step(CONTROLS_TITLE, 50, false, 0, 0, false);
    assert(opened.input.menu_action == GAME_MENU_SETTINGS && !opened.mode_changed);
    (void)step(CONTROLS_SETTINGS, 60, false, 0, 0, false);
    (void)step(CONTROLS_SETTINGS, 70, true, 120, 64, false);
    controls_output mode = step(CONTROLS_SETTINGS, 90, false, 0, 0, false);
    assert(mode.mode_changed && state.mode == CONTROLS_RELATIVE_DRAG);
    assert(strcmp(controls_mode_name(state.mode), "BORDER DRAG") == 0);
    (void)step(CONTROLS_TITLE, 100, false, 0, 0, false);
    (void)step(CONTROLS_TITLE, 110, true, 120, 110, false);
    assert(step(CONTROLS_TITLE, 150, false, 0, 0, false).input.start);
    (void)step(CONTROLS_PLAY, 160, true, 120, 120, false);
    assert(!step(CONTROLS_PAUSE, 180, false, 0, 0, false).input.pause);
    (void)step(CONTROLS_PAUSE, 200, true, 120, 92, false);
    assert(step(CONTROLS_PAUSE, 250, false, 0, 0, false).input.pause);
    assert(!step(CONTROLS_PAUSE, 270, false, 0, 0, false).input.pause);
}
static void quit_tap_reboot_hold_and_cancel(void) {
    init();
    (void)step(CONTROLS_PAUSE, 0, false, 0, 0, false);
    (void)step(CONTROLS_PAUSE, 100, true, 120, 170, false);
    assert(step(CONTROLS_PAUSE, 200, false, 0, 0, false).input.quit);
    assert(!step(CONTROLS_PAUSE, 210, false, 0, 0, false).input.quit);
    (void)step(CONTROLS_PAUSE, 1000, true, 120, 193, false);
    assert(controls_reboot_progress(&state, 3500) == 500);
    assert(!step(CONTROLS_PAUSE, 5999, true, 120, 193, false).reboot_requested);
    assert(step(CONTROLS_PAUSE, 6000, true, 120, 193, false).reboot_requested);
    assert(!step(CONTROLS_PAUSE, 6100, true, 120, 193, false).reboot_requested);
    assert(!step(CONTROLS_PAUSE, 6200, false, 0, 0, false).input.quit);
    (void)step(CONTROLS_PAUSE, 7000, true, 120, 193, false);
    (void)step(CONTROLS_PAUSE, 7020, true, 200, 193, false);
    assert(controls_reboot_progress(&state, 7500) == 0);
    assert(!step(CONTROLS_PAUSE, 13000, true, 120, 193, false).reboot_requested);
    assert(!step(CONTROLS_PAUSE, 13010, false, 0, 0, false).input.quit);
    /* A brief REBOOT tap does nothing. */
    (void)step(CONTROLS_PAUSE, 14000, true, 120, 193, false);
    assert(!step(CONTROLS_PAUSE, 14100, false, 0, 0, false).reboot_requested);
    assert(!step(CONTROLS_PAUSE, 14110, false, 0, 0, false).input.quit);
}
static void physical_fire_debounce_and_context(void) {
    init();
    assert(!step(CONTROLS_PLAY, 10, false, 0, 0, true).input.fire);
    assert(!step(CONTROLS_PLAY, 20, false, 0, 0, false).input.fire);
    assert(!step(CONTROLS_PLAY, 30, false, 0, 0, true).input.fire);
    assert(!step(CONTROLS_PLAY, 54, false, 0, 0, true).input.fire);
    assert(step(CONTROLS_PLAY, 55, false, 0, 0, true).input.fire);
    assert(step(CONTROLS_PLAY, 100, false, 0, 0, true).input.fire);
    (void)step(CONTROLS_PLAY, 110, false, 0, 0, false);
    assert(!step(CONTROLS_PLAY, 135, false, 0, 0, false).input.fire);
    controls_init(&state);
    (void)step(CONTROLS_TITLE, 0, false, 0, 0, false);
    (void)step(CONTROLS_TITLE, 10, false, 0, 0, true);
    assert(step(CONTROLS_TITLE, 35, false, 0, 0, true).input.start);
    assert(!step(CONTROLS_TITLE, 50, false, 0, 0, true).input.start);
    assert(!step(CONTROLS_PLAY, 60, false, 0, 0, true).input.fire);
    (void)step(CONTROLS_PLAY, 80, false, 0, 0, false);
    (void)step(CONTROLS_PLAY, 105, false, 0, 0, false);
    (void)step(CONTROLS_PLAY, 120, false, 0, 0, true);
    assert(step(CONTROLS_PLAY, 145, false, 0, 0, true).input.fire);
}
static void drag_relative_wrap_deadzone_and_teleport(void) {
    init();
    controls_set_mode(&state, CONTROLS_RELATIVE_DRAG);
    assert(step(CONTROLS_PLAY, 10, true, 220, 120, false).input.angle_delta == 0);
    int delta = step(CONTROLS_PLAY, 20, true, 210, 160, false).input.angle_delta;
    assert(delta > 4000 && delta < 5000);
    assert(step(CONTROLS_PLAY, 30, true, 220, 120, false).input.angle_delta < 0);
    (void)step(CONTROLS_PLAY, 40, false, 0, 0, false);
    (void)step(CONTROLS_PLAY, 50, true, 220, 110, false);
    delta = step(CONTROLS_PLAY, 60, true, 220, 130, false).input.angle_delta;
    assert(delta > 1500 && delta < 2500); /* Wrap through 65535/0. */
    assert(step(CONTROLS_PLAY, 70, true, 120, 120, false).input.angle_delta == 0);
    assert(step(CONTROLS_PLAY, 80, true, 20, 120, false).input.angle_delta == 0);
    assert(step(CONTROLS_PLAY, 90, true, 220, 120, false).input.angle_delta == 0); /* Teleport. */
    assert(step(CONTROLS_PLAY, 100, true, 210, 160, false).input.angle_delta > 0);
    assert(step(CONTROLS_PLAY, 110, false, 0, 0, false).input.angle_delta == 0);
    assert(step(CONTROLS_PLAY, 120, true, 40, 170, false).input.angle_delta == 0);
}
static void clock_wrap_and_bad_samples(void) {
    init();
    (void)step(CONTROLS_PAUSE, UINT32_MAX - 1100u, false, 0, 0, false);
    (void)step(CONTROLS_PAUSE, UINT32_MAX - 1000u, true, 120, 193, false);
    assert(step(CONTROLS_PAUSE, 3999, true, 120, 193, false).reboot_requested);
    (void)step(CONTROLS_PLAY, 4100, false, 0, 0, false);
    assert(step(CONTROLS_PLAY, 4200, true, 240, 20, false).input.turn == 0);
    assert(controls_hit_test(CONTROLS_PAUSE, 120, 193) == CONTROLS_TARGET_REBOOT);
    assert(controls_hit_test(CONTROLS_TITLE, 120, 160) == CONTROLS_TARGET_SETTINGS);
    assert(controls_hit_test(CONTROLS_PLAY, 0, 0) == CONTROLS_TARGET_NONE);
}
static void cancellation_and_paused_mode(void) {
    init();
    (void)step(CONTROLS_PLAY, 10, true, 120, 120, false);
    controls_cancel_touch(&state);
    assert(!step(CONTROLS_PLAY, 20, false, 0, 0, false).input.pause);
    (void)step(CONTROLS_PLAY, 30, true, 120, 120, false);
    (void)step(CONTROLS_PLAY, 40, true, 160, 120, false);
    (void)step(CONTROLS_PLAY, 50, true, 120, 120, false);
    assert(!step(CONTROLS_PLAY, 60, false, 0, 0, false).input.pause);
    (void)step(CONTROLS_PAUSE, 100, false, 0, 0, false);
    (void)step(CONTROLS_PAUSE, 110, true, 120, 125, false);
    controls_output out = step(CONTROLS_PAUSE, 150, false, 0, 0, false);
    assert(out.input.menu_action == GAME_MENU_SETTINGS && !out.mode_changed);
    assert(!out.input.quit && !out.input.pause && !out.input.start);
    (void)step(CONTROLS_PAUSE, 200, true, 120, 170, false);
    controls_cancel_touch(&state);
    assert(!step(CONTROLS_PAUSE, 250, false, 0, 0, false).input.quit);
}
static void drag_both_directions_in_every_quadrant(void) {
    static const uint16_t xy[9][2] = {{120, 20}, {190, 50}, {220, 120}, {190, 190}, {120, 220},
                                      {50, 190}, {20, 120}, {50, 50},   {120, 20}};
    init();
    controls_set_mode(&state, CONTROLS_RELATIVE_DRAG);
    (void)step(CONTROLS_PLAY, 10, true, xy[0][0], xy[0][1], false);
    for (unsigned i = 1; i < 9u; ++i) {
        controls_output o = step(CONTROLS_PLAY, 10u + i * 10u, true, xy[i][0], xy[i][1], false);
        assert(o.input.angle_delta == 8192 && o.input.turn == 0);
    }
    for (unsigned i = 8; i > 0u; --i) {
        controls_output o =
            step(CONTROLS_PLAY, 100u + (9u - i) * 10u, true, xy[i - 1u][0], xy[i - 1u][1], false);
        assert(o.input.angle_delta == -8192 && !o.input.quit && !o.input.pause);
    }
}
static void touch_error_waits_for_a_confirmed_release(void) {
    init();
    (void)step(CONTROLS_PAUSE, 0, false, 0, 0, false);
    (void)step(CONTROLS_PAUSE, 10, true, 120, 170, false);
    controls_cancel_touch(&state);
    /* Firmware's synthetic released sample after an I2C error is not release. */
    assert(!step(CONTROLS_PAUSE, 20, false, 0, 0, false).input.quit);
    assert(!step(CONTROLS_PAUSE, 30, true, 120, 170, false).input.quit);
    assert(!step(CONTROLS_PAUSE, 40, false, 0, 0, false).input.quit);
    (void)step(CONTROLS_PAUSE, 50, true, 120, 170, false);
    assert(step(CONTROLS_PAUSE, 70, false, 0, 0, false).input.quit);
    (void)step(CONTROLS_PAUSE, 80, true, 120, 170, false);
    controls_cancel_touch(&state);
    controls_sample invalid = {.now_ms = 90, .touch_invalid = true};
    assert(!controls_update(&state, invalid, CONTROLS_PAUSE).input.quit);
    assert(!step(CONTROLS_PAUSE, 100, true, 120, 170, false).input.quit);
    assert(!step(CONTROLS_PAUSE, 110, false, 0, 0, false).input.quit);
    (void)step(CONTROLS_PAUSE, 120, true, 120, 170, false);
    assert(step(CONTROLS_PAUSE, 140, false, 0, 0, false).input.quit);
}
static controls_output motion_tick(uint32_t ms, int32_t ax, int32_t az, int32_t gy, bool valid) {
    controls_sample input = {
        .now_ms = ms,
        .motion_valid = valid,
        .motion = {.accel_mg = {ax, 0, az}, .gyro_mdps = {0, gy, 0}, .sampled_ms = ms}};
    return controls_update(&state, input, CONTROLS_PLAY);
}
static void tilt_calibration_steering_and_stale_stop(void) {
    init();
    controls_set_mode(&state, CONTROLS_TILT);
    for (uint32_t ms = 100; ms < 600; ms += 20) {
        assert(motion_tick(ms, 0, 1000, 0, true).input.angle_delta == 0);
        assert(!controls_tilt_ready(&state));
    }
    (void)motion_tick(600, 0, 1000, 0, true);
    assert(controls_tilt_ready(&state));
    assert(controls_tilt_progress(&state, 600) == 1000);
    /* Noise within roughly one degree must not drift the ship. */
    for (uint32_t ms = 620; ms < 900; ms += 20)
        assert(motion_tick(ms, 15, 1000, 0, true).input.angle_delta == 0);
    int delta = 0;
    for (uint32_t ms = 900; ms < 1900; ms += 20)
        delta = motion_tick(ms, 423, 906, 0, true).input.angle_delta;
    assert(delta > 500 && delta <= 900);
    for (uint32_t ms = 1900; ms < 2900; ms += 20)
        delta = motion_tick(ms, -423, 906, 0, true).input.angle_delta;
    assert(delta < -500 && delta >= -900);
    assert(motion_tick(3010, 0, 0, 0, false).input.angle_delta == 0);
    assert(!controls_tilt_ready(&state));
    /* Reconnection calibrates again rather than spinning at the last angle. */
    assert(motion_tick(3030, 423, 906, 0, true).input.angle_delta == 0);
    assert(!controls_tilt_ready(&state));
}
static void tilt_calibration_requires_stationary_samples(void) {
    init();
    controls_set_mode(&state, CONTROLS_TILT);
    for (uint32_t ms = 0; ms < 1000; ms += 20)
        (void)motion_tick(ms, 0, 1000, 30000, true);
    assert(!controls_tilt_ready(&state));
    for (uint32_t ms = 1000; ms <= 1500; ms += 20)
        (void)motion_tick(ms, 0, 1000, 2000, true);
    assert(controls_tilt_ready(&state));
    /* Learned gyro bias must not move a stationary board. */
    for (uint32_t ms = 1520; ms < 2000; ms += 20)
        assert(motion_tick(ms, 0, 1000, 2000, true).input.angle_delta == 0);
    (void)step(CONTROLS_PAUSE, 2010, false, 0, 0, false);
    assert(motion_tick(2030, 0, 1000, 2000, true).input.angle_delta == 0);
    assert(controls_tilt_ready(&state));
    controls_tilt_reset(&state);
    assert(!controls_tilt_ready(&state));
    controls_set_mode(&state, CONTROLS_SPLIT_HOLD);
    assert(motion_tick(2100, 423, 906, 0, true).input.angle_delta == 0);
}
static void three_modes_cycle_through_menu(void) {
    controls_init(&state);
    (void)step(CONTROLS_SETTINGS, 0, false, 0, 0, false);
    for (unsigned i = 0; i < 3u; ++i) {
        (void)step(CONTROLS_SETTINGS, 10u + i * 100u, true, 120, 64, false);
        assert(step(CONTROLS_SETTINGS, 30u + i * 100u, false, 0, 0, false).mode_changed);
        assert(state.mode == (controls_mode)((i + 1u) % 3u));
    }
}
static void tilt_uses_gyro_and_handles_wrap(void) {
    init();
    controls_set_mode(&state, CONTROLS_TILT);
    for (uint32_t ms = 0; ms <= 500; ms += 20)
        (void)motion_tick(ms, 0, 1000, 0, true);
    assert(controls_tilt_ready(&state));
    int delta = 0;
    /* Acceleration still flat, but gyro predicts the changing bank immediately. */
    for (uint32_t ms = 520; ms <= 640; ms += 20)
        delta = motion_tick(ms, 0, 1000, -90000, true).input.angle_delta;
    assert(delta > 0);
    for (uint32_t ms = 660; ms <= 2500; ms += 20)
        delta = motion_tick(ms, 0, 1000, 0, true).input.angle_delta;
    assert(delta == 0); /* Gravity correction removes gyro integration drift. */
    init();
    controls_set_mode(&state, CONTROLS_TILT);
    uint32_t began = UINT32_MAX - 249u;
    for (uint32_t elapsed = 0; elapsed <= 500; elapsed += 20)
        (void)motion_tick(began + elapsed, 15, -1000, 0, true);
    assert(controls_tilt_ready(&state));
    /* Neutral straddles atan2's +/-180-degree wrap: no artificial full turn. */
    for (uint32_t elapsed = 520; elapsed < 1000; elapsed += 20)
        assert(motion_tick(began + elapsed, -15, -1000, 0, true).input.angle_delta == 0);
    for (uint32_t elapsed = 1000; elapsed < 2400; elapsed += 20) {
        delta = motion_tick(began + elapsed, 1000, 0, 0, true).input.angle_delta;
        assert(delta >= -900 && delta <= 900);
    }
    assert(delta == 900);
    assert(motion_tick(began + 2400, 5000, 0, 0, true).input.angle_delta == 0);
    assert(!controls_tilt_ready(&state));
}
static void touch_error_does_not_rearm_reboot(void) {
    init();
    (void)step(CONTROLS_PAUSE, 0, false, 0, 0, false);
    (void)step(CONTROLS_PAUSE, 100, true, 120, 193, false);
    controls_cancel_touch(&state);
    controls_sample invalid = {.now_ms = 4100, .touch_invalid = true};
    assert(!controls_update(&state, invalid, CONTROLS_PAUSE).reboot_requested);
    assert(!step(CONTROLS_PAUSE, 4200, true, 120, 193, false).reboot_requested);
    assert(!step(CONTROLS_PAUSE, 9300, true, 120, 193, false).reboot_requested);
    assert(controls_reboot_progress(&state, 9300) == 0);
    (void)step(CONTROLS_PAUSE, 9400, false, 0, 0, false);
    (void)step(CONTROLS_PAUSE, 9500, true, 120, 193, false);
    assert(step(CONTROLS_PAUSE, 14500, true, 120, 193, false).reboot_requested);
}
static void settings_actions_and_half_open_bounds(void) {
    static const struct {
        unsigned x0, x1, y0, y1;
        controls_target target;
        game_menu_action action;
    } items[] = {{42, 80, 94, 120, CONTROLS_TARGET_FIRE_DOWN, GAME_MENU_FIRE_DOWN},
                 {160, 198, 94, 120, CONTROLS_TARGET_FIRE_UP, GAME_MENU_FIRE_UP},
                 {42, 80, 137, 163, CONTROLS_TARGET_ENEMY_DOWN, GAME_MENU_ENEMY_DOWN},
                 {160, 198, 137, 163, CONTROLS_TARGET_ENEMY_UP, GAME_MENU_ENEMY_UP},
                 {42, 80, 180, 206, CONTROLS_TARGET_TILT_DOWN, GAME_MENU_TILT_DOWN},
                 {160, 198, 180, 206, CONTROLS_TARGET_TILT_UP, GAME_MENU_TILT_UP},
                 {78, 162, 209, 231, CONTROLS_TARGET_BACK, GAME_MENU_BACK}};
    controls_init(&state);
    (void)step(CONTROLS_PAUSE, 0, false, 0, 0, false);
    assert(step(CONTROLS_PAUSE, 10, true, 120, 125, false).input.menu_action == GAME_MENU_NONE);
    assert(step(CONTROLS_PAUSE, 30, false, 0, 0, false).input.menu_action == GAME_MENU_SETTINGS);
    for (unsigned i = 0; i < sizeof items / sizeof items[0]; ++i) {
        assert(controls_hit_test(CONTROLS_SETTINGS, (uint16_t)items[i].x0, (uint16_t)items[i].y0) ==
               items[i].target);
        assert(controls_hit_test(CONTROLS_SETTINGS, (uint16_t)(items[i].x1 - 1u),
                                 (uint16_t)(items[i].y1 - 1u)) == items[i].target);
        assert(controls_hit_test(CONTROLS_SETTINGS, (uint16_t)(items[i].x0 - 1u),
                                 (uint16_t)items[i].y0) == CONTROLS_TARGET_NONE);
        assert(controls_hit_test(CONTROLS_SETTINGS, (uint16_t)items[i].x1, (uint16_t)items[i].y0) ==
               CONTROLS_TARGET_NONE);
        assert(controls_hit_test(CONTROLS_SETTINGS, (uint16_t)items[i].x0,
                                 (uint16_t)(items[i].y0 - 1u)) == CONTROLS_TARGET_NONE);
        assert(controls_hit_test(CONTROLS_SETTINGS, (uint16_t)items[i].x0, (uint16_t)items[i].y1) ==
               CONTROLS_TARGET_NONE);
        (void)step(CONTROLS_SETTINGS, 100u + i * 100u, false, 0, 0, false);
        controls_output down = step(CONTROLS_SETTINGS, 110u + i * 100u, true, items[i].x0 + 4u,
                                    items[i].y0 + 4u, false);
        assert(down.input.menu_action == GAME_MENU_NONE);
        controls_output released = step(CONTROLS_SETTINGS, 130u + i * 100u, false, 0, 0, false);
        assert(released.input.menu_action == items[i].action);
        assert(!released.input.pause && !released.input.start && !released.input.quit &&
               !released.reboot_requested && !released.mode_changed);
        assert(step(CONTROLS_SETTINGS, 140u + i * 100u, false, 0, 0, false).input.menu_action ==
               GAME_MENU_NONE);
    }
    assert(controls_hit_test(CONTROLS_SETTINGS, 120, 120) == CONTROLS_TARGET_NONE);
    assert(controls_hit_test(CONTROLS_SETTINGS, 240, 90) == CONTROLS_TARGET_NONE);
}
static void settings_cancel_hold_and_boot_resume(void) {
    controls_init(&state);
    (void)step(CONTROLS_SETTINGS, 0, false, 0, 0, false);
    (void)step(CONTROLS_SETTINGS, 10, true, 180, 106, false);
    assert(step(CONTROLS_SETTINGS, 400, true, 180, 106, false).input.menu_action == GAME_MENU_NONE);
    assert(step(CONTROLS_SETTINGS, 800, true, 180, 106, false).input.menu_action == GAME_MENU_NONE);
    assert(step(CONTROLS_SETTINGS, 810, false, 0, 0, false).input.menu_action == GAME_MENU_NONE);
    (void)step(CONTROLS_SETTINGS, 900, true, 180, 106, false);
    (void)step(CONTROLS_SETTINGS, 910, true, 150, 106, false);
    (void)step(CONTROLS_SETTINGS, 920, true, 180, 106, false);
    assert(step(CONTROLS_SETTINGS, 930, false, 0, 0, false).input.menu_action == GAME_MENU_NONE);
    (void)step(CONTROLS_SETTINGS, 1000, true, 180, 106, false);
    controls_cancel_touch(&state);
    controls_sample invalid = {.now_ms = 1010, .touch_invalid = true};
    assert(controls_update(&state, invalid, CONTROLS_SETTINGS).input.menu_action == GAME_MENU_NONE);
    (void)step(CONTROLS_SETTINGS, 1020, true, 180, 106, false);
    assert(step(CONTROLS_SETTINGS, 1030, false, 0, 0, false).input.menu_action == GAME_MENU_NONE);
    (void)step(CONTROLS_SETTINGS, 1040, true, 180, 106, false);
    assert(step(CONTROLS_SETTINGS, 1050, false, 0, 0, false).input.menu_action ==
           GAME_MENU_FIRE_UP);
    /* Touch held across entering Settings is consumed until released. */
    (void)step(CONTROLS_PAUSE, 1100, true, 180, 106, false);
    (void)step(CONTROLS_SETTINGS, 1110, true, 180, 106, false);
    assert(step(CONTROLS_SETTINGS, 1120, false, 0, 0, false).input.menu_action == GAME_MENU_NONE);
    (void)step(CONTROLS_SETTINGS, 1200, false, 0, 0, true);
    controls_output boot = step(CONTROLS_SETTINGS, 1225, false, 0, 0, true);
    assert(!boot.input.pause && !boot.input.start && boot.input.menu_action == GAME_MENU_BACK);
    assert(!step(CONTROLS_SETTINGS, 1250, false, 0, 0, true).input.pause);
}
static int sensitivity_output(uint16_t sensitivity, int32_t filtered, uint32_t now_ms) {
    init();
    controls_set_mode(&state, CONTROLS_TILT);
    state.tilt_ready = true;
    state.tilt_available = true;
    state.tilt_have_sample = true;
    state.tilt_last_ms = 1000;
    state.tilt_filtered = filtered;
    controls_sample input = {.now_ms = now_ms, .tilt_sensitivity_percent = sensitivity};
    return controls_update(&state, input, CONTROLS_PLAY).input.angle_delta;
}
static void tilt_sensitivity_default_limits_deadzone_and_stale(void) {
    assert(sensitivity_output(0, 2550, 1000) == 450);
    assert(sensitivity_output(100, 2550, 1000) == 450);
    assert(sensitivity_output(50, 2550, 1000) == 225);
    assert(sensitivity_output(200, 2550, 1000) == 900);
    assert(sensitivity_output(1, 2550, 1000) == 225);
    assert(sensitivity_output(UINT16_MAX, 2550, 1000) == 900);
    assert(sensitivity_output(200, -2550, 1000) == -900);
    assert(sensitivity_output(200, 546, 1000) == 0);
    assert(sensitivity_output(200, -546, 1000) == 0);
    assert(sensitivity_output(200, 30000, 1000) == 900);
    assert(sensitivity_output(50, 8556, 1000) == 900); /* Gain applied before cap. */
    assert(sensitivity_output(200, 2550, 1101) == 0);
}
static controls_output pose_tick(controls_screen screen, uint32_t ms, int32_t x, int32_t y,
                                 int32_t z, int32_t gx, int32_t gy, int32_t gz) {
    controls_sample input = {
        .now_ms = ms,
        .motion_valid = true,
        .motion = {.accel_mg = {x, y, z}, .gyro_mdps = {gx, gy, gz}, .sampled_ms = ms}};
    return controls_update(&state, input, screen);
}
static void tilt_calibrates_upright_and_all_comfortable_poses(void) {
    static const int32_t poses[][3] = {
        {0, 1000, 0}, {0, -1000, 0}, {0, 0, 1000}, {0, 0, -1000}, {0, 500, -866}};
    for (unsigned p = 0; p < sizeof poses / sizeof poses[0]; ++p) {
        init();
        controls_set_mode(&state, CONTROLS_TILT);
        for (uint32_t ms = 0; ms <= 600; ms += 20)
            (void)pose_tick(CONTROLS_PLAY, ms, poses[p][0], poses[p][1], poses[p][2], -2671, -2250,
                            15);
        assert(controls_tilt_ready(&state));
        int delta = 0;
        for (uint32_t ms = 620; ms <= 1620; ms += 20)
            delta = pose_tick(CONTROLS_PLAY, ms, 423, poses[p][1] * 906 / 1000,
                              poses[p][2] * 906 / 1000, -2671, -2250, 15)
                        .input.angle_delta;
        assert(delta > 500 && delta <= 900);
        for (uint32_t ms = 1640; ms <= 2640; ms += 20)
            delta = pose_tick(CONTROLS_PLAY, ms, -423, poses[p][1] * 906 / 1000,
                              poses[p][2] * 906 / 1000, -2671, -2250, 15)
                        .input.angle_delta;
        assert(delta < -500 && delta >= -900);
    }
}
static void settings_tilt_preview_and_fresh_context_preservation(void) {
    init();
    controls_set_mode(&state, CONTROLS_TILT);
    for (uint32_t ms = 0; ms <= 600; ms += 20) {
        assert(pose_tick(CONTROLS_SETTINGS, ms, 0, 1000, 0, 0, 0, 0).input.angle_delta == 0);
        if (ms == 240u)
            assert(controls_tilt_progress(&state, ms) == 480);
    }
    assert(controls_tilt_ready(&state));
    assert(controls_tilt_preview(&state) == 0);
    for (uint32_t ms = 620; ms <= 1620; ms += 20)
        assert(pose_tick(CONTROLS_SETTINGS, ms, 423, 906, 0, 0, 0, 0).input.angle_delta == 0);
    assert(controls_tilt_preview(&state) > 500 && controls_tilt_preview(&state) <= 1000);
    assert(pose_tick(CONTROLS_TITLE, 1640, 423, 906, 0, 0, 0, 0).input.angle_delta == 0);
    assert(pose_tick(CONTROLS_GAME_OVER, 1660, 423, 906, 0, 0, 0, 0).input.angle_delta == 0);
    assert(pose_tick(CONTROLS_PAUSE, 1680, 423, 906, 0, 0, 0, 0).input.angle_delta == 0);
    assert(controls_tilt_ready(&state));
    assert(pose_tick(CONTROLS_PLAY, 1700, 423, 906, 0, 0, 0, 0).input.angle_delta > 500);
    assert(controls_tilt_preview(&state) > 500);
    controls_sample stale = {.now_ms = 1801};
    assert(controls_update(&state, stale, CONTROLS_SETTINGS).input.angle_delta == 0);
    assert(!controls_tilt_ready(&state) && controls_tilt_preview(&state) == 0);
    assert(controls_tilt_progress(&state, 1801) == 0);
    assert(pose_tick(CONTROLS_PLAY, 1820, 423, 906, 0, 0, 0, 0).input.angle_delta == 0);
    assert(!controls_tilt_ready(&state));
}
static void learned_gyro_axis_predicts_bank_in_all_poses(void) {
    static const int32_t cases[][6] = {{0, 0, 1000, 0, -90000, 0},
                                       {0, 0, -1000, 0, 90000, 0},
                                       {0, 1000, 0, 0, 0, 90000},
                                       {0, -1000, 0, 0, 0, -90000},
                                       {0, 500, -866, 0, 77940, 45000}};
    for (unsigned c = 0; c < sizeof cases / sizeof cases[0]; ++c) {
        const int32_t *v = cases[c];
        init();
        controls_set_mode(&state, CONTROLS_TILT);
        for (uint32_t ms = 0; ms <= 600; ms += 20)
            (void)pose_tick(CONTROLS_PLAY, ms, v[0], v[1], v[2], 1234, -2250, 31);
        assert(controls_tilt_ready(&state));
        int delta = 0;
        for (uint32_t ms = 620; ms <= 740; ms += 20)
            delta =
                pose_tick(CONTROLS_PLAY, ms, v[0], v[1], v[2], 1234 + v[3], -2250 + v[4], 31 + v[5])
                    .input.angle_delta;
        assert(delta > 0);
        for (uint32_t ms = 760; ms <= 2800; ms += 20)
            delta =
                pose_tick(CONTROLS_PLAY, ms, v[0], v[1], v[2], 1234, -2250, 31).input.angle_delta;
        assert(delta == 0);
        for (uint32_t ms = 2820; ms <= 2940; ms += 20)
            delta =
                pose_tick(CONTROLS_PLAY, ms, v[0], v[1], v[2], 1234 - v[3], -2250 - v[4], 31 - v[5])
                    .input.angle_delta;
        assert(delta < 0);
    }
}
static void title_over_settings_and_mode_only_in_settings(void) {
    static const controls_screen screens[] = {CONTROLS_TITLE, CONTROLS_GAME_OVER, CONTROLS_PAUSE};
    for (unsigned i = 0; i < 3u; ++i) {
        controls_init(&state);
        unsigned y = screens[i] == CONTROLS_PAUSE ? 125u : 160u;
        (void)step(screens[i], 0, false, 0, 0, false);
        (void)step(screens[i], 10, true, 120, y, false);
        controls_output out = step(screens[i], 30, false, 0, 0, false);
        assert(out.input.menu_action == GAME_MENU_SETTINGS && !out.mode_changed);
        assert(state.mode == CONTROLS_SPLIT_HOLD);
    }
    assert(controls_hit_test(CONTROLS_SETTINGS, 48, 52) == CONTROLS_TARGET_MODE);
    assert(controls_hit_test(CONTROLS_SETTINGS, 191, 75) == CONTROLS_TARGET_MODE);
    assert(controls_hit_test(CONTROLS_SETTINGS, 48, 76) == CONTROLS_TARGET_NONE);
    assert(controls_hit_test(CONTROLS_PAUSE, 120, 110) == CONTROLS_TARGET_NONE);
}
static void degenerate_bank_projection_stops_then_recalibrates(void) {
    init();
    controls_set_mode(&state, CONTROLS_TILT);
    for (uint32_t ms = 0; ms <= 600; ms += 20)
        (void)pose_tick(CONTROLS_PLAY, ms, 0, 1000, 0, 0, 0, 0);
    assert(controls_tilt_ready(&state));
    /* This large pitch change is perpendicular to the learned bank plane. */
    assert(pose_tick(CONTROLS_PLAY, 620, 0, 0, 1000, 0, 0, 0).input.angle_delta == 0);
    assert(!controls_tilt_ready(&state));
    for (uint32_t ms = 640; ms <= 1240; ms += 20)
        (void)pose_tick(CONTROLS_PLAY, ms, 0, 0, 1000, 0, 0, 0);
    assert(controls_tilt_ready(&state));
    assert(pose_tick(CONTROLS_PLAY, 1260, 0, 0, 0, 0, 0, 0).input.angle_delta == 0);
    assert(!controls_tilt_ready(&state));
    /* A side-standing neutral also has a deterministic safe fallback basis. */
    for (uint32_t ms = 1280; ms <= 1880; ms += 20)
        (void)pose_tick(CONTROLS_PLAY, ms, 1000, 0, 0, 0, 0, 0);
    assert(controls_tilt_ready(&state));
}
int main(void) {
    degenerate_bank_projection_stops_then_recalibrates();
    settings_tilt_preview_and_fresh_context_preservation();
    learned_gyro_axis_predicts_bank_in_all_poses();
    title_over_settings_and_mode_only_in_settings();
    tilt_calibrates_upright_and_all_comfortable_poses();
    settings_actions_and_half_open_bounds();
    settings_cancel_hold_and_boot_resume();
    tilt_sensitivity_default_limits_deadzone_and_stale();
    split_hold_and_release();
    menu_actions_and_transition_consumption();
    quit_tap_reboot_hold_and_cancel();
    physical_fire_debounce_and_context();
    drag_relative_wrap_deadzone_and_teleport();
    clock_wrap_and_bad_samples();
    cancellation_and_paused_mode();
    drag_both_directions_in_every_quadrant();
    touch_error_waits_for_a_confirmed_release();
    tilt_calibration_steering_and_stale_stop();
    tilt_calibration_requires_stationary_samples();
    three_modes_cycle_through_menu();
    tilt_uses_gyro_and_handles_wrap();
    touch_error_does_not_rearm_reboot();
    puts("controls: all behavior tests passed");
    return 0;
}
