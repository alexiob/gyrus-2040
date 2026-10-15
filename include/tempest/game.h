#ifndef TEMPEST_GAME_H
#define TEMPEST_GAME_H
#include "engine/game_api.h"
#define TEMPEST_ENEMIES 32u
#define TEMPEST_SHOTS 8u
#define TEMPEST_HOSTILE 12u
#define TEMPEST_RAILS 17u
#define TEMPEST_DEPTH 65536
#define TEMPEST_VISUAL_EVENTS 8u
#define TEMPEST_INTRO_TICKS 60u
/* Depth Q16: zero at far rim, 65536 at player rim. Lane position Q8. */
typedef enum {
    TEMPEST_TITLE,
    TEMPEST_PLAY,
    TEMPEST_CLEAR,
    TEMPEST_DIVE,
    TEMPEST_FLIGHT,
    TEMPEST_DEATH,
    TEMPEST_PAUSED,
    TEMPEST_OVER,
    TEMPEST_INTRO /* Appended: preserve existing phase values 0..7. */
} tempest_phase;
typedef enum {
    TEMPEST_FLIPPER,
    TEMPEST_TANKER,
    TEMPEST_SPIKER,
    TEMPEST_FUSEBALL,
    TEMPEST_PULSAR
} tempest_enemy_kind;
typedef struct {
    int16_t x, y;
} tempest_point;
typedef struct {
    const char *name;
    uint8_t rail_count, lane_count;
    bool closed;
    tempest_point rim[TEMPEST_RAILS];
} tempest_web;
typedef struct {
    int32_t depth;
    uint16_t age, timer;
    int16_t lane_q8;
    int8_t direction;
    uint8_t kind, active, landed, hp, rim, crossing, retreat, payload;
} tempest_enemy;
typedef struct {
    int32_t depth;
    uint8_t lane, active;
} tempest_shot;
typedef enum {
    TEMPEST_FX_MUZZLE,
    TEMPEST_FX_HIT,
    TEMPEST_FX_SPLIT,
    TEMPEST_FX_LAND,
    TEMPEST_FX_ZAP,
    TEMPEST_FX_DEATH,
    TEMPEST_FX_PULSE
} tempest_visual_kind;
typedef struct {
    int32_t depth;
    int16_t lane_q8;
    uint8_t age, life, kind;
} tempest_visual_event;
typedef struct tempest_state {
    uint32_t tick, play_tick, rng, score, high_score, next_bonus;
    uint16_t stage, phase_ticks, fire_cooldown, spawn_timer, invulnerable;
    uint16_t spawned, stage_target, destroyed, escaped;
    int32_t claw_q8, dive_depth;
    tempest_phase phase, resume_phase, pause_phase;
    game_settings settings;
    engine_ui ui;
    bool effects, invincible, autofire, settings_open;
    uint8_t lives, zaps, fire_remainder;
    tempest_enemy enemies[TEMPEST_ENEMIES];
    tempest_shot shots[TEMPEST_SHOTS], hostile[TEMPEST_HOSTILE];
    int32_t spikes[TEMPEST_RAILS];
    /* Presentation only: updated identically in classic/neon, omitted from hash. */
    tempest_visual_event visual[TEMPEST_VISUAL_EVENTS];
    uint8_t visual_cursor, zap_flash;
} tempest_state;
extern const tempest_web tempest_webs[16];
const tempest_web *tempest_current_web(const tempest_state *);
int tempest_neighbor(const tempest_web *, int lane, int direction);
void tempest_project(const tempest_web *, int lane_q8, int32_t depth, int *x, int *y);
void tempest_init(tempest_state *, uint32_t seed);
void tempest_start(tempest_state *);
bool tempest_set_stage(tempest_state *, unsigned stage);
void tempest_step(tempest_state *, const engine_input *);
void tempest_render(const tempest_state *, uint8_t *pixels);
uint32_t tempest_hash(const tempest_state *);
const char *tempest_phase_name(tempest_phase);
#endif
