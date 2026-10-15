#include "gyrus/game.h"
#include <limits.h>
#include <string.h>

static const int16_t sine[256] = {
#include "sine_table.inc"
};
static uint16_t add_angle(uint16_t a, int32_t delta) {
    return (uint16_t)((int32_t)a + delta);
}
int16_t game_sin_q15(uint16_t angle) {
    return sine[angle >> 8u];
}
int16_t game_cos_q15(uint16_t angle) {
    return sine[(uint8_t)((angle >> 8u) + 64u)];
}
void game_polar(uint16_t angle, int32_t radius, int16_t *x, int16_t *y) {
    if (radius > 200 * 256)
        radius = 200 * 256;
    if (radius < -200 * 256)
        radius = -200 * 256;
    *x = (int16_t)(120 + ((int32_t)game_sin_q15(angle) * radius) / 8388608);
    *y = (int16_t)(120 - ((int32_t)game_cos_q15(angle) * radius) / 8388608);
}
static uint32_t random_word(game_state *g) {
    uint32_t x = g->rng;
    x ^= x << 13u;
    x ^= x >> 17u;
    x ^= x << 5u;
    g->rng = x;
    return x;
}
const char *game_planet_name(unsigned p) {
    static const char *const names[] = {"NEPTUNE", "URANUS", "SATURN", "JUPITER", "MARS", "EARTH"};
    return names[p % 6u];
}
const char *game_phase_name(game_phase p) {
    static const char *const names[] = {"title",  "intro",     "play",  "chance", "arrival",
                                        "paused", "game_over", "clear", "warp"};
    return (unsigned)p < 9u ? names[p] : "invalid";
}
static void clear_entities(game_state *g) {
    memset(g->enemies, 0, sizeof g->enemies);
    memset(g->shots, 0, sizeof g->shots);
    memset(g->hostile, 0, sizeof g->hostile);
    memset(g->particles, 0, sizeof g->particles);
}
void game_init(game_state *g, uint32_t seed) {
    memset(g, 0, sizeof *g);
    g->rng = seed ? seed : 0x20401983u;
    g->lives = 3;
    g->autofire = true;
    g->settings = (game_settings){5, 100, 100};
    g->stage = 1;
    g->warp = 2;
    g->phase = GAME_TITLE;
    g->ship_angle = 32768;
    for (unsigned i = 0; i < GAME_STARS; ++i) {
        g->stars[i] = (game_star){.angle = (uint16_t)random_word(g),
                                  .radius = (uint16_t)(random_word(g) % (120u * 256u)),
                                  .speed = (uint8_t)(1u + i % 3u),
                                  .color = (uint8_t)(i % 4u)};
    }
}
bool game_set_stage(game_state *g, unsigned stage) {
    if (stage < 1u || stage > 4095u)
        return false;
    g->stage = (uint16_t)stage;
    if (stage <= 3u) {
        g->planet = 0;
        g->warp = (uint8_t)(3u - stage);
        g->cycle = 0;
    } else {
        const unsigned group = (stage - 4u) / 4u, local = (stage - 4u) % 4u;
        g->planet = (uint8_t)((group + 1u) % 6u);
        g->warp = (uint8_t)(3u - local);
        const unsigned cycle = (group + 1u) / 6u;
        g->cycle = (uint8_t)(cycle > 12u ? 12u : cycle);
    }
    g->phase = GAME_INTRO;
    g->phase_ticks = 0;
    g->pause_settings = false;
    g->clear_chance = false;
    g->transition_bonus = 0;
    g->spawned = 0;
    g->destroyed = 0;
    g->escaped = 0;
    g->stage_target = 40;
    g->fire_cooldown = 0;
    g->fire_rate_remainder = 0;
    g->enemy_time_remainder = 0;
    g->enemy_ticks = 0;
    g->invulnerable = 90;
    clear_entities(g);
    return true;
}
void game_start(game_state *g) {
    const uint32_t high = g->high_score, seed = g->rng;
    const uint8_t mode = g->control_mode;
    const game_settings settings = g->settings;
    const bool invincible = g->invincible, autofire = g->autofire;
    game_init(g, seed);
    g->high_score = high;
    g->control_mode = mode;
    g->settings = settings;
    g->invincible = invincible;
    g->autofire = autofire;
    (void)game_set_stage(g, 1);
}
bool game_set_setting(game_state *g, game_setting setting, int32_t value) {
    if (!g)
        return false;
    switch (setting) {
    case GAME_SETTING_FIRE_RATE:
        if (value < 1 || value > 10)
            return false;
        if (g->settings.fire_rate != (uint8_t)value) {
            g->settings.fire_rate = (uint8_t)value;
            g->fire_cooldown = 0;
            g->fire_rate_remainder = 0;
        }
        return true;
    case GAME_SETTING_ENEMY_SPEED:
        if (value < 25 || value > 200 || value % 25 != 0)
            return false;
        g->settings.enemy_speed_percent = (uint16_t)value;
        return true;
    case GAME_SETTING_TILT_SENSITIVITY:
        if (value < 50 || value > 200 || value % 25 != 0)
            return false;
        g->settings.tilt_sensitivity_percent = (uint16_t)value;
        return true;
    }
    return false;
}
static void pause_action(game_state *g, game_menu_action action) {
    if (action == GAME_MENU_SETTINGS) {
        g->pause_settings = true;
        return;
    }
    if (action == GAME_MENU_BACK) {
        g->pause_settings = false;
        return;
    }
    if (!g->pause_settings)
        return;
    switch (action) {
    case GAME_MENU_FIRE_DOWN:
        (void)game_set_setting(g, GAME_SETTING_FIRE_RATE, (int32_t)g->settings.fire_rate - 1);
        break;
    case GAME_MENU_FIRE_UP:
        (void)game_set_setting(g, GAME_SETTING_FIRE_RATE, (int32_t)g->settings.fire_rate + 1);
        break;
    case GAME_MENU_ENEMY_DOWN:
        (void)game_set_setting(g, GAME_SETTING_ENEMY_SPEED,
                               (int32_t)g->settings.enemy_speed_percent - 25);
        break;
    case GAME_MENU_ENEMY_UP:
        (void)game_set_setting(g, GAME_SETTING_ENEMY_SPEED,
                               (int32_t)g->settings.enemy_speed_percent + 25);
        break;
    case GAME_MENU_TILT_DOWN:
        (void)game_set_setting(g, GAME_SETTING_TILT_SENSITIVITY,
                               (int32_t)g->settings.tilt_sensitivity_percent - 25);
        break;
    case GAME_MENU_TILT_UP:
        (void)game_set_setting(g, GAME_SETTING_TILT_SENSITIVITY,
                               (int32_t)g->settings.tilt_sensitivity_percent + 25);
        break;
    default:
        break;
    }
}
static void score_add(game_state *g, uint32_t amount) {
    const uint32_t before = g->score < 30000u ? 0u : 1u + (g->score - 30000u) / 60000u;
    g->score = amount > 99999999u - g->score ? 99999999u : g->score + amount;
    const uint32_t after = g->score < 30000u ? 0u : 1u + (g->score - 30000u) / 60000u;
    if (after > before && g->lives < 9u)
        ++g->lives;
    if (g->score > g->high_score)
        g->high_score = g->score;
}
static void explosion(game_state *g, uint16_t angle, int32_t radius, uint8_t color) {
    int16_t x, y;
    game_polar(angle, radius, &x, &y);
    unsigned added = 0;
    for (unsigned i = 0; i < GAME_PARTICLES && added < 6u; ++i)
        if (g->particles[i].life == 0u) {
            const uint32_t r = random_word(g);
            g->particles[i] = (game_particle){.x = x,
                                              .y = y,
                                              .vx = (int16_t)((int)(r % 5u) - 2),
                                              .vy = (int16_t)((int)((r >> 8u) % 5u) - 2),
                                              .life = (uint8_t)(18u + r % 10u),
                                              .color = color};
            ++added;
        }
}
static game_enemy *spawn_enemy(game_state *g, uint8_t kind, uint16_t angle, int32_t radius,
                               uint8_t mode) {
    for (unsigned i = 0; i < GAME_ENEMIES; ++i)
        if (!g->enemies[i].active) {
            const int16_t speed = (int16_t)(170u + (unsigned)g->cycle * 12u + kind * 12u);
            g->enemies[i] = (game_enemy){.radius = radius,
                                         .angle = angle,
                                         .kind = kind,
                                         .mode = mode,
                                         .hp = 1,
                                         .active = 1,
                                         .angular_speed = (i & 1u) ? (int16_t)-speed : speed};
            return &g->enemies[i];
        }
    return NULL;
}
static void fire_shot(game_shot *pool, unsigned capacity, uint16_t angle, int32_t radius) {
    for (unsigned i = 0; i < capacity; ++i)
        if (!pool[i].active) {
            pool[i] = (game_shot){.radius = radius, .angle = angle, .active = 1};
            return;
        }
}
static bool near_polar(uint16_t a, int32_t r, uint16_t b, int32_t s, int threshold) {
    int32_t diff = r - s;
    if (diff < 0)
        diff = -diff;
    if (diff > (threshold + 1) * 256)
        return false;
    int16_t ax, ay, bx, by;
    game_polar(a, r, &ax, &ay);
    game_polar(b, s, &bx, &by);
    const int32_t dx = (int32_t)ax - bx, dy = (int32_t)ay - by;
    return dx * dx + dy * dy <= threshold * threshold;
}
static void die(game_state *g) {
    if (g->invincible || g->invulnerable > 0u || g->phase != GAME_PLAY)
        return;
    explosion(g, g->ship_angle, 104 * 256, 8);
    if (g->lives > 0u)
        --g->lives;
    g->double_fire = 0;
    g->invulnerable = 150;
    g->ship_angle = 32768;
    memset(g->hostile, 0, sizeof g->hostile);
    memset(g->shots, 0, sizeof g->shots);
    if (g->lives == 0u) {
        g->phase = GAME_OVER;
        g->phase_ticks = 0;
    }
}
static void kill_enemy(game_state *g, game_enemy *e) {
    e->active = 0;
    explosion(g, e->angle, e->radius, (uint8_t)(e->kind == 1u ? 6u : 9u));
    if (e->kind <= 2u) {
        ++g->destroyed;
        score_add(g, g->phase == GAME_CHANCE ? 100u : 100u + (uint32_t)e->kind * 50u);
    } else if (e->kind == 3u) {
        score_add(g, g->double_fire ? 2000u : 1000u);
        g->double_fire = 1;
    } else
        score_add(g, e->kind == 5u ? 200u : 500u);
}
static void update_stars_particles(game_state *g) {
    unsigned star_speed = g->phase == GAME_WARP ? 1u + 7u * g->phase_ticks / GAME_WARP_TICKS
                                                : (g->phase == GAME_ARRIVAL ? 4u : 1u);
    for (unsigned i = 0; i < GAME_STARS; ++i) {
        game_star *s = &g->stars[i];
        s->radius = (uint16_t)(s->radius + (32u + (unsigned)s->speed * 20u) * star_speed);
        if (s->radius > 122u * 256u) {
            s->radius = 4u * 256u;
            s->angle = (uint16_t)random_word(g);
        }
    }
    for (unsigned i = 0; i < GAME_PARTICLES; ++i) {
        game_particle *p = &g->particles[i];
        if (p->life == 0u)
            continue;
        --p->life;
        p->x = (int16_t)(p->x + p->vx);
        p->y = (int16_t)(p->y + p->vy);
    }
}
static void update_enemies(game_state *g) {
    const bool chance = g->phase == GAME_CHANCE;
    for (unsigned i = 0; i < GAME_ENEMIES; ++i) {
        game_enemy *e = &g->enemies[i];
        if (!e->active)
            continue;
        ++e->age;
        if (e->kind == 3u || e->kind == 6u) {
            e->angle = add_angle(e->angle, e->angular_speed / 8);
            if (e->age > 360u)
                e->active = 0;
        } else if (e->kind == 4u || e->kind == 5u) {
            e->radius += e->kind == 4u ? 384 : 96;
            if (e->radius > 130 * 256)
                e->active = 0;
        } else if (chance || e->mode == 4u) {
            e->angle = add_angle(e->angle, e->angular_speed * 2);
            e->radius += e->age < 85u ? 96 : 200;
            if (e->radius > 130 * 256) {
                e->active = 0;
                ++g->escaped;
            }
        } else {
            if (e->mode == 0u) {
                e->radius += 64;
                e->angle = add_angle(e->angle, e->angular_speed);
                if (e->age >= 90u) {
                    e->mode = 1;
                    e->radius = (int32_t)(24u + (i % 3u) * 10u) * 256;
                }
            } else if (e->mode == 1u) {
                e->angle = add_angle(e->angle, e->angular_speed / 4);
                if (e->age > 150u && (e->age + i * 17u) % 170u == 0u)
                    e->mode = 2;
            } else if (e->mode == 2u) {
                const int16_t difference = (int16_t)(uint16_t)(g->ship_angle - e->angle);
                e->angle = add_angle(e->angle, difference / 24);
                e->radius += 256 + (int32_t)g->cycle * 10;
                if (e->radius > 126 * 256) {
                    e->mode = 3;
                    e->angle = add_angle(e->angle, 8192);
                }
            } else {
                e->radius -= 320;
                e->angle = add_angle(e->angle, e->angular_speed / 4);
                if (e->radius <= 32 * 256) {
                    e->mode = 1;
                    e->radius = 32 * 256;
                }
            }
            if (e->age > 780u) {
                e->active = 0;
                ++g->escaped;
                continue;
            }
            if (e->age > 100u && (e->age + i * 23u) % 240u == 0u)
                fire_shot(g->hostile, GAME_HOSTILE_SHOTS,
                          add_angle(g->ship_angle, (int32_t)(random_word(g) % 1601u) - 800),
                          e->radius);
        }
    }
}
static void update_player_shots(game_state *g) {
    for (unsigned i = 0; i < GAME_SHOTS; ++i) {
        game_shot *s = &g->shots[i];
        if (!s->active)
            continue;
        s->radius -= 3 * 256;
        ++s->age;
        if (s->radius < 4 * 256)
            s->active = 0;
    }
}
static void update_hostile_shots(game_state *g) {
    for (unsigned i = 0; i < GAME_HOSTILE_SHOTS; ++i) {
        game_shot *s = &g->hostile[i];
        if (!s->active)
            continue;
        s->radius += 256 + (int32_t)g->cycle * 10;
        ++s->age;
        if (s->radius > 130 * 256)
            s->active = 0;
    }
}
static bool beam_hits(game_state *g, const game_enemy *a, const game_enemy *b) {
    int16_t ax, ay, bx, by, px, py;
    game_polar(a->angle, a->radius, &ax, &ay);
    game_polar(b->angle, b->radius, &bx, &by);
    game_polar(g->ship_angle, 104 * 256, &px, &py);
    const int32_t dx = (int32_t)bx - ax, dy = (int32_t)by - ay, vx = (int32_t)px - ax,
                  vy = (int32_t)py - ay;
    const int32_t denom = dx * dx + dy * dy, dot = vx * dx + vy * dy;
    if (denom == 0 || dot < 0 || dot > denom)
        return false;
    const int64_t cross = (int64_t)vx * dy - (int64_t)vy * dx;
    return cross * cross <= (int64_t)25 * denom;
}
static void collisions(game_state *g) {
    /* The player moves every tick, including ticks without enemy movement. */
    for (unsigned i = 0; i < GAME_HOSTILE_SHOTS; ++i) {
        game_shot *shot = &g->hostile[i];
        if (shot->active && near_polar(shot->angle, shot->radius, g->ship_angle, 104 * 256, 6)) {
            shot->active = 0;
            die(g);
        }
    }
    for (unsigned i = 0; i < GAME_SHOTS; ++i) {
        game_shot *s = &g->shots[i];
        if (!s->active)
            continue;
        for (unsigned j = 0; j < GAME_ENEMIES; ++j) {
            game_enemy *e = &g->enemies[j];
            if (!e->active || e->kind == 4u)
                continue;
            if (near_polar(s->angle, s->radius, e->angle, e->radius, e->kind == 3u ? 8 : 7)) {
                s->active = 0;
                if (e->hp > 1u)
                    --e->hp;
                else
                    kill_enemy(g, e);
                break;
            }
        }
    }
    const game_enemy *beam = NULL;
    for (unsigned i = 0; i < GAME_ENEMIES; ++i) {
        const game_enemy *e = &g->enemies[i];
        if (!e->active)
            continue;
        if (e->kind == 5u) {
            if (beam != NULL && beam_hits(g, beam, e))
                die(g);
            beam = e;
        }
        /* Entrants begin near center; only outward attacks and hazards are dangerous. */
        if ((e->mode == 2u || e->kind >= 3u) &&
            near_polar(e->angle, e->radius, g->ship_angle, 104 * 256, 8))
            die(g);
    }
}
static void spawns(game_state *g) {
    const bool chance = g->phase == GAME_CHANCE;
    const unsigned gap = chance ? 8u : 12u;
    if (g->spawned < g->stage_target && g->enemy_ticks % gap == 0u) {
        const unsigned wave = g->spawned / 10u;
        uint16_t angle = (uint16_t)(wave * 16384u + (g->spawned % 10u) * 1300u + g->planet * 1800u);
        if (wave & 1u)
            angle = (uint16_t)(65535u - angle);
        if (spawn_enemy(g, (uint8_t)((wave + g->planet) % 3u), angle, 4 * 256, chance ? 4 : 0) !=
            NULL)
            ++g->spawned;
    }
    if (chance)
        return;
    if (g->enemy_ticks == 260u) {
        (void)spawn_enemy(g, 6, add_angle(g->ship_angle, -3000), 68 * 256, 1);
        (void)spawn_enemy(g, 3, g->ship_angle, 68 * 256, 1);
        (void)spawn_enemy(g, 6, add_angle(g->ship_angle, 3000), 68 * 256, 1);
    }
    if (g->stage >= 2u && g->enemy_ticks % 240u == 120u)
        (void)spawn_enemy(g, 4, add_angle(g->ship_angle, (int32_t)(random_word(g) % 4001u) - 2000),
                          0, 2);
    if (g->stage >= 6u && g->enemy_ticks % 480u == 300u) {
        (void)spawn_enemy(g, 5, add_angle(g->ship_angle, -5500), 0, 2);
        (void)spawn_enemy(g, 5, add_angle(g->ship_angle, 5500), 0, 2);
    }
}
static void next_stage(game_state *g) {
    (void)game_set_stage(g, g->stage >= 4095u ? 24u : (unsigned)g->stage + 1u);
    g->ship_angle = 32768;
}
void game_step(game_state *g, const game_input *input) {
    const game_input zero = {0};
    const game_input *in = input ? input : &zero;
    if (in->quit) {
        g->phase = GAME_TITLE;
        g->pause_settings = false;
        g->phase_ticks = 0;
        clear_entities(g);
        return;
    }
    if (g->phase == GAME_TITLE || g->phase == GAME_OVER) {
        if (g->pause_settings || in->menu_action == GAME_MENU_SETTINGS) {
            pause_action(g, in->menu_action);
            return;
        }
    }
    if (in->start && (g->phase == GAME_TITLE || g->phase == GAME_OVER)) {
        game_start(g);
        return;
    }
    if (in->pause && g->phase != GAME_TITLE && g->phase != GAME_OVER) {
        g->pause_settings = false;
        if (g->phase == GAME_PAUSED)
            g->phase = g->resume_phase;
        else {
            g->resume_phase = g->phase;
            g->phase = GAME_PAUSED;
        }
        return;
    }
    if (g->phase == GAME_PAUSED) {
        pause_action(g, in->menu_action);
        return;
    }
    ++g->tick;
    ++g->phase_ticks;
    update_stars_particles(g);
    if (g->phase == GAME_TITLE || g->phase == GAME_OVER)
        return;
    if (g->phase == GAME_CLEAR) {
        const unsigned duration = g->clear_chance ? GAME_CHANCE_RESULT_TICKS : GAME_CLEAR_TICKS;
        if (g->phase_ticks >= duration) {
            g->phase = GAME_WARP;
            g->phase_ticks = 0;
        }
        return;
    }
    if (g->phase == GAME_WARP) {
        if (g->phase_ticks >= GAME_WARP_TICKS) {
            if (!g->clear_chance && g->warp == 1u) {
                g->phase = GAME_ARRIVAL;
                g->phase_ticks = 0;
            } else
                next_stage(g);
        }
        return;
    }
    if (g->phase == GAME_ARRIVAL) {
        if (g->phase_ticks >= GAME_ARRIVAL_TICKS)
            next_stage(g);
        return;
    }
    g->ship_angle = add_angle(g->ship_angle, (int32_t)in->turn * 600 + in->angle_delta);
    if (g->invulnerable > 0u)
        --g->invulnerable;
    if (g->phase == GAME_INTRO) {
        if (g->phase_ticks >= 90u) {
            g->phase = g->warp == 0u ? GAME_CHANCE : GAME_PLAY;
            g->phase_ticks = 0;
        }
        return;
    }
    if (g->fire_cooldown > 0u)
        --g->fire_cooldown;
    if ((in->fire || g->autofire) && g->fire_cooldown == 0u) {
        fire_shot(g->shots, GAME_SHOTS, add_angle(g->ship_angle, g->double_fire ? -550 : 0),
                  100 * 256);
        if (g->double_fire)
            fire_shot(g->shots, GAME_SHOTS, add_angle(g->ship_angle, 550), 100 * 256);
        const unsigned rate = g->settings.fire_rate;
        g->fire_cooldown = (uint16_t)(GAME_TICK_HZ / rate);
        g->fire_rate_remainder = (uint8_t)(g->fire_rate_remainder + GAME_TICK_HZ % rate);
        if (g->fire_rate_remainder >= rate) {
            ++g->fire_cooldown;
            g->fire_rate_remainder = (uint8_t)(g->fire_rate_remainder - rate);
        }
    }
    /* A separate fractional clock keeps formations, lifetimes, hazards and
     * hostile fire coherent at25..200%, while the player stays at60Hz. */
    unsigned enemy_time = (unsigned)g->enemy_time_remainder + g->settings.enemy_speed_percent;
    unsigned enemy_steps = enemy_time / 100u;
    g->enemy_time_remainder = (uint8_t)(enemy_time % 100u);
    if (enemy_steps == 0u) {
        update_player_shots(g);
        collisions(g);
    } else
        for (unsigned step = 0; step < enemy_steps; ++step) {
            ++g->enemy_ticks;
            spawns(g);
            update_enemies(g);
            if (step == 0u)
                update_player_shots(g);
            update_hostile_shots(g);
            collisions(g);
            if (g->phase == GAME_OVER)
                break;
        }
    if (g->phase == GAME_OVER)
        return;
    bool any = false;
    for (unsigned i = 0; i < GAME_ENEMIES; ++i)
        if (g->enemies[i].active && g->enemies[i].kind <= 2u) {
            any = true;
            break;
        }
    if (g->spawned >= g->stage_target && !any) {
        g->clear_chance = g->phase == GAME_CHANCE;
        g->transition_bonus =
            g->clear_chance
                ? (g->destroyed == g->stage_target ? 10000u : (uint32_t)g->destroyed * 100u)
                : 0u;
        if (g->clear_chance)
            score_add(g, g->transition_bonus);
        g->phase = GAME_CLEAR;
        g->phase_ticks = 0;
        clear_entities(g);
    }
}
static void hash_word(uint32_t *hash, uint32_t word) {
    for (unsigned i = 0; i < 4u; ++i) {
        *hash ^= (word >> (i * 8u)) & 255u;
        *hash *= 16777619u;
    }
}
uint32_t game_state_hash(const game_state *g) {
    uint32_t h = 2166136261u;
#define H(v) hash_word(&h, (uint32_t)(v))
    H(g->tick);
    H(g->rng);
    H(g->score);
    H(g->high_score);
    H(g->stage);
    H(g->phase_ticks);
    H(g->ship_angle);
    H(g->fire_cooldown);
    H(g->invulnerable);
    H(g->spawned);
    H(g->destroyed);
    H(g->escaped);
    H(g->stage_target);
    H(g->lives);
    H(g->planet);
    H(g->warp);
    H(g->double_fire);
    H(g->cycle);
    H(g->phase);
    H(g->resume_phase);
    H(g->invincible);
    H(g->autofire);
    H(g->pause_settings);
    H(g->clear_chance);
    H(g->transition_bonus);
    H(g->settings.fire_rate);
    H(g->settings.enemy_speed_percent);
    H(g->settings.tilt_sensitivity_percent);
    H(g->fire_rate_remainder);
    H(g->enemy_time_remainder);
    H(g->enemy_ticks);
    for (unsigned i = 0; i < GAME_ENEMIES; ++i) {
        const game_enemy *e = &g->enemies[i];
        H(e->radius);
        H(e->angle);
        H(e->age);
        H(e->kind);
        H(e->mode);
        H(e->hp);
        H(e->active);
        H(e->angular_speed);
    }
    for (unsigned i = 0; i < GAME_SHOTS + GAME_HOSTILE_SHOTS; ++i) {
        const game_shot *s = i < GAME_SHOTS ? &g->shots[i] : &g->hostile[i - GAME_SHOTS];
        H(s->radius);
        H(s->angle);
        H(s->age);
        H(s->active);
    }
    for (unsigned i = 0; i < GAME_PARTICLES; ++i) {
        const game_particle *p = &g->particles[i];
        H(p->x);
        H(p->y);
        H(p->vx);
        H(p->vy);
        H(p->life);
        H(p->color);
    }
    for (unsigned i = 0; i < GAME_STARS; ++i) {
        const game_star *s = &g->stars[i];
        H(s->angle);
        H(s->radius);
        H(s->speed);
        H(s->color);
    }
#undef H
    return h;
}
