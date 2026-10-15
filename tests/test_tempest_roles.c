#include "tempest/game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static tempest_state fresh(unsigned stage) {
    tempest_state s;
    tempest_init(&s, 54321);
    tempest_start(&s);
    assert(tempest_set_stage(&s, stage));
    s.spawned = s.stage_target;
    s.autofire = false;
    s.invulnerable = 0;
    return s;
}
static tempest_enemy foe(unsigned kind, unsigned lane, int depth) {
    return (tempest_enemy){.active = 1,
                           .landed = 1,
                           .hp = 1,
                           .kind = (uint8_t)kind,
                           .depth = depth,
                           .lane_q8 = (int16_t)(lane * 256),
                           .direction = 1};
}
static unsigned count(const tempest_state *s) {
    unsigned n = 0;
    for (unsigned i = 0; i < TEMPEST_ENEMIES; i++)
        n += s->enemies[i].active != 0;
    return n;
}
static void pursuit_and_contact(void) {
    engine_input still = {0};
    tempest_state s = fresh(1);
    /* Keep a deep waiting threat so the documented rim-only clear exception
       cannot end this controlled pursuit encounter prematurely. */
    s.enemies[1] = (tempest_enemy){.active = 1, .landed = 0, .timer = 60000};
    s.enemies[0] = foe(TEMPEST_FLIPPER, 5, TEMPEST_DEPTH);
    s.enemies[0].rim = 1;
    s.enemies[0].direction = 1;
    tempest_step(&s, &still);
    assert(s.enemies[0].direction == -1); /* Shortest closed-web route toward claw. */
    s = fresh(8);
    s.enemies[1] = (tempest_enemy){.active = 1, .timer = 60000};
    s.enemies[0] = foe(TEMPEST_FLIPPER, 8, TEMPEST_DEPTH);
    s.enemies[0].rim = 1;
    s.enemies[0].direction = 1;
    tempest_step(&s, &still);
    assert(s.enemies[0].direction == -1);
    /* A late pounce cannot be erased by a shot created after contact. */
    s = fresh(1);
    s.autofire = true;
    s.enemies[1] = (tempest_enemy){.active = 1, .timer = 60000};
    s.enemies[0] = foe(TEMPEST_FLIPPER, 1, TEMPEST_DEPTH);
    s.enemies[0].rim = 1;
    s.enemies[0].crossing = 1;
    s.enemies[0].direction = -1;
    s.enemies[0].timer = 176;
    tempest_state moving = s;
    for (unsigned i = 0; i < 6; i++)
        tempest_step(&s, &still);
    assert(s.phase == TEMPEST_DEATH && s.lives == 2);
    engine_input evade = {.turn = -1};
    for (unsigned i = 0; i < 6; i++)
        tempest_step(&moving, &evade);
    assert(moving.phase == TEMPEST_PLAY && moving.lives == 3);
    /* Flat rim enemies resist inward fire; an upright flip remains shootable. */
    s = fresh(1);
    s.enemies[1] = (tempest_enemy){.active = 1, .timer = 60000};
    s.enemies[0] = foe(TEMPEST_FLIPPER, 1, TEMPEST_DEPTH);
    s.enemies[0].rim = 1;
    s.enemies[0].crossing = 1;
    s.enemies[0].direction = -1;
    s.enemies[0].timer = 32;
    s.shots[0] = (tempest_shot){TEMPEST_DEPTH, 0, 1};
    tempest_step(&s, &still);
    assert(s.enemies[0].active && s.score == 0);
    s.enemies[0].timer = 112;
    s.shots[0] = (tempest_shot){TEMPEST_DEPTH, 0, 1};
    tempest_step(&s, &still);
    assert(!s.enemies[0].active && s.score == 150);
    s = fresh(11);
    s.autofire = true;
    s.enemies[0] = foe(TEMPEST_FUSEBALL, 1, 63000);
    tempest_step(&s, &still);
    assert(s.lives == 2); /* Rail touches claw's adjacent edge. */
}
static void stationary_replay_pressure(void) {
    unsigned damaged = 0;
    for (unsigned seed = 1; seed <= 64; seed++) {
        tempest_state s;
        tempest_init(&s, seed);
        assert(tempest_set_stage(&s, 1));
        for (unsigned t = 0; t < 2500 && s.stage == 1; t++) {
            tempest_step(&s, NULL);
            if (s.phase == TEMPEST_DEATH || s.phase == TEMPEST_OVER) {
                damaged++;
                break;
            }
        }
    }
    /* Natural first-wave attacks must defeat stationary autofire in most seeded
       encounters. This is a pressure regression, not an every-seed death rule. */
    assert(damaged >= 48);
    tempest_state empty;
    tempest_init(&empty, 1);
    assert(tempest_set_stage(&empty, 1));
    empty.spawn_timer = 60000;
    for (unsigned t = 0; t < 1200; t++)
        tempest_step(&empty, NULL);
    assert(empty.phase == TEMPEST_PLAY && empty.lives == 3); /* No inactivity penalty. */
}
int main(void) {
    pursuit_and_contact();
    stationary_replay_pressure();
    engine_input in = {0};
    tempest_state s = fresh(3);
    s.enemies[0] = foe(TEMPEST_TANKER, 2, 20000);
    s.enemies[0].payload = TEMPEST_FLIPPER;
    s.shots[0] = (tempest_shot){22000, 2, 1};
    tempest_step(&s, &in);
    assert(s.score == 100 && count(&s) == 2);
    for (unsigned i = 0; i < 2; i++)
        assert(s.enemies[i].kind == TEMPEST_FLIPPER);
    s = fresh(41);
    s.enemies[0] = foe(TEMPEST_TANKER, 2, 59999);
    s.enemies[0].payload = TEMPEST_PULSAR;
    tempest_step(&s, &in);
    assert(count(&s) == 2 && s.enemies[0].kind == TEMPEST_PULSAR &&
           s.enemies[1].kind == TEMPEST_PULSAR);
    s = fresh(11);
    s.enemies[0] = foe(TEMPEST_FUSEBALL, 2, 20000);
    s.shots[0] = (tempest_shot){22000, 2, 1};
    tempest_step(&s, &in);
    assert(s.enemies[0].active && s.score == 0);
    s.enemies[0].crossing = 1;
    s.enemies[0].timer = 60;
    s.shots[0] = (tempest_shot){22000, 2, 1};
    tempest_step(&s, &in);
    assert(!s.enemies[0].active && s.score == 250);
    s = fresh(1);
    s.enemies[0] = foe(TEMPEST_FLIPPER, 0, TEMPEST_DEPTH);
    s.enemies[0].rim = 1;
    tempest_step(&s, &in);
    assert(s.lives == 2 && s.phase == TEMPEST_DEATH);
    s = fresh(4);
    s.enemies[0] = foe(TEMPEST_SPIKER, 3, 30000);
    tempest_step(&s, &in);
    assert(s.spikes[3] > 30000);
    int depth = s.spikes[3];
    s.enemies[0].active = 0;
    s.shots[0] = (tempest_shot){depth + 100, 3, 1};
    tempest_step(&s, &in);
    assert(s.spikes[3] < depth && s.spikes[3] > 0);
    s = fresh(1);
    s.score = 19950;
    s.enemies[0] = foe(TEMPEST_FLIPPER, 3, 20000);
    s.shots[0] = (tempest_shot){22000, 3, 1};
    tempest_step(&s, &in);
    assert(s.score == 20100 && s.lives == 4 && s.next_bonus == 40000);
    s = fresh(1);
    s.enemies[0] = foe(TEMPEST_FLIPPER, 4, TEMPEST_DEPTH);
    s.enemies[0].rim = 1;
    tempest_step(&s, &in);
    assert(s.phase == TEMPEST_CLEAR);
    s = fresh(1);
    s.hostile[0] = (tempest_shot){20000, 2, 1};
    s.shots[0] = (tempest_shot){22000, 2, 1};
    tempest_step(&s, &in);
    assert(!s.hostile[0].active && !s.shots[0].active); /* fire can intercept enemy shots */
    /* Pause is a complete simulation freeze and resumes its exact dive depth. */
    s = fresh(4);
    s.phase = TEMPEST_DIVE;
    s.dive_depth = 50000;
    in.pause = true;
    tempest_step(&s, &in);
    assert(s.phase == TEMPEST_PAUSED);
    in = (engine_input){0};
    uint32_t h = tempest_hash(&s);
    for (unsigned i = 0; i < 100; i++)
        tempest_step(&s, &in);
    assert(h == tempest_hash(&s));
    in.pause = true;
    tempest_step(&s, &in);
    assert(s.phase == TEMPEST_DIVE && s.dive_depth == 50000);
    /* Every spawned family respects its introduction stage across seeded schedules. */
    for (unsigned stage = 1; stage <= 20; stage++) {
        s = fresh(stage);
        s.invincible = true;
        s.spawned = 0;
        s.stage_target = 100;
        for (unsigned tick = 0; tick < 2000; tick++) {
            s.spawn_timer = 0;
            in = (engine_input){0};
            tempest_step(&s, &in);
            for (unsigned i = 0; i < TEMPEST_ENEMIES; i++)
                if (s.enemies[i].active) {
                    unsigned k = s.enemies[i].kind;
                    assert(k != TEMPEST_TANKER || stage >= 3);
                    assert(k != TEMPEST_SPIKER || stage >= 4);
                    assert(k != TEMPEST_FUSEBALL || stage >= 11);
                    assert(k != TEMPEST_PULSAR || stage >= 20);
                }
        }
    }
    /* Pausing a death must not overwrite its distinct gameplay continuation. */
    s = fresh(1);
    s.enemies[0] = foe(TEMPEST_FLIPPER, 0, TEMPEST_DEPTH);
    s.enemies[0].rim = 1;
    in = (engine_input){0};
    tempest_step(&s, &in);
    assert(s.phase == TEMPEST_DEATH);
    s.enemies[1] = foe(TEMPEST_FLIPPER, 4, 1000);
    for (unsigned i = 0; i < 20; i++)
        tempest_step(&s, &in);
    uint16_t death_ticks = s.phase_ticks;
    in.pause = true;
    tempest_step(&s, &in);
    in = (engine_input){0};
    for (unsigned i = 0; i < 30; i++)
        tempest_step(&s, &in);
    assert(s.phase_ticks == death_ticks);
    in.pause = true;
    tempest_step(&s, &in);
    assert(s.phase == TEMPEST_DEATH && s.phase_ticks == death_ticks);
    in = (engine_input){0};
    for (unsigned i = 0; i < 75u - death_ticks; i++)
        tempest_step(&s, &in);
    assert(s.phase == TEMPEST_PLAY && s.lives == 2);
    s = fresh(4);
    s.phase = TEMPEST_DIVE;
    s.dive_depth = 30000;
    s.spikes[0] = 30000;
    tempest_step(&s, &in);
    assert(s.phase == TEMPEST_DEATH);
    for (unsigned i = 0; i < 20; i++)
        tempest_step(&s, &in);
    in.pause = true;
    tempest_step(&s, &in);
    in = (engine_input){0};
    for (unsigned i = 0; i < 30; i++)
        tempest_step(&s, &in);
    in.pause = true;
    tempest_step(&s, &in);
    in = (engine_input){0};
    for (unsigned i = 0; i < 175; i++)
        tempest_step(&s, &in);
    assert(s.stage == 5 && s.lives == 2 && s.phase == TEMPEST_PLAY);
    puts("Tempest enemy roles and collision contracts passed");
    return 0;
}
