#ifndef GYRUS_GAME_H
#define GYRUS_GAME_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define GAME_WIDTH 240u
#define GAME_HEIGHT 240u
#define GAME_ENEMIES 48u
#define GAME_SHOTS 48u
#define GAME_HOSTILE_SHOTS 48u
#define GAME_PARTICLES 24u
#define GAME_STARS 64u
#define GAME_TICK_HZ 60u
#define GAME_CLEAR_TICKS 30u
#define GAME_CHANCE_RESULT_TICKS 120u
#define GAME_WARP_TICKS 90u
#define GAME_ARRIVAL_TICKS 120u
/* Angle0=up, 16384=right. Positive turn is clockwise. Radii use Q8.8 pixels. */
typedef enum {
    GAME_TITLE,
    GAME_INTRO,
    GAME_PLAY,
    GAME_CHANCE,
    GAME_ARRIVAL,
    GAME_PAUSED,
    GAME_OVER,
    GAME_CLEAR,
    GAME_WARP
} game_phase;
typedef enum {
    GAME_MENU_NONE,
    GAME_MENU_SETTINGS,
    GAME_MENU_BACK,
    GAME_MENU_FIRE_DOWN,
    GAME_MENU_FIRE_UP,
    GAME_MENU_ENEMY_DOWN,
    GAME_MENU_ENEMY_UP,
    GAME_MENU_TILT_DOWN,
    GAME_MENU_TILT_UP
} game_menu_action;
typedef enum {
    GAME_SETTING_FIRE_RATE,
    GAME_SETTING_ENEMY_SPEED,
    GAME_SETTING_TILT_SENSITIVITY
} game_setting;
typedef struct {
    uint8_t fire_rate;                 /* Volleys/second,1..10. Default5. */
    uint16_t enemy_speed_percent;      /*25..200 in25% steps;100=the current tuned speed. */
    uint16_t tilt_sensitivity_percent; /*50..200 in25% steps;100=the current gain. */
} game_settings;
typedef struct {
    int8_t turn;
    int16_t angle_delta;
    bool fire, start, pause, quit;
    game_menu_action menu_action;
} game_input;
typedef struct {
    int32_t radius;
    uint16_t angle;
    uint16_t age;
    uint8_t kind, mode, hp, active;
    int16_t angular_speed;
} game_enemy;
typedef struct {
    int32_t radius;
    uint16_t angle;
    uint16_t age;
    uint8_t active;
} game_shot;
typedef struct {
    int16_t x, y, vx, vy;
    uint8_t life, color;
} game_particle;
typedef struct {
    uint16_t angle;
    uint16_t radius;
    uint8_t speed, color;
} game_star;
typedef struct game_state {
    uint32_t tick, rng, score, high_score;
    uint16_t stage, phase_ticks, ship_angle, fire_cooldown, invulnerable;
    uint16_t spawned, destroyed, escaped, stage_target;
    uint8_t lives, planet, warp, double_fire, cycle, control_mode;
    uint16_t reboot_progress;
    game_phase phase, resume_phase;
    bool invincible, autofire;
    game_settings settings;
    uint8_t fire_rate_remainder, enemy_time_remainder;
    uint16_t enemy_ticks;
    bool pause_settings;       /* Settings page in Title/Over/Paused; gates menu actions. */
    bool clear_chance;         /* Current clear sequence is a chance-stage result. */
    uint32_t transition_bonus; /* Awarded once when the chance result begins. */
    bool tilt_ready;           /* UI only; excluded from simulation hash. */
    uint16_t tilt_progress;    /* Calibration0..1000, UI only. */
    int16_t tilt_preview;      /* Steering-1000..1000, UI only. */
    game_enemy enemies[GAME_ENEMIES];
    game_shot shots[GAME_SHOTS], hostile[GAME_HOSTILE_SHOTS];
    game_particle particles[GAME_PARTICLES];
    game_star stars[GAME_STARS];
} game_state;
void game_init(game_state *state, uint32_t seed);
void game_start(game_state *state);
bool game_set_setting(game_state *state, game_setting setting, int32_t value);
void game_step(game_state *state, const game_input *input);
bool game_set_stage(game_state *state, unsigned stage);
/* Canonical deterministic state hash, excludes padding. */
uint32_t game_state_hash(const game_state *state);
int16_t game_sin_q15(uint16_t angle);
int16_t game_cos_q15(uint16_t angle);
void game_polar(uint16_t angle, int32_t radius_q8, int16_t *x, int16_t *y);
/* Caller provides exactly57600 indexed8 pixels. Pure rendering, no state changes. */
void game_render(const game_state *state, uint8_t framebuffer[GAME_WIDTH * GAME_HEIGHT]);
const char *game_phase_name(game_phase phase);
const char *game_planet_name(unsigned planet);
#endif
