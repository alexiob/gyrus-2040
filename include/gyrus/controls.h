#ifndef GYRUS_CONTROLS_H
#define GYRUS_CONTROLS_H
#include "gyrus/game.h"
#include <stdbool.h>
#include <stdint.h>

/* Half-open UI rectangles, in native 240x240 display coordinates. */
#define CONTROLS_START_X0 60u
#define CONTROLS_START_X1 180u
#define CONTROLS_START_Y0 92u
#define CONTROLS_START_Y1 136u
#define CONTROLS_MODE_X0 48u
#define CONTROLS_MODE_X1 192u
#define CONTROLS_MODE_Y0 145u
#define CONTROLS_MODE_Y1 177u
#define CONTROLS_RESUME_X0 60u
#define CONTROLS_RESUME_X1 180u
#define CONTROLS_RESUME_Y0 80u
#define CONTROLS_RESUME_Y1 104u
#define CONTROLS_PAUSE_MODE_X0 48u
#define CONTROLS_PAUSE_MODE_X1 192u
#define CONTROLS_PAUSE_MODE_Y0 115u
#define CONTROLS_PAUSE_MODE_Y1 137u
#define CONTROLS_QUIT_X0 60u
#define CONTROLS_QUIT_X1 180u
#define CONTROLS_QUIT_Y0 145u
#define CONTROLS_QUIT_Y1 169u
#define CONTROLS_REBOOT_X0 68u
#define CONTROLS_REBOOT_X1 172u
#define CONTROLS_REBOOT_Y0 181u
#define CONTROLS_REBOOT_Y1 205u
#define CONTROLS_CENTER_X 120
#define CONTROLS_CENTER_Y 120
#define CONTROLS_CENTER_RADIUS 36
#define CONTROLS_DRAG_MIN_RADIUS 56
#define CONTROLS_DEBOUNCE_MS 25u
#define CONTROLS_REBOOT_HOLD_MS 5000u

#define CONTROLS_TILT_CALIBRATE_MS 500u
#define CONTROLS_MOTION_STALE_MS 100u
typedef enum {
    CONTROLS_SPLIT_HOLD,
    CONTROLS_RELATIVE_DRAG,
    CONTROLS_TILT,
    CONTROLS_MODE_COUNT
} controls_mode;
typedef struct {
    int32_t accel_mg[3];
    int32_t gyro_mdps[3];
    uint32_t sampled_ms;
} controls_motion;
typedef enum { CONTROLS_TITLE, CONTROLS_PLAY, CONTROLS_PAUSE, CONTROLS_GAME_OVER } controls_screen;
typedef enum {
    CONTROLS_TARGET_NONE,
    CONTROLS_TARGET_START,
    CONTROLS_TARGET_MODE,
    CONTROLS_TARGET_QUIT,
    CONTROLS_TARGET_REBOOT,
    CONTROLS_TARGET_PAUSE
} controls_target;
typedef struct {
    uint32_t now_ms;
    bool touch_pressed;
    uint16_t touch_x, touch_y;
    bool fire_pressed;  /* Raw physical BOOT sample, active high. */
    bool touch_invalid; /* Bus error, not a physical release. Zero means valid. */
    bool motion_valid;  /* True only for a newly acquired, complete IMU sample. */
    controls_motion motion;
} controls_sample;
typedef struct {
    game_input input;
    bool reboot_requested; /* One pulse after a continuous five-second REBOOT hold. */
    bool mode_changed;
} controls_output;
typedef struct {
    controls_mode mode;
    controls_screen screen;
    bool screen_valid, touch_down, touch_cancelled, reboot_sent;
    bool touch_wait_release, touch_ignore_sample;
    bool fire_candidate, fire_stable, fire_blocked;
    uint32_t fire_since_ms, touch_since_ms;
    uint16_t touch_origin_x, touch_origin_y, drag_angle;
    bool drag_valid;
    controls_target touch_target;
    bool tilt_ready, tilt_available, tilt_have_sample, tilt_calibrating;
    uint16_t tilt_calibration_count, tilt_reference;
    uint32_t tilt_last_ms, tilt_calibration_started;
    int32_t tilt_neutral, tilt_angle, tilt_filtered, tilt_gyro_bias;
    int32_t tilt_angle_sum, tilt_gyro_sum;

} controls_state;

void controls_init(controls_state *state);
void controls_set_mode(controls_state *state, controls_mode mode);
/* Call before update when the touch bus/read fails; suppresses release actions. */
void controls_cancel_touch(controls_state *state);
void controls_tilt_reset(controls_state *state);
bool controls_tilt_ready(const controls_state *state);
uint16_t controls_tilt_progress(const controls_state *state, uint32_t now_ms);
const char *controls_mode_name(controls_mode mode);
controls_target controls_hit_test(controls_screen screen, uint16_t x, uint16_t y);
/* Call with a monotonic wrapping uint32 millisecond clock, normally once/tick.
 * Set touch_invalid on a failed sensor read; cancellation waits for a valid release. Screen changes
 * consume held inputs. There is no allocation or floating point in this layer. */
controls_output controls_update(controls_state *state, controls_sample sample,
                                controls_screen screen);
/* 0..1000 progress for the REBOOT affordance. Zero when no valid REBOOT hold. */
uint16_t controls_reboot_progress(const controls_state *state, uint32_t now_ms);
#endif
