#include "gyrus/game.h"
#include "engine/math.h"
#include "games/gyrus/waves.h"
#include <limits.h>
#include <string.h>

static uint16_t add_angle(uint16_t a, int32_t delta) {
    return (uint16_t)((int32_t)a + delta);
}
int16_t game_sin_q15(uint16_t angle) {
    return engine_sin_q15(angle);
}
int16_t game_cos_q15(uint16_t angle) {
    return engine_cos_q15(angle);
}
void game_polar(uint16_t angle, int32_t radius, int16_t *x, int16_t *y) {
    engine_polar(angle, radius, x, y);
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
    g->combat_ticks = 0;
    g->final_escape_ticks = 0;
    g->fifth_spawn_tick = 0;
    g->entrance_complete = false;
    g->fifth_decided = false;
    memset(g->entrance_kills, 0, sizeof g->entrance_kills);
    g->fifth_path_planet = 0;
    g->fifth_path_formation = 0;
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
    const uint8_t old_rate = g->settings.fire_rate;
    if (!engine_settings_set(&g->settings, setting, value))
        return false;
    if (g->settings.fire_rate != old_rate) {
        g->fire_cooldown = 0;
        g->fire_rate_remainder = 0;
    }
    return true;
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
bool game_debug_complete_stage(game_state *g) {
    if (!g || (g->phase != GAME_PLAY && g->phase != GAME_CHANCE))
        return false;
    for (unsigned i = 0; i < GAME_ENEMIES; ++i)
        if (g->enemies[i].kind <= 2u)
            g->enemies[i].active = 0;
    g->spawned = g->stage_target;
    g->destroyed = g->stage_target;
    g->escaped = 0;
    g->entrance_complete = true;
    g->fifth_decided = true;
    return true;
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
        if (g->phase == GAME_PLAY && e->mode == GAME_ENEMY_ENTER && e->formation < 5u &&
            g->entrance_kills[e->formation] < 10u) {
            ++g->entrance_kills[e->formation];
            if (g->entrance_kills[e->formation] == 10u)
                score_add(g, 1000u + (uint32_t)e->formation * 500u);
        }
        score_add(g, g->phase == GAME_CHANCE        ? 100u
                     : e->mode == GAME_ENEMY_ESCAPE ? 1000u
                                                    : 100u + (uint32_t)e->kind * 50u);
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
static void enemy_mode(game_enemy *e, game_enemy_mode mode) {
    e->mode = (uint8_t)mode;
    e->age = 0;
}
static int32_t angle_difference(uint16_t target, uint16_t current) {
    const uint16_t delta = (uint16_t)(target - current);
    return delta <= 32767u ? (int32_t)delta : (int32_t)delta - 65536;
}
static void update_enemies(game_state *g) {
    const bool chance = g->phase == GAME_CHANCE;
    unsigned survivors = 0;
    bool entering = false;
    for (unsigned i = 0; i < GAME_ENEMIES; ++i) {
        const game_enemy *e = &g->enemies[i];
        if (e->active && e->kind <= 2u) {
            ++survivors;
            entering |= e->mode == GAME_ENEMY_ENTER;
        }
    }
    if (!chance && !g->fifth_decided && g->spawned >= 40u && !entering) {
        unsigned initial_kills = 0;
        for (unsigned formation = 0; formation < 4u; ++formation)
            initial_kills += g->entrance_kills[formation];
        g->fifth_decided = true;
        /* JoyStik confirms a fifth/last reinforcement when the formation is
         * depleted. The >3/4 trigger is an inferred contemporary-guide rule;
         * this decision clock is authored, not a ROM-exact reconstruction. */
        if (initial_kills > 30u) {
            const uint32_t choice = random_word(g);
            g->fifth_path_planet = (uint8_t)(choice % 5u);
            g->fifth_path_formation = (uint8_t)((choice / 5u) % 4u);
            g->stage_target = 50;
            g->fifth_spawn_tick = g->enemy_ticks;
        }
    }
    if (!chance && g->fifth_decided && !g->entrance_complete && g->spawned >= g->stage_target &&
        !entering)
        g->entrance_complete = true;
    if (g->entrance_complete && !chance) {
        g->combat_ticks = (uint16_t)((g->combat_ticks + 1u) % 960u);
        if (survivors > 0u && survivors <= 2u) {
            if (g->final_escape_ticks < 360u)
                ++g->final_escape_ticks;
        } else
            g->final_escape_ticks = 0;
    }
    for (unsigned i = 0; i < GAME_ENEMIES; ++i) {
        game_enemy *e = &g->enemies[i];
        if (!e->active)
            continue;
        if (e->age < UINT16_MAX)
            ++e->age;
        if (e->kind == 3u || e->kind == 6u) {
            e->angle = add_angle(e->angle, e->angular_speed / 8);
            if (e->age > 360u)
                e->active = 0;
        } else if (e->kind == 4u || e->kind == 5u) {
            e->radius += e->kind == 4u ? 384 : 96;
            if (e->radius > 130 * 256)
                e->active = 0;
        } else if (chance || e->mode == GAME_ENEMY_ESCAPE) {
            e->angle = add_angle(e->angle, e->angular_speed * 2);
            e->radius += chance && e->age < 85u ? 96 : 200;
            if (e->radius > 130 * 256) {
                e->active = 0;
                ++g->escaped;
            }
        } else {
            uint16_t home_angle;
            int32_t home_radius;
            gyrus_wave_home(g->planet, e->formation, e->member, g->combat_ticks, &home_angle,
                            &home_radius);
            if (e->mode == GAME_ENEMY_ENTER) {
                gyrus_wave_entry(g->planet, e->formation, e->member,
                                 e->formation == 4u ? g->fifth_path_planet : g->planet,
                                 e->formation == 4u ? g->fifth_path_formation : e->formation,
                                 e->age, &e->angle, &e->radius);
                if (e->age >= GYRUS_ENTRY_TICKS) {
                    /* Contemporary JoyStik explicitly distinguishes Mars:
                     * each ship peels from entry separately. Other destinations
                     * assemble the four groups before their combined attack. */
                    enemy_mode(e, g->planet == 4u ? GAME_ENEMY_ATTACK : GAME_ENEMY_HOLD);
                }
            } else if (e->mode == GAME_ENEMY_HOLD) {
                /* Approach the moving berth smoothly; never depend on pool index. */
                e->angle = add_angle(e->angle, angle_difference(home_angle, e->angle) / 6);
                if (g->entrance_complete) {
                    const unsigned identity = (unsigned)e->formation * 10u + e->member;
                    if (g->final_escape_ticks >= 360u && e->attacks > 0u)
                        enemy_mode(e, GAME_ENEMY_ESCAPE);
                    else if (g->combat_ticks >= 60u &&
                             g->combat_ticks % 240u == (60u + identity * 17u) % 240u)
                        enemy_mode(e, GAME_ENEMY_ATTACK);
                }
            } else if (e->mode == GAME_ENEMY_ATTACK) {
                /* Three authored silhouettes follow the original's independent
                 * circling and sharper/gentler steering roles. Sprite colors
                 * remain our original art; these are not ROM trajectories. */
                if (e->kind == 0u)
                    e->angle = add_angle(e->angle, e->angular_speed);
                else {
                    const int32_t steer =
                        angle_difference(g->ship_angle, e->angle) / (e->kind == 1u ? 24 : 40);
                    const int32_t weave = e->kind == 1u ? 150 : 65;
                    e->angle = add_angle(e->angle, steer + ((e->age / 24u) & 1u ? weave : -weave));
                }
                e->radius += 256 + (int32_t)g->cycle * 10;
                if (e->radius > 130 * 256)
                    enemy_mode(e, GAME_ENEMY_OUTSIDE_RETURN);
            } else if (e->mode == GAME_ENEMY_OUTSIDE_RETURN) {
                /* Leave the circular viewport, curve around, and reappear. The
                 * enemy remains alive and counts toward completion throughout. */
                e->angle = add_angle(e->angle, e->angular_speed * 2);
                if (e->age >= 30u)
                    enemy_mode(e, GAME_ENEMY_RETURN);
            } else if (e->mode == GAME_ENEMY_RETURN) {
                e->radius -= 320;
                e->angle = add_angle(e->angle, angle_difference(home_angle, e->angle) / 8);
                if (e->radius <= home_radius) {
                    e->radius = home_radius;
                    if (e->attacks < UINT8_MAX)
                        ++e->attacks;
                    enemy_mode(e, GAME_ENEMY_HOLD);
                }
            }
            if ((g->entrance_complete || g->planet == 4u) && e->mode == GAME_ENEMY_ATTACK &&
                e->age % 60u == 30u)
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
    const unsigned formation = g->spawned / 10u;
    const unsigned ordinal = g->spawned % 10u;
    /* Interleave the two branches:0,5,1,6,... Each pair launches on
     * consecutive bounded updates rather than waiting for the other train. */
    const unsigned path_planet = formation == 4u ? g->fifth_path_planet : g->planet;
    const bool single_file = path_planet == 2u || path_planet == 4u;
    const unsigned member = chance || single_file ? ordinal : ordinal / 2u + (ordinal % 2u) * 5u;
    const unsigned launch = single_file ? ordinal : ordinal / 2u;
    /* Reinforcement timing starts when it is selected. Initial survivors may
     * settle after the old absolute fifth-group deadline has already passed. */
    const unsigned group_start =
        formation == 4u ? g->fifth_spawn_tick : formation * GYRUS_GROUP_TICKS;
    const unsigned scheduled =
        chance ? (unsigned)g->spawned * 8u + 8u : group_start + launch * GYRUS_MEMBER_TICKS + 1u;
    if (g->spawned < g->stage_target && g->enemy_ticks >= scheduled) {
        uint16_t angle;
        int32_t radius;
        if (chance) {
            angle = (uint16_t)(formation * 16384u + member * 1300u + g->planet * 1800u);
            radius = 4 * 256;
        } else
            gyrus_wave_entry(g->planet, (uint8_t)formation, (uint8_t)member,
                             formation == 4u ? g->fifth_path_planet : g->planet,
                             formation == 4u ? g->fifth_path_formation : (uint8_t)formation, 0,
                             &angle, &radius);
        game_enemy *e = spawn_enemy(g, (uint8_t)((formation + g->planet) % 3u), angle, radius,
                                    chance ? GAME_ENEMY_ESCAPE : GAME_ENEMY_ENTER);
        if (e) {
            const int16_t speed = (int16_t)(170u + (unsigned)g->cycle * 12u + e->kind * 12u);
            e->formation = (uint8_t)formation;
            e->member = (uint8_t)member;
            e->angular_speed = formation & 1u ? (int16_t)-speed : speed;
            ++g->spawned;
        }
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
    if (g->phase_ticks < UINT16_MAX)
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
            /* A common multiple of hazard periods preserves cadence indefinitely
             * without wrapping into the one-shot entrance/satellite schedule. */
            if (g->enemy_ticks >= 28800u)
                g->enemy_ticks = 14400u;
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
    /* Completion requires an accounted resolution for every scheduled plane.
     * A lost/inactive entity can never silently turn into a successful clear. */
    if (g->spawned == g->stage_target && !any &&
        (g->phase == GAME_CHANCE || (g->entrance_complete && g->escaped <= 2u)) &&
        (unsigned)g->destroyed + g->escaped == g->stage_target) {
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
    H(g->combat_ticks);
    H(g->final_escape_ticks);
    H(g->fifth_spawn_tick);
    H(g->entrance_complete);
    H(g->fifth_decided);
    H(g->fifth_path_planet);
    H(g->fifth_path_formation);
    for (unsigned formation = 0; formation < 5u; ++formation)
        H(g->entrance_kills[formation]);
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
        H(e->formation);
        H(e->member);
        H(e->attacks);
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
