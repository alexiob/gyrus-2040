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
    assert(step(CONTROLS_PLAY, 17, true, 30, 120, false).input.turn == -1);
    assert(step(CONTROLS_PLAY, 34, true, 210, 120, false).input.turn == 1);
    assert(step(CONTROLS_PLAY, 51, true, 120, 45, false).input.turn == 0);
    assert(step(CONTROLS_PLAY, 68, false, 0, 0, false).input.turn == 0);
    assert(step(CONTROLS_PLAY, 85, true, 120, 120, false).input.turn == 0);
    assert(step(CONTROLS_PLAY, 102, false, 0, 0, false).input.pause);
}
static void menu_actions_and_transition_consumption(void) {
    controls_init(&state);
    (void)step(CONTROLS_TITLE, 0, false, 0, 0, false);
    (void)step(CONTROLS_TITLE, 17, true, 120, 160, false);
    controls_output o = step(CONTROLS_TITLE, 50, false, 0, 0, false);
    assert(o.mode_changed && state.mode == CONTROLS_RELATIVE_DRAG);
    assert(strcmp(controls_mode_name(state.mode), "BORDER DRAG") == 0);
    (void)step(CONTROLS_TITLE, 100, true, 120, 110, false);
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
    (void)step(CONTROLS_PAUSE, 100, true, 120, 157, false);
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
    assert(controls_hit_test(CONTROLS_TITLE, 120, 160) == CONTROLS_TARGET_MODE);
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
    assert(out.mode_changed && state.mode == CONTROLS_RELATIVE_DRAG);
    assert(!out.input.quit && !out.input.pause && !out.input.start);
    (void)step(CONTROLS_PAUSE, 200, true, 120, 157, false);
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
    (void)step(CONTROLS_PAUSE, 10, true, 120, 157, false);
    controls_cancel_touch(&state);
    /* Firmware's synthetic released sample after an I2C error is not release. */
    assert(!step(CONTROLS_PAUSE, 20, false, 0, 0, false).input.quit);
    assert(!step(CONTROLS_PAUSE, 30, true, 120, 157, false).input.quit);
    assert(!step(CONTROLS_PAUSE, 40, false, 0, 0, false).input.quit);
    (void)step(CONTROLS_PAUSE, 50, true, 120, 157, false);
    assert(step(CONTROLS_PAUSE, 70, false, 0, 0, false).input.quit);
    (void)step(CONTROLS_PAUSE, 80, true, 120, 157, false);
    controls_cancel_touch(&state);
    controls_sample invalid = {.now_ms = 90, .touch_invalid = true};
    assert(!controls_update(&state, invalid, CONTROLS_PAUSE).input.quit);
    assert(!step(CONTROLS_PAUSE, 100, true, 120, 157, false).input.quit);
    assert(!step(CONTROLS_PAUSE, 110, false, 0, 0, false).input.quit);
    (void)step(CONTROLS_PAUSE, 120, true, 120, 157, false);
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
    assert(!controls_tilt_ready(&state));
    controls_tilt_reset(&state);
    assert(!controls_tilt_ready(&state));
    controls_set_mode(&state, CONTROLS_SPLIT_HOLD);
    assert(motion_tick(2100, 423, 906, 0, true).input.angle_delta == 0);
}
static void three_modes_cycle_through_menu(void) {
    controls_init(&state);
    (void)step(CONTROLS_TITLE, 0, false, 0, 0, false);
    for (unsigned i = 0; i < 3u; ++i) {
        (void)step(CONTROLS_TITLE, 10u + i * 100u, true, 120, 160, false);
        assert(step(CONTROLS_TITLE, 30u + i * 100u, false, 0, 0, false).mode_changed);
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
    assert(delta == -900);
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
int main(void) {
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
