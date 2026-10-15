#include "tempest/game.h"
#include <limits.h>
#include <string.h>
static uint32_t random_word(tempest_state *s) {
    uint32_t x = s->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return s->rng = x;
}
const char *tempest_phase_name(tempest_phase p) {
    static const char *const names[] = {"title", "play",   "clear",     "dive", "flight",
                                        "death", "paused", "game_over", "intro"};
    return (unsigned)p < 9 ? names[p] : "invalid";
}
static void visual_event(tempest_state *s, tempest_visual_kind kind, int lane_q8, int32_t depth) {
    static const uint8_t lifetime[] = {5, 22, 24, 14, 18, 38, 20};
    tempest_visual_event *v = &s->visual[s->visual_cursor];
    *v = (tempest_visual_event){.depth = depth,
                                .lane_q8 = (int16_t)lane_q8,
                                .age = 0,
                                .life = lifetime[kind],
                                .kind = (uint8_t)kind};
    s->visual_cursor = (uint8_t)((s->visual_cursor + 1u) % TEMPEST_VISUAL_EVENTS);
}
static void visual_step(tempest_state *s) {
    if (s->zap_flash)
        s->zap_flash--;
    for (unsigned i = 0; i < TEMPEST_VISUAL_EVENTS; i++) {
        tempest_visual_event *v = &s->visual[i];
        if (v->life && ++v->age >= v->life)
            v->life = 0;
    }
}
static unsigned claw_lane(const tempest_state *s) {
    return (unsigned)s->claw_q8 / 256u;
}
static void award(tempest_state *s, unsigned points) {
    if (UINT32_MAX - s->score < points)
        s->score = UINT32_MAX;
    else
        s->score += points;
    if (s->score > s->high_score)
        s->high_score = s->score;
    while (s->score >= s->next_bonus && s->next_bonus) {
        if (s->lives < 5)
            s->lives++;
        if (UINT32_MAX - s->next_bonus < 20000u) {
            s->next_bonus = 0;
            break;
        }
        s->next_bonus += 20000;
    }
}
void tempest_init(tempest_state *s, uint32_t seed) {
    memset(s, 0, sizeof *s);
    s->rng = seed ? seed : 0x19812040u;
    s->stage = 1;
    s->lives = 3;
    s->zaps = 2;
    s->next_bonus = 20000;
    s->autofire = true;
    s->settings = (game_settings){5, 100, 100};
    s->phase = TEMPEST_TITLE;
}
bool tempest_set_stage(tempest_state *s, unsigned level) {
    if (!s || level < 1 || level > 4095)
        return false;
    s->stage = (uint16_t)level;
    s->phase = TEMPEST_PLAY;
    s->phase_ticks = 0;
    s->settings_open = false;
    s->claw_q8 = 0;
    s->dive_depth = TEMPEST_DEPTH;
    s->zaps = 2;
    s->fire_cooldown = 0;
    s->fire_remainder = 0;
    s->spawned = 0;
    s->destroyed = 0;
    s->escaped = 0;
    s->spawn_timer = 60;
    s->invulnerable = 90;
    unsigned difficulty = level > 81 ? 81 : level;
    s->stage_target = (uint16_t)(10 + difficulty / 2);
    memset(s->enemies, 0, sizeof s->enemies);
    memset(s->shots, 0, sizeof s->shots);
    memset(s->hostile, 0, sizeof s->hostile);
    memset(s->spikes, 0, sizeof s->spikes);
    memset(s->visual, 0, sizeof s->visual);
    s->visual_cursor = 0;
    s->zap_flash = 0;
    return true;
}
void tempest_start(tempest_state *s) {
    uint32_t high = s->high_score, rng = s->rng;
    game_settings settings = s->settings;
    bool effects = s->effects, invincible = s->invincible, autofire = s->autofire;
    engine_ui ui = s->ui;
    tempest_init(s, rng);
    s->high_score = high;
    s->settings = settings;
    s->effects = effects;
    s->ui = ui;
    s->invincible = invincible;
    s->autofire = autofire;
    (void)tempest_set_stage(s, 1);
    s->phase = TEMPEST_INTRO;
}
static int free_enemy(tempest_state *s) {
    for (unsigned i = 0; i < TEMPEST_ENEMIES; i++)
        if (!s->enemies[i].active)
            return (int)i;
    return -1;
}
static void add_enemy(tempest_state *s, unsigned kind, unsigned lane, int32_t depth, bool landed) {
    int i = free_enemy(s);
    if (i < 0)
        return;
    tempest_enemy *e = &s->enemies[i];
    *e = (tempest_enemy){.depth = depth,
                         .timer = 30,
                         .lane_q8 = (int16_t)(lane * 256),
                         .direction = (random_word(s) & 1u) ? 1 : -1,
                         .kind = (uint8_t)kind,
                         .active = 1,
                         .landed = (uint8_t)landed,
                         .hp = 1};
    if (kind == TEMPEST_TANKER)
        e->payload = (uint8_t)(s->stage >= 41   ? TEMPEST_PULSAR
                               : s->stage >= 33 ? TEMPEST_FUSEBALL
                                                : TEMPEST_FLIPPER);
}
static unsigned enemy_points(const tempest_enemy *e) {
    static const unsigned points[] = {150, 100, 50, 250, 200};
    if (e->kind == TEMPEST_FUSEBALL)
        return e->depth > 45000 ? 750 : e->depth > 22000 ? 500 : 250;
    return points[e->kind < 5 ? e->kind : 0];
}
static void kill_enemy(tempest_state *s, unsigned i, bool split) {
    tempest_enemy e = s->enemies[i];
    if (!e.active)
        return;
    s->enemies[i].active = 0;
    visual_event(s, split && e.kind == TEMPEST_TANKER ? TEMPEST_FX_SPLIT : TEMPEST_FX_HIT,
                 e.lane_q8 + 128, e.depth);
    s->destroyed++;
    award(s, enemy_points(&e));
    if (split && e.kind == TEMPEST_TANKER) {
        const tempest_web *w = tempest_current_web(s);
        unsigned lane = (unsigned)e.lane_q8 / 256;
        int adjacent = tempest_neighbor(w, (int)lane, e.direction);
        if (adjacent < 0)
            adjacent = (int)lane;
        add_enemy(s, e.payload, lane, e.depth, true);
        add_enemy(s, e.payload, (unsigned)adjacent, e.depth, true);
    }
}
static void die(tempest_state *s, bool diving) {
    if (s->invincible || s->invulnerable || s->phase == TEMPEST_DEATH)
        return;
    visual_event(s, TEMPEST_FX_DEATH, s->claw_q8 + 128, diving ? s->dive_depth : TEMPEST_DEPTH);
    if (s->lives)
        s->lives--;
    s->resume_phase = diving ? TEMPEST_FLIGHT : TEMPEST_PLAY;
    s->phase = s->lives ? TEMPEST_DEATH : TEMPEST_OVER;
    s->phase_ticks = 0;
    memset(s->shots, 0, sizeof s->shots);
    memset(s->hostile, 0, sizeof s->hostile);
    /* Respawn cleanup removes only immediate rim threats; deep enemies persist. */
    for (unsigned i = 0; i < TEMPEST_ENEMIES; i++)
        if (s->enemies[i].active && s->enemies[i].depth > 46000)
            s->enemies[i].active = 0;
}
static void move_claw(tempest_state *s, const engine_input *in) {
    const tempest_web *w = tempest_current_web(s);
    int32_t delta = (int32_t)in->turn * 46;
    delta += (int32_t)in->angle_delta * w->lane_count * 256 / 65536;
    if (delta > 384)
        delta = 384;
    if (delta < -384)
        delta = -384;
    s->claw_q8 += delta;
    int32_t max = (int32_t)w->lane_count * 256;
    if (w->closed) {
        s->claw_q8 %= max;
        if (s->claw_q8 < 0)
            s->claw_q8 += max;
    } else {
        if (s->claw_q8 < 0)
            s->claw_q8 = 0;
        if (s->claw_q8 >= max)
            s->claw_q8 = max - 1;
    }
}
static void fire(tempest_state *s) {
    if (s->fire_cooldown)
        return;
    for (unsigned i = 0; i < TEMPEST_SHOTS; i++)
        if (!s->shots[i].active) {
            s->shots[i] = (tempest_shot){TEMPEST_DEPTH, (uint8_t)claw_lane(s), 1};
            visual_event(s, TEMPEST_FX_MUZZLE, s->claw_q8 + 128, TEMPEST_DEPTH - 5000);
            unsigned rate = s->settings.fire_rate ? s->settings.fire_rate : 5;
            unsigned interval = 60 / rate;
            s->fire_remainder = (uint8_t)(s->fire_remainder + 60 % rate);
            if (s->fire_remainder >= rate) {
                s->fire_remainder = (uint8_t)(s->fire_remainder - rate);
                interval++;
            }
            s->fire_cooldown = (uint16_t)interval;
            break;
        }
}
static void zap(tempest_state *s) {
    if (!s->zaps)
        return;
    if (s->zaps == 2) {
        for (unsigned i = 0; i < TEMPEST_ENEMIES; i++)
            if (s->enemies[i].active && s->enemies[i].landed)
                kill_enemy(s, i, false);
    } else {
        unsigned count = 0;
        for (unsigned i = 0; i < TEMPEST_ENEMIES; i++)
            count += s->enemies[i].active && s->enemies[i].landed;
        if (count) {
            unsigned chosen = random_word(s) % count;
            for (unsigned i = 0; i < TEMPEST_ENEMIES; i++)
                if (s->enemies[i].active && s->enemies[i].landed) {
                    if (!chosen) {
                        kill_enemy(s, i, false);
                        break;
                    }
                    chosen--;
                }
        }
    }
    s->zaps--;
    s->zap_flash = 18;
    visual_event(s, TEMPEST_FX_ZAP, s->claw_q8 + 128, TEMPEST_DEPTH);
}
static void spawn(tempest_state *s) {
    if (s->spawned >= s->stage_target)
        return;
    if (s->spawn_timer) {
        s->spawn_timer--;
        return;
    }
    if (free_enemy(s) < 0)
        return;
    unsigned r = random_word(s) % 12u, kind = TEMPEST_FLIPPER;
    if (s->stage >= 20 && r == 0)
        kind = TEMPEST_PULSAR;
    else if (s->stage >= 11 && r < 3)
        kind = TEMPEST_FUSEBALL;
    else if (s->stage >= 4 && r < 5)
        kind = TEMPEST_SPIKER;
    else if (s->stage >= 3 && r < 8)
        kind = TEMPEST_TANKER;
    add_enemy(s, kind, random_word(s) % tempest_current_web(s)->lane_count, 0, false);
    s->spawned++;
    unsigned d = s->stage > 81 ? 81 : s->stage;
    s->spawn_timer = (uint16_t)((52 - d / 2) * 100 / s->settings.enemy_speed_percent);
    if (s->spawn_timer < 5)
        s->spawn_timer = 5;
}
static void enemy_fire(tempest_state *s, const tempest_enemy *e) {
    for (unsigned i = 0; i < TEMPEST_HOSTILE; i++)
        if (!s->hostile[i].active) {
            s->hostile[i] = (tempest_shot){e->depth, (uint8_t)(e->lane_q8 / 256), 1};
            return;
        }
}
/* The manual's rim attacks and seeking enemies require deliberate pursuit.
 * Direction changes only between flips; open webs never take a wrap shortcut. */
static int pursuit_direction(const tempest_state *s, const tempest_enemy *e) {
    const tempest_web *w = tempest_current_web(s);
    int from = e->lane_q8 / 256, target = (int)claw_lane(s), delta = target - from;
    if (!delta)
        return 0;
    if (w->closed) {
        int n = w->lane_count;
        if (delta > n / 2)
            delta -= n;
        else if (delta < -n / 2)
            delta += n;
    }
    return delta > 0 ? 1 : -1;
}
static bool enemy_contacts_player(const tempest_state *s, const tempest_enemy *e) {
    if (!e->active || !e->landed)
        return false;
    const tempest_web *w = tempest_current_web(s);
    int lane = e->lane_q8 / 256, player = (int)claw_lane(s);
    int next = tempest_neighbor(w, lane, e->direction);
    /* The electrified lane and its rendered boundary rails remain at the
     * source until a flip completes and lane_q8 advances. */
    if (e->kind == TEMPEST_PULSAR && e->age % 150u >= 120u && lane == player)
        return true;
    if (e->depth < 62000)
        return false;
    if (e->kind == TEMPEST_FUSEBALL) {
        if (e->crossing)
            return (e->direction > 0 ? lane : tempest_neighbor(w, lane, -1)) == player;
        return lane == player || tempest_neighbor(w, lane, -1) == player;
    }
    if (e->crossing)
        return (e->timer < 64 && lane == player) || (e->timer >= 192 && next == player);
    return lane == player;
}
static bool player_contacts_enemies(tempest_state *s) {
    for (unsigned i = 0; i < TEMPEST_ENEMIES; i++)
        if (enemy_contacts_player(s, &s->enemies[i])) {
            die(s, false);
            if (s->phase != TEMPEST_PLAY)
                return true;
        }
    return false;
}
static bool flip_step(tempest_state *s, tempest_enemy *e, int rate) {
    const tempest_web *w = tempest_current_web(s);
    if (!e->crossing) {
        if (e->rim || e->kind == TEMPEST_FUSEBALL) {
            int direction = pursuit_direction(s, e);
            if (!direction)
                return false;
            e->direction = (int8_t)direction;
        }
        int next = tempest_neighbor(w, e->lane_q8 / 256, e->direction);
        if (next < 0) {
            e->direction = (int8_t)-e->direction;
            next = tempest_neighbor(w, e->lane_q8 / 256, e->direction);
        }
        if (next < 0)
            return false;
        e->crossing = 1;
        e->timer = 0;
    }
    e->timer = (uint16_t)(e->timer + rate);
    if (e->timer >= 256) {
        int next = tempest_neighbor(w, e->lane_q8 / 256, e->direction);
        if (next >= 0)
            e->lane_q8 = (int16_t)(next * 256);
        e->crossing = 0;
        e->timer = 0;
        return true;
    }
    return false;
}
static void enemies_step(tempest_state *s) {
    unsigned d = s->stage > 81 ? 81 : s->stage;
    int speed = (int)((140 + d * 5) * s->settings.enemy_speed_percent / 100);
    for (unsigned i = 0; i < TEMPEST_ENEMIES; i++) {
        tempest_enemy *e = &s->enemies[i];
        if (!e->active)
            continue;
        if (!e->landed) {
            if (e->timer)
                e->timer--;
            else {
                e->landed = 1;
                e->timer = 0;
                visual_event(s, TEMPEST_FX_LAND, e->lane_q8 + 128, 0);
            }
            continue;
        }
        e->age++;
        unsigned lane = (unsigned)e->lane_q8 / 256;
        if (e->kind == TEMPEST_SPIKER) {
            if (!e->retreat) {
                e->depth += speed;
                if (e->depth > s->spikes[lane])
                    s->spikes[lane] = e->depth;
                if (e->depth >= 40000 + (int32_t)((i * 1237u + s->stage * 157u) % 17000u))
                    e->retreat = 1;
            } else {
                e->depth -= speed * 2;
                if (e->depth <= 0) {
                    e->active = 0;
                    s->escaped++;
                }
            }
        } else if (e->kind == TEMPEST_FUSEBALL) {
            e->depth += e->retreat ? -speed * 2 : speed * 2;
            if (e->depth > 64000) {
                e->depth = 64000;
                e->retreat = 1;
            }
            if (e->depth < 8000) {
                e->depth = 8000;
                e->retreat = 0;
            }
            if (e->crossing || e->age % 75u == 0)
                (void)flip_step(s, e, 12);
        } else {
            if (!e->rim)
                e->depth += e->kind == TEMPEST_TANKER ? speed * 3 / 4 : speed;
            if (e->kind == TEMPEST_TANKER && e->depth >= 60000) {
                tempest_enemy copy = *e;
                visual_event(s, TEMPEST_FX_SPLIT, e->lane_q8 + 128, e->depth);
                e->active = 0;
                int next = tempest_neighbor(tempest_current_web(s), (int)lane, e->direction);
                add_enemy(s, copy.payload, lane, copy.depth, true);
                add_enemy(s, copy.payload, (unsigned)(next < 0 ? (int)lane : next), copy.depth,
                          true);
                continue;
            }
            if (e->depth >= TEMPEST_DEPTH) {
                e->depth = TEMPEST_DEPTH;
                e->rim = 1;
            }
            if (e->kind == TEMPEST_FLIPPER || e->kind == TEMPEST_PULSAR) {
                if (e->crossing || e->rim || (s->stage > 1 && e->age % 140u == 0))
                    (void)flip_step(s, e, e->rim ? 12 : 8);
            }
        }
        if (!e->active)
            continue;
        lane = (unsigned)e->lane_q8 / 256;
        if (e->kind != TEMPEST_SPIKER && e->kind != TEMPEST_FUSEBALL && e->age % 200u == 0 &&
            (!e->rim))
            enemy_fire(s, e);
        if (e->kind == TEMPEST_PULSAR && e->age % 150u == 120u)
            visual_event(s, TEMPEST_FX_PULSE, e->lane_q8 + 128, e->depth);
        if (enemy_contacts_player(s, e))
            die(s, false);
        if (s->phase != TEMPEST_PLAY)
            return;
    }
}
static void shots_step(tempest_state *s) {
    const int step = 2700;
    for (unsigned i = 0; i < TEMPEST_SHOTS; i++) {
        tempest_shot *b = &s->shots[i];
        if (!b->active)
            continue;
        int32_t old = b->depth;
        b->depth -= step;
        /* Select nearest hit along the swept segment, rather than array order. */
        int target = -1;
        int32_t nearest = -1;
        for (unsigned j = 0; j < TEMPEST_ENEMIES; j++) {
            tempest_enemy *e = &s->enemies[j];
            if (!e->active || !e->landed)
                continue;
            unsigned lane = (unsigned)e->lane_q8 / 256;
            if (e->kind == TEMPEST_FUSEBALL && !e->crossing)
                continue;
            /* A rim Flipper is exposed only while standing upright mid-flip.
             * Flat/pouncing rim enemies cannot be retroactively shot through
             * the muzzle. A jump crosses one lane, not two full lane hitboxes. */
            if (e->rim && (!e->crossing || e->timer < 64 || e->timer > 160))
                continue;
            bool same = lane == b->lane;
            if (e->kind == TEMPEST_FUSEBALL) {
                int crossed = e->direction > 0
                                  ? (int)lane
                                  : tempest_neighbor(tempest_current_web(s), (int)lane, -1);
                same = crossed == (int)b->lane;
            } else if (e->crossing) {
                int next = tempest_neighbor(tempest_current_web(s), (int)lane, e->direction);
                if (e->timer >= 64 && e->timer <= 160)
                    same = same || next == (int)b->lane;
                else if (e->timer > 160)
                    same = next == (int)b->lane;
            }
            if (same && e->depth <= old + 1800 && e->depth >= b->depth - 1800 &&
                e->depth > nearest) {
                target = (int)j;
                nearest = e->depth;
            }
        }
        int hostile_target = -1;
        int32_t hostile_depth = -1;
        for (unsigned j = 0; j < TEMPEST_HOSTILE; j++) {
            tempest_shot *enemy_shot = &s->hostile[j];
            if (enemy_shot->active && enemy_shot->lane == b->lane &&
                enemy_shot->depth <= old + 1800 && enemy_shot->depth >= b->depth - 1800 &&
                enemy_shot->depth > hostile_depth) {
                hostile_target = (int)j;
                hostile_depth = enemy_shot->depth;
            }
        }
        if (hostile_target >= 0 && hostile_depth > nearest && hostile_depth >= s->spikes[b->lane]) {
            s->hostile[hostile_target].active = 0;
            b->active = 0;
            continue;
        }
        if (target >= 0 && nearest >= s->spikes[b->lane]) {
            kill_enemy(s, (unsigned)target, true);
            b->active = 0;
            continue;
        }
        if (s->spikes[b->lane] > 0 && b->depth <= s->spikes[b->lane]) {
            s->spikes[b->lane] -= 2600;
            if (s->spikes[b->lane] < 0)
                s->spikes[b->lane] = 0;
            award(s, 2);
            visual_event(s, TEMPEST_FX_HIT, b->lane * 256 + 128, s->spikes[b->lane]);
            b->active = 0;
        } else if (b->depth < 0)
            b->active = 0;
    }
    for (unsigned i = 0; i < TEMPEST_HOSTILE; i++) {
        tempest_shot *b = &s->hostile[i];
        if (!b->active)
            continue;
        b->depth += 1000 * s->settings.enemy_speed_percent / 100;
        if (b->depth >= TEMPEST_DEPTH) {
            b->active = 0;
            if (b->lane == claw_lane(s))
                die(s, false);
        }
    }
}
static bool combat_done(const tempest_state *s) {
    if (s->spawned < s->stage_target)
        return false;
    for (unsigned i = 0; i < TEMPEST_ENEMIES; i++)
        if (s->enemies[i].active && !(s->enemies[i].kind == TEMPEST_FLIPPER && s->enemies[i].rim))
            return false;
    return true;
}
void tempest_step(tempest_state *s, const engine_input *input) {
    const engine_input empty = {0};
    const engine_input *in = input ? input : &empty;
    /* Engine input is already a debounced one-tick action, including G1 pulses. */
    bool special = in->special;
    if (s->phase == TEMPEST_PAUSED) {
        if (s->settings_open) {
            if (in->menu_action == GAME_MENU_BACK)
                s->settings_open = false;
            else
                (void)engine_settings_menu(&s->settings, in->menu_action);
        } else if (in->menu_action == GAME_MENU_SETTINGS)
            s->settings_open = true;
        else if (in->pause || in->start) {
            s->phase = s->pause_phase;
        } else if (in->quit) {
            s->phase = TEMPEST_TITLE;
            s->settings_open = false;
        }
        return;
    }
    if (s->phase == TEMPEST_TITLE || s->phase == TEMPEST_OVER) {
        s->tick++;
        if (s->settings_open) {
            if (in->menu_action == GAME_MENU_BACK)
                s->settings_open = false;
            else
                (void)engine_settings_menu(&s->settings, in->menu_action);
        } else if (in->menu_action == GAME_MENU_SETTINGS)
            s->settings_open = true;
        else if (in->start)
            tempest_start(s);
        return;
    }
    if (in->pause) {
        s->pause_phase = s->phase;
        s->phase = TEMPEST_PAUSED;
        return;
    }
    s->tick++;
    s->phase_ticks++;
    visual_step(s);
    if (s->phase == TEMPEST_INTRO) {
        if (s->phase_ticks >= TEMPEST_INTRO_TICKS) {
            s->phase = TEMPEST_PLAY;
            s->phase_ticks = 0;
        }
        return;
    }
    if (s->phase == TEMPEST_DEATH) {
        if (s->phase_ticks >= 75) {
            s->phase = s->resume_phase;
            s->phase_ticks = 0;
            s->invulnerable = 120;
            if (s->phase == TEMPEST_FLIGHT)
                s->phase_ticks = 0;
        }
        return;
    }
    if (s->phase == TEMPEST_FLIGHT) {
        if (s->phase_ticks >= 60) {
            unsigned next = s->stage == 4095 ? 1 : s->stage + 1u;
            (void)tempest_set_stage(s, next);
            s->phase = TEMPEST_INTRO;
        }
        return;
    }
    if (s->phase == TEMPEST_CLEAR) {
        if (s->phase_ticks >= 30) {
            s->phase = TEMPEST_DIVE;
            s->phase_ticks = 0;
            s->dive_depth = TEMPEST_DEPTH;
            s->invulnerable = 0;
        }
        return;
    }
    if (s->invulnerable)
        s->invulnerable--;
    move_claw(s, in);
    /* Resolve existing contact before creating a new inward shot or Zapper.
     * The next enemy movement is checked again after its flip advances. */
    if (s->phase == TEMPEST_PLAY && player_contacts_enemies(s))
        return;
    if (s->fire_cooldown)
        s->fire_cooldown--;
    if (in->fire || s->autofire)
        fire(s);
    if (s->phase == TEMPEST_DIVE) {
        s->dive_depth -= 900;
        shots_step(s);
        if (s->phase == TEMPEST_DEATH || s->phase == TEMPEST_OVER) {
            s->resume_phase = TEMPEST_FLIGHT;
            return;
        }
        if (s->spikes[claw_lane(s)] >= s->dive_depth && s->spikes[claw_lane(s)] > 0) {
            die(s, true);
            if (s->phase == TEMPEST_DEATH || s->phase == TEMPEST_OVER)
                return;
        }
        if (s->dive_depth <= 0) {
            award(s, 1000);
            s->phase = TEMPEST_FLIGHT;
            s->phase_ticks = 0;
        }
        return;
    }
    s->play_tick++;
    spawn(s);
    if (special)
        zap(s);
    shots_step(s);
    if (s->phase != TEMPEST_PLAY)
        return;
    enemies_step(s);
    if (s->phase == TEMPEST_PLAY && combat_done(s)) {
        s->phase = TEMPEST_CLEAR;
        s->phase_ticks = 0;
    }
}
/* Explicit field-wise FNV-1a. No padding, UI telemetry or cosmetic option. */
static uint32_t hash_word(uint32_t h, uint32_t v) {
    for (unsigned i = 0; i < 4; i++) {
        h ^= (v >> (i * 8)) & 255u;
        h *= 16777619u;
    }
    return h;
}
uint32_t tempest_hash(const tempest_state *s) {
    uint32_t h = 2166136261u;
#define H(v) h = hash_word(h, (uint32_t)(v))
    H(s->tick);
    H(s->play_tick);
    H(s->rng);
    H(s->score);
    H(s->high_score);
    H(s->next_bonus);
    H(s->stage);
    H(s->phase_ticks);
    H(s->phase);
    H(s->resume_phase);
    H(s->pause_phase);
    H(s->fire_cooldown);
    H(s->spawn_timer);
    H(s->invulnerable);
    H(s->spawned);
    H(s->stage_target);
    H(s->destroyed);
    H(s->escaped);
    H(s->claw_q8);
    H(s->dive_depth);
    H(s->settings.fire_rate);
    H(s->settings.enemy_speed_percent);
    H(s->settings.tilt_sensitivity_percent);
    H(s->invincible);
    H(s->autofire);
    H(s->settings_open);
    H(s->lives);
    H(s->zaps);
    H(s->fire_remainder);
    for (unsigned i = 0; i < TEMPEST_ENEMIES; i++) {
        const tempest_enemy *e = &s->enemies[i];
        H(e->active);
        if (!e->active)
            continue;
        H(e->depth);
        H(e->age);
        H(e->timer);
        H(e->lane_q8);
        H(e->direction);
        H(e->kind);
        H(e->landed);
        H(e->hp);
        H(e->rim);
        H(e->crossing);
        H(e->retreat);
        H(e->payload);
    }
    for (unsigned i = 0; i < TEMPEST_SHOTS; i++) {
        H(s->shots[i].active);
        if (s->shots[i].active) {
            H(s->shots[i].depth);
            H(s->shots[i].lane);
        }
    }
    for (unsigned i = 0; i < TEMPEST_HOSTILE; i++) {
        H(s->hostile[i].active);
        if (s->hostile[i].active) {
            H(s->hostile[i].depth);
            H(s->hostile[i].lane);
        }
    }
    for (unsigned i = 0; i < TEMPEST_RAILS; i++)
        H(s->spikes[i]);
#undef H
    return h;
}
