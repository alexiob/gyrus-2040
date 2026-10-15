#include "gyrus/controls.h"
#include <stddef.h>
#include <string.h>

static bool in_rect(uint16_t x, uint16_t y, unsigned x0, unsigned x1, unsigned y0, unsigned y1) {
    return x >= x0 && x < x1 && y >= y0 && y < y1;
}
static int32_t radius_squared(uint16_t x, uint16_t y) {
    int32_t dx = (int32_t)x - CONTROLS_CENTER_X, dy = (int32_t)y - CONTROLS_CENTER_Y;
    return dx * dx + dy * dy;
}
controls_target controls_hit_test(controls_screen screen, uint16_t x, uint16_t y) {
    if (x >= 240u || y >= 240u)
        return CONTROLS_TARGET_NONE;
    if (screen == CONTROLS_TITLE || screen == CONTROLS_GAME_OVER) {
        if (in_rect(x, y, CONTROLS_START_X0, CONTROLS_START_X1, CONTROLS_START_Y0,
                    CONTROLS_START_Y1))
            return CONTROLS_TARGET_START;
        if (in_rect(x, y, CONTROLS_MODE_X0, CONTROLS_MODE_X1, CONTROLS_MODE_Y0, CONTROLS_MODE_Y1))
            return CONTROLS_TARGET_MODE;
    } else if (screen == CONTROLS_PAUSE) {
        if (in_rect(x, y, CONTROLS_RESUME_X0, CONTROLS_RESUME_X1, CONTROLS_RESUME_Y0,
                    CONTROLS_RESUME_Y1))
            return CONTROLS_TARGET_PAUSE;
        if (in_rect(x, y, CONTROLS_PAUSE_MODE_X0, CONTROLS_PAUSE_MODE_X1, CONTROLS_PAUSE_MODE_Y0,
                    CONTROLS_PAUSE_MODE_Y1))
            return CONTROLS_TARGET_MODE;
        if (in_rect(x, y, CONTROLS_QUIT_X0, CONTROLS_QUIT_X1, CONTROLS_QUIT_Y0, CONTROLS_QUIT_Y1))
            return CONTROLS_TARGET_QUIT;
        if (in_rect(x, y, CONTROLS_REBOOT_X0, CONTROLS_REBOOT_X1, CONTROLS_REBOOT_Y0,
                    CONTROLS_REBOOT_Y1))
            return CONTROLS_TARGET_REBOOT;
    } else if (radius_squared(x, y) <= CONTROLS_CENTER_RADIUS * CONTROLS_CENTER_RADIUS) {
        return CONTROLS_TARGET_PAUSE;
    }
    return CONTROLS_TARGET_NONE;
}
void controls_init(controls_state *state) {
    if (state != NULL)
        memset(state, 0, sizeof *state);
}
void controls_set_mode(controls_state *state, controls_mode mode) {
    if (state == NULL)
        return;
    state->mode = (unsigned)mode < (unsigned)CONTROLS_MODE_COUNT ? mode : CONTROLS_SPLIT_HOLD;
    controls_tilt_reset(state);
    state->drag_valid = false;
    if (state->touch_down)
        state->touch_cancelled = true;
}
void controls_cancel_touch(controls_state *state) {
    if (state == NULL)
        return;
    state->touch_cancelled = true;
    state->touch_wait_release = true;
    state->touch_ignore_sample = true;
    state->drag_valid = false;
}
const char *controls_mode_name(controls_mode mode) {
    if (mode == CONTROLS_TILT)
        return "TILT";
    return mode == CONTROLS_RELATIVE_DRAG ? "BORDER DRAG" : "LEFT / RIGHT";
}
/* atan(i/32), in 65536 units/turn. Five-bit linear interpolation.
 * The ratio is rounded to 1/1024; angle error stays below 8 units. */
static const uint16_t atan_table[33] = {0,    326,  651,  975,  1297, 1617, 1933, 2246, 2555,
                                        2860, 3159, 3453, 3742, 4025, 4302, 4572, 4836, 5094,
                                        5344, 5589, 5826, 6058, 6282, 6500, 6712, 6917, 7117,
                                        7310, 7498, 7679, 7856, 8026, 8192};
static uint16_t atan_ratio(unsigned minor, unsigned major) {
    unsigned q = (minor * 1024u + major / 2u) / major;
    if (q >= 1024u)
        return 8192u;
    unsigned index = q >> 5u, fraction = q & 31u;
    return (uint16_t)(atan_table[index] +
                      ((unsigned)(atan_table[index + 1u] - atan_table[index]) * fraction + 16u) /
                          32u);
}
static uint16_t touch_angle(uint16_t x, uint16_t y) {
    int dx = (int)x - CONTROLS_CENTER_X, dy = (int)y - CONTROLS_CENTER_Y;
    unsigned ax = (unsigned)(dx < 0 ? -dx : dx), ay = (unsigned)(dy < 0 ? -dy : dy);
    uint16_t angle = ax >= ay ? atan_ratio(ay, ax) : (uint16_t)(16384u - atan_ratio(ax, ay));
    uint32_t full;
    if (dx >= 0)
        full = dy >= 0 ? angle : 65536u - angle;
    else
        full = dy >= 0 ? 32768u - angle : 32768u + angle;
    return (uint16_t)full;
}
static int32_t angle_difference(uint16_t a, uint16_t b) {
    int32_t difference = (int32_t)a - (int32_t)b;
    if (difference > 32767)
        difference -= 65536;
    else if (difference < -32768)
        difference += 65536;
    return difference;
}
static uint16_t gravity_angle(int32_t x, int32_t z) {
    unsigned ax = (unsigned)(x < 0 ? -x : x), az = (unsigned)(z < 0 ? -z : z);
    uint16_t angle = az >= ax ? atan_ratio(ax, az) : (uint16_t)(16384u - atan_ratio(az, ax));
    if (z >= 0)
        return (uint16_t)(x >= 0 ? angle : 65536u - angle);
    return (uint16_t)(x >= 0 ? 32768u - angle : 32768u + angle);
}
void controls_tilt_reset(controls_state *state) {
    if (state == NULL)
        return;
    state->tilt_ready = false;
    state->tilt_available = false;
    state->tilt_have_sample = false;
    state->tilt_calibrating = false;
    state->tilt_calibration_count = 0;
    state->tilt_angle_sum = 0;
    state->tilt_gyro_sum = 0;
    state->tilt_filtered = 0;
}
bool controls_tilt_ready(const controls_state *state) {
    return state != NULL && state->mode == CONTROLS_TILT && state->tilt_ready &&
           state->tilt_available;
}
uint16_t controls_tilt_progress(const controls_state *state, uint32_t now_ms) {
    if (state == NULL || state->mode != CONTROLS_TILT || !state->tilt_available)
        return 0;
    if (state->tilt_ready)
        return 1000;
    if (!state->tilt_calibrating)
        return 0;
    uint32_t elapsed = now_ms - state->tilt_calibration_started;
    return elapsed >= CONTROLS_TILT_CALIBRATE_MS ? 1000u : (uint16_t)(elapsed * 2u);
}
static int16_t tilt_input(controls_state *state, const controls_sample *sample) {
    if (state->tilt_have_sample && sample->now_ms - state->tilt_last_ms > CONTROLS_MOTION_STALE_MS)
        controls_tilt_reset(state);
    const controls_motion *m = &sample->motion;
    bool fresh = sample->motion_valid &&
                 sample->now_ms - m->sampled_ms <= CONTROLS_MOTION_STALE_MS &&
                 (!state->tilt_have_sample || m->sampled_ms != state->tilt_last_ms);
    if (fresh) {
        if (state->tilt_have_sample &&
            m->sampled_ms - state->tilt_last_ms > CONTROLS_MOTION_STALE_MS)
            controls_tilt_reset(state);
        for (unsigned i = 0; i < 3u; ++i) {
            if (m->accel_mg[i] < -4000 || m->accel_mg[i] > 4000 || m->gyro_mdps[i] < -512000 ||
                m->gyro_mdps[i] > 512000) {
                controls_tilt_reset(state);
                return 0;
            }
        }
        int32_t x = m->accel_mg[0], y = m->accel_mg[1], z = m->accel_mg[2];
        int32_t norm = x * x + y * y + z * z;
        /* Reject shocks/freefall and an unusable near-zero X/Z projection. */
        if (norm < 700 * 700 || norm > 1300 * 1300 || x * x + z * z < 250 * 250) {
            controls_tilt_reset(state);
            return 0;
        }
        uint16_t angle = gravity_angle(x, z);
        uint32_t dt = state->tilt_have_sample ? m->sampled_ms - state->tilt_last_ms : 0u;
        state->tilt_have_sample = true;
        state->tilt_available = true;
        state->tilt_last_ms = m->sampled_ms;
        if (!state->tilt_ready) {
            /* QST allows up to10dps initial zero-rate offset. Permit15dps
             * here, then learn the Y bias; the two-degree gravity-pose check
             * independently rejects actual movement during calibration. */
            bool stationary = true;
            for (unsigned i = 0; i < 3u; ++i)
                if (m->gyro_mdps[i] < -15000 || m->gyro_mdps[i] > 15000)
                    stationary = false;
            /* Also detect a slow pose change that a biased gyro might miss. */
            int32_t pose_change =
                state->tilt_calibrating ? angle_difference(angle, state->tilt_reference) : 0;
            if (!stationary || pose_change < -364 || pose_change > 364) {
                state->tilt_calibrating = false;
                state->tilt_calibration_count = 0;
                state->tilt_angle_sum = 0;
                state->tilt_gyro_sum = 0;
                return 0;
            }
            if (!state->tilt_calibrating) {
                state->tilt_calibrating = true;
                state->tilt_calibration_started = m->sampled_ms;
                state->tilt_reference = angle;
                state->tilt_angle_sum = 0;
                state->tilt_gyro_sum = 0;
                state->tilt_calibration_count = 0;
            }
            if (state->tilt_calibration_count < 128u) {
                state->tilt_angle_sum += angle_difference(angle, state->tilt_reference);
                state->tilt_gyro_sum += m->gyro_mdps[1];
                ++state->tilt_calibration_count;
            }
            if (m->sampled_ms - state->tilt_calibration_started >= CONTROLS_TILT_CALIBRATE_MS &&
                state->tilt_calibration_count >= 20u) {
                state->tilt_neutral =
                    (uint16_t)((int32_t)state->tilt_reference +
                               state->tilt_angle_sum / state->tilt_calibration_count);
                state->tilt_angle = state->tilt_neutral;
                state->tilt_gyro_bias = state->tilt_gyro_sum / state->tilt_calibration_count;
                state->tilt_filtered = 0;
                state->tilt_ready = true;
            }
            return 0;
        }
        /* Positive body-Y angular velocity decreases the gravity roll angle.
         * 5493 converts (millidegrees/second * milliseconds) to turn units.
         * Acceleration corrects drift by 1/16 per fresh sample; a 1/4 low-pass
         * on relative bank removes jitter before the proportional speed map. */
        int32_t predicted =
            state->tilt_angle - (m->gyro_mdps[1] - state->tilt_gyro_bias) * (int32_t)dt / 5493;
        state->tilt_angle =
            (uint16_t)(predicted + angle_difference(angle, (uint16_t)predicted) / 16);
        int32_t relative =
            angle_difference((uint16_t)state->tilt_angle, (uint16_t)state->tilt_neutral);
        state->tilt_filtered += (relative - state->tilt_filtered) / 4;
    }
    if (!state->tilt_ready || !state->tilt_available)
        return 0;
    int32_t bank = state->tilt_filtered;
    int32_t amount = bank < 0 ? -bank : bank;
    if (amount <= 546)
        return 0; /* Three degrees around calibrated neutral. */
    int32_t speed = (amount - 546) * 900 / (4551 - 546); /* Full rate near 25 degrees. */
    if (speed > 900)
        speed = 900;
    return (int16_t)(bank < 0 ? -speed : speed);
}
uint16_t controls_reboot_progress(const controls_state *state, uint32_t now_ms) {
    if (state == NULL || !state->touch_down || state->touch_cancelled ||
        state->touch_target != CONTROLS_TARGET_REBOOT)
        return 0;
    uint32_t elapsed = now_ms - state->touch_since_ms;
    return elapsed >= CONTROLS_REBOOT_HOLD_MS ? 1000u : (uint16_t)(elapsed / 5u);
}
controls_output controls_update(controls_state *state, controls_sample sample,
                                controls_screen screen) {
    controls_output out = {0};
    if (state == NULL)
        return out;
    bool fire_rise = false;
    if (sample.fire_pressed != state->fire_candidate) {
        state->fire_candidate = sample.fire_pressed;
        state->fire_since_ms = sample.now_ms;
    }
    if (state->fire_stable != state->fire_candidate &&
        sample.now_ms - state->fire_since_ms >= CONTROLS_DEBOUNCE_MS) {
        state->fire_stable = state->fire_candidate;
        fire_rise = state->fire_stable;
    }
    if (!state->fire_stable && !sample.fire_pressed)
        state->fire_blocked = false;
    bool screen_changed = !state->screen_valid || screen != state->screen;
    if (screen_changed) {
        state->screen = screen;
        state->screen_valid = true;
        if (state->mode == CONTROLS_TILT)
            controls_tilt_reset(state);
        state->touch_down = sample.touch_pressed;
        state->touch_cancelled = sample.touch_pressed;
        state->touch_target = CONTROLS_TARGET_NONE;
        state->drag_valid = false;
        state->fire_blocked = sample.fire_pressed || state->fire_stable;
    }
    if (!state->fire_blocked) {
        if (screen == CONTROLS_PLAY)
            out.input.fire = state->fire_stable;
        else if (fire_rise) {
            if (screen == CONTROLS_PAUSE)
                out.input.pause = true;
            else
                out.input.start = true;
        }
    }
    if (state->mode == CONTROLS_TILT && screen == CONTROLS_PLAY)
        out.input.angle_delta = tilt_input(state, &sample);
    bool invalid = sample.touch_invalid || state->touch_ignore_sample ||
                   (sample.touch_pressed && (sample.touch_x >= 240u || sample.touch_y >= 240u));
    state->touch_ignore_sample = false;
    if (invalid) {
        state->touch_wait_release = true;
        state->touch_cancelled = true;
        state->touch_down = false;
        state->drag_valid = false;
        return out;
    }
    if (state->touch_wait_release) {
        if (!sample.touch_pressed)
            state->touch_wait_release = false;
        state->touch_down = false;
        state->drag_valid = false;
        return out;
    }
    if (sample.touch_pressed && !state->touch_down) {
        state->touch_down = true;
        state->touch_cancelled = false;
        state->reboot_sent = false;
        state->touch_origin_x = sample.touch_x;
        state->touch_origin_y = sample.touch_y;
        state->touch_since_ms = sample.now_ms;
        state->touch_target = controls_hit_test(screen, sample.touch_x, sample.touch_y);
        state->drag_valid = false;
    }
    if (sample.touch_pressed && state->touch_down) {
        if (state->touch_target != CONTROLS_TARGET_NONE) {
            int dx = (int)sample.touch_x - (int)state->touch_origin_x;
            int dy = (int)sample.touch_y - (int)state->touch_origin_y;
            if (dx * dx + dy * dy > 144 ||
                controls_hit_test(screen, sample.touch_x, sample.touch_y) != state->touch_target)
                state->touch_cancelled = true;
            if (!state->touch_cancelled && !state->reboot_sent &&
                state->touch_target == CONTROLS_TARGET_REBOOT &&
                sample.now_ms - state->touch_since_ms >= CONTROLS_REBOOT_HOLD_MS) {
                out.reboot_requested = true;
                state->reboot_sent = true;
            }
        } else if (screen == CONTROLS_PLAY && !state->touch_cancelled) {
            int32_t r2 = radius_squared(sample.touch_x, sample.touch_y);
            if (state->mode == CONTROLS_SPLIT_HOLD) {
                if (r2 > CONTROLS_CENTER_RADIUS * CONTROLS_CENTER_RADIUS && r2 <= 120 * 120) {
                    if (sample.touch_x < 108u)
                        out.input.turn = -1;
                    else if (sample.touch_x >= 132u)
                        out.input.turn = 1;
                }
            } else if (state->mode == CONTROLS_RELATIVE_DRAG &&
                       r2 >= CONTROLS_DRAG_MIN_RADIUS * CONTROLS_DRAG_MIN_RADIUS &&
                       r2 <= 120 * 120) {
                uint16_t angle = touch_angle(sample.touch_x, sample.touch_y);
                if (state->drag_valid) {
                    int32_t delta = (int32_t)angle - (int32_t)state->drag_angle;
                    if (delta > 32767)
                        delta -= 65536;
                    else if (delta < -32768)
                        delta += 65536;
                    /* Discontinuities re-anchor; a bad report cannot spin the ship. */
                    if (delta >= -8192 && delta <= 8192)
                        out.input.angle_delta = (int16_t)delta;
                }
                state->drag_angle = angle;
                state->drag_valid = true;
            } else
                state->drag_valid = false;
        }
    } else if (state->touch_down) {
        if (!state->touch_cancelled && !state->reboot_sent &&
            sample.now_ms - state->touch_since_ms <= 650u) {
            switch (state->touch_target) {
            case CONTROLS_TARGET_START:
                out.input.start = true;
                break;
            case CONTROLS_TARGET_PAUSE:
                out.input.pause = true;
                break;
            case CONTROLS_TARGET_QUIT:
                out.input.quit = true;
                break;
            case CONTROLS_TARGET_MODE:
                controls_set_mode(state, (controls_mode)(((unsigned)state->mode + 1u) %
                                                         (unsigned)CONTROLS_MODE_COUNT));
                out.mode_changed = true;
                break;
            default:
                break;
            }
        }
        state->touch_down = false;
        state->drag_valid = false;
    }
    return out;
}
