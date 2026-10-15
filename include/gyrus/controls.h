#ifndef GYRUS_CONTROLS_H
#define GYRUS_CONTROLS_H
#include "engine/input.h"
#include <stdbool.h>
#include <stdint.h>

/* Half-open UI rectangles, in native 240x240 display coordinates. */
#define CONTROLS_START_X0 60u
#define CONTROLS_START_X1 180u
#define CONTROLS_START_Y0 92u
#define CONTROLS_START_Y1 136u
#define CONTROLS_RESUME_X0 60u
#define CONTROLS_RESUME_X1 180u
#define CONTROLS_RESUME_Y0 80u
#define CONTROLS_RESUME_Y1 104u
#define CONTROLS_QUIT_X0 60u
#define CONTROLS_QUIT_X1 180u
#define CONTROLS_QUIT_Y0 149u
#define CONTROLS_QUIT_Y1 173u
#define CONTROLS_REBOOT_X0 68u
#define CONTROLS_REBOOT_X1 172u
#define CONTROLS_REBOOT_Y0 183u
#define CONTROLS_REBOOT_Y1 207u
#define CONTROLS_SETTINGS_X0 60u
#define CONTROLS_SETTINGS_X1 180u
#define CONTROLS_SETTINGS_Y0 115u
#define CONTROLS_SETTINGS_Y1 139u
#define CONTROLS_FIRE_DOWN_X0 42u
#define CONTROLS_FIRE_DOWN_X1 80u
#define CONTROLS_FIRE_DOWN_Y0 94u
#define CONTROLS_FIRE_DOWN_Y1 120u
#define CONTROLS_FIRE_UP_X0 160u
#define CONTROLS_FIRE_UP_X1 198u
#define CONTROLS_FIRE_UP_Y0 94u
#define CONTROLS_FIRE_UP_Y1 120u
#define CONTROLS_ENEMY_DOWN_X0 42u
#define CONTROLS_ENEMY_DOWN_X1 80u
#define CONTROLS_ENEMY_DOWN_Y0 137u
#define CONTROLS_ENEMY_DOWN_Y1 163u
#define CONTROLS_ENEMY_UP_X0 160u
#define CONTROLS_ENEMY_UP_X1 198u
#define CONTROLS_ENEMY_UP_Y0 137u
#define CONTROLS_ENEMY_UP_Y1 163u
#define CONTROLS_TILT_DOWN_X0 42u
#define CONTROLS_TILT_DOWN_X1 80u
#define CONTROLS_TILT_DOWN_Y0 180u
#define CONTROLS_TILT_DOWN_Y1 206u
#define CONTROLS_TILT_UP_X0 160u
#define CONTROLS_TILT_UP_X1 198u
#define CONTROLS_TILT_UP_Y0 180u
#define CONTROLS_TILT_UP_Y1 206u
#define CONTROLS_BACK_X0 78u
#define CONTROLS_BACK_X1 162u
#define CONTROLS_BACK_Y0 209u
#define CONTROLS_BACK_Y1 231u
/* Title Settings occupies the former mode button. Mode selection is inside Settings. */
#define CONTROLS_TITLE_SETTINGS_X0 48u
#define CONTROLS_TITLE_SETTINGS_X1 192u
#define CONTROLS_TITLE_SETTINGS_Y0 145u
#define CONTROLS_TITLE_SETTINGS_Y1 177u
#define CONTROLS_SETTINGS_MODE_X0 48u
#define CONTROLS_SETTINGS_MODE_X1 192u
#define CONTROLS_SETTINGS_MODE_Y0 52u
#define CONTROLS_SETTINGS_MODE_Y1 76u
#define CONTROLS_CALIBRATE_X0 68u
#define CONTROLS_CALIBRATE_X1 172u
#define CONTROLS_CALIBRATE_Y0 216u
#define CONTROLS_CALIBRATE_Y1 240u
#define CONTROLS_HUB_PREV_X0 12u
#define CONTROLS_HUB_PREV_X1 60u
#define CONTROLS_HUB_NEXT_X0 180u
#define CONTROLS_HUB_NEXT_X1 228u
#define CONTROLS_HUB_ARROW_Y0 76u
#define CONTROLS_HUB_ARROW_Y1 140u
#define CONTROLS_HUB_PLAY_Y0 145u
#define CONTROLS_HUB_PLAY_Y1 177u
#define CONTROLS_HUB_SETTINGS_Y0 184u
#define CONTROLS_HUB_SETTINGS_Y1 208u
#define CONTROLS_STYLE_X0 66u
#define CONTROLS_STYLE_X1 174u
#define CONTROLS_STYLE_Y0 112u
#define CONTROLS_STYLE_Y1 138u
#define CONTROLS_HOME_X0 14u
#define CONTROLS_HOME_X1 48u
#define CONTROLS_HOME_Y0 94u
#define CONTROLS_HOME_Y1 128u
#define CONTROLS_SPECIAL_X0 90u
#define CONTROLS_SPECIAL_X1 150u
#define CONTROLS_SPECIAL_Y0 176u
#define CONTROLS_SPECIAL_Y1 202u
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
/* Display-aligned body frame: +X is screen-right. Hardware backends rotate
 * acceleration and angular velocity together from their sensor mounting. */
typedef struct {
    int32_t accel_mg[3];
    int32_t gyro_mdps[3];
    uint32_t sampled_ms;
} controls_motion;
typedef enum {
    CONTROLS_TITLE,
    CONTROLS_PLAY,
    CONTROLS_PAUSE,
    CONTROLS_GAME_OVER,
    CONTROLS_SETTINGS,
    CONTROLS_HUB,
    CONTROLS_TEMPEST_PLAY
} controls_screen;
typedef enum {
    CONTROLS_TARGET_NONE,
    CONTROLS_TARGET_START,
    CONTROLS_TARGET_MODE,
    CONTROLS_TARGET_QUIT,
    CONTROLS_TARGET_REBOOT,
    CONTROLS_TARGET_PAUSE,
    CONTROLS_TARGET_SETTINGS,
    CONTROLS_TARGET_BACK,
    CONTROLS_TARGET_FIRE_DOWN,
    CONTROLS_TARGET_FIRE_UP,
    CONTROLS_TARGET_ENEMY_DOWN,
    CONTROLS_TARGET_ENEMY_UP,
    CONTROLS_TARGET_TILT_DOWN,
    CONTROLS_TARGET_TILT_UP,
    CONTROLS_TARGET_CALIBRATE,
    CONTROLS_TARGET_PREVIOUS,
    CONTROLS_TARGET_NEXT,
    CONTROLS_TARGET_STYLE,
    CONTROLS_TARGET_HOME,
    CONTROLS_TARGET_SPECIAL
} controls_target;
typedef struct {
    uint32_t now_ms;
    bool touch_pressed;
    uint16_t touch_x, touch_y;
    bool fire_pressed;  /* Raw physical BOOT sample, active high. */
    bool touch_invalid; /* Bus error, not a physical release. Zero means valid. */
    bool motion_valid;  /* True only for a newly acquired, complete IMU sample. */
    controls_motion motion;
    uint16_t tilt_sensitivity_percent; /*0=100 default; other values clamp50..200. */
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
    uint16_t touch_origin_x, touch_origin_y, touch_last_x, touch_last_y, drag_angle;
    bool drag_valid;
    controls_target touch_target;
    bool tilt_ready, tilt_available, tilt_have_sample, tilt_calibrating;
    uint16_t tilt_calibration_count;
    uint32_t tilt_last_ms, tilt_calibration_started;
    int32_t tilt_angle, tilt_filtered;
    int32_t tilt_gravity[3], tilt_right[3], tilt_axis[3]; /* Learned Q14 basis. */
    int32_t tilt_calibration_reference[3], tilt_accel_sum[3];
    int32_t tilt_gyro_sum[3], tilt_gyro_bias[3];
    int16_t tilt_preview_value;

} controls_state;

void controls_init(controls_state *state);
void controls_set_mode(controls_state *state, controls_mode mode);
/* Call before update when the touch bus/read fails; suppresses release actions. */
void controls_cancel_touch(controls_state *state);
/* Consume held touch/BOOT after changing active games, retaining tilt calibration. */
void controls_consume_inputs(controls_state *state);
void controls_tilt_reset(controls_state *state);
bool controls_tilt_ready(const controls_state *state);
uint16_t controls_tilt_progress(const controls_state *state, uint32_t now_ms);
/* Post-sensitivity steering preview, signed -1000..1000; zero until ready. */
int16_t controls_tilt_preview(const controls_state *state);
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
