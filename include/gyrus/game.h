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
/* Angle0=up, 16384=right. Positive turn is clockwise. Radii use Q8.8 pixels. */
typedef enum { GAME_TITLE, GAME_INTRO, GAME_PLAY, GAME_CHANCE, GAME_ARRIVAL, GAME_PAUSED, GAME_OVER } game_phase;
typedef struct {
    int8_t turn;
    int16_t angle_delta;
    bool fire,start,pause,quit;
} game_input;
typedef struct {
    int32_t radius;
    uint16_t angle;
    uint16_t age;
    uint8_t kind,mode,hp,active;
    int16_t angular_speed;
} game_enemy;
typedef struct { int32_t radius; uint16_t angle; uint16_t age; uint8_t active; } game_shot;
typedef struct { int16_t x,y,vx,vy; uint8_t life,color; } game_particle;
typedef struct { uint16_t angle; uint16_t radius; uint8_t speed,color; } game_star;
typedef struct game_state {
    uint32_t tick,rng,score,high_score;
    uint16_t stage,phase_ticks,ship_angle,fire_cooldown,invulnerable;
    uint16_t spawned,destroyed,escaped,stage_target;
    uint8_t lives,planet,warp,double_fire,cycle;
    game_phase phase,resume_phase;
    bool invincible,autofire;
    game_enemy enemies[GAME_ENEMIES];
    game_shot shots[GAME_SHOTS],hostile[GAME_HOSTILE_SHOTS];
    game_particle particles[GAME_PARTICLES];
    game_star stars[GAME_STARS];
} game_state;
void game_init(game_state *state,uint32_t seed);
void game_start(game_state *state);
void game_step(game_state *state,const game_input *input);
bool game_set_stage(game_state *state,unsigned stage);
/* Canonical deterministic state hash, excludes padding. */
uint32_t game_state_hash(const game_state *state);
int16_t game_sin_q15(uint16_t angle);
int16_t game_cos_q15(uint16_t angle);
void game_polar(uint16_t angle,int32_t radius_q8,int16_t *x,int16_t *y);
/* Caller provides exactly57600 indexed8 pixels. Pure rendering, no state changes. */
void game_render(const game_state *state,uint8_t framebuffer[GAME_WIDTH*GAME_HEIGHT]);
const char *game_phase_name(game_phase phase);
const char *game_planet_name(unsigned planet);
#endif
