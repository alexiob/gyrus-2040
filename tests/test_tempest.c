#include "tempest/game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static tempest_state fresh(unsigned stage) {
    tempest_state s;
    tempest_init(&s, 1234);
    tempest_start(&s);
    assert(tempest_set_stage(&s, stage));
    s.spawned = s.stage_target;
    s.spawn_timer = 60000;
    return s;
}
static unsigned shots(const tempest_state *s) {
    unsigned n = 0;
    for (unsigned i = 0; i < TEMPEST_SHOTS; i++)
        n += s->shots[i].active != 0;
    return n;
}
static void entry_exit_feedback(void) {
    tempest_state s;
    tempest_init(&s, 1234);
    tempest_start(&s);
    assert(s.phase == TEMPEST_INTRO && s.phase_ticks == 0);
    engine_input in = {0};
    for (unsigned i = 0; i < TEMPEST_INTRO_TICKS - 1; i++)
        tempest_step(&s, &in);
    assert(s.phase == TEMPEST_INTRO && s.spawned == 0 && shots(&s) == 0);
    in.pause = true;
    tempest_step(&s, &in);
    assert(s.phase == TEMPEST_PAUSED);
    uint16_t frozen = s.phase_ticks;
    in = (engine_input){0};
    for (unsigned i = 0; i < 10; i++)
        tempest_step(&s, &in);
    assert(s.phase_ticks == frozen);
    in.pause = true;
    tempest_step(&s, &in);
    assert(s.phase == TEMPEST_INTRO);
    in = (engine_input){0};
    tempest_step(&s, &in);
    assert(s.phase == TEMPEST_PLAY);
    assert(tempest_set_stage(&s, 4) && s.phase == TEMPEST_PLAY); /* Debug remains immediate. */
    s.phase = TEMPEST_FLIGHT;
    s.phase_ticks = 59;
    tempest_step(&s, &in);
    assert(s.stage == 5 && s.phase == TEMPEST_INTRO);
    for (unsigned i = 0; i < TEMPEST_INTRO_TICKS; i++)
        tempest_step(&s, &in);
    assert(s.phase == TEMPEST_PLAY && s.spawned == 0);
    s.enemies[0] = (tempest_enemy){
        .active = 1, .landed = 1, .kind = TEMPEST_FLIPPER, .depth = 20000, .lane_q8 = 512};
    s.enemies[1] = (tempest_enemy){.active = 1, .landed = 0, .timer = 60};
    in.special = true;
    tempest_step(&s, &in);
    assert(s.zap_flash > 0 && s.zaps == 1 && !s.enemies[0].active);
    unsigned events = 0;
    for (unsigned i = 0; i < TEMPEST_VISUAL_EVENTS; i++)
        events += s.visual[i].life != 0;
    assert(events >= 2); /* Target burst and global zap feedback. */
    uint32_t h = tempest_hash(&s);
    tempest_state copy = s;
    memset(copy.visual, 0, sizeof copy.visual);
    copy.zap_flash = 0;
    copy.visual_cursor = 7;
    assert(h == tempest_hash(&copy));
    in = (engine_input){.pause = true};
    tempest_step(&s, &in);
    copy = s;
    in = (engine_input){0};
    for (unsigned i = 0; i < 30; i++)
        tempest_step(&s, &in);
    assert(!memcmp(s.visual, copy.visual, sizeof s.visual) && s.zap_flash == copy.zap_flash);
}
int main(void) {
    entry_exit_feedback();
    tempest_state s = fresh(1);
    const tempest_web *w = tempest_current_web(&s);
    assert(w->closed && tempest_neighbor(w, 0, -1) == w->lane_count - 1);
    s = fresh(8);
    w = tempest_current_web(&s);
    assert(!w->closed && tempest_neighbor(w, 0, -1) == -1);
    assert(tempest_neighbor(w, w->lane_count - 1, 1) == -1);
    s.claw_q8 = 0;
    engine_input in = {.turn = -1};
    tempest_step(&s, &in);
    assert(s.claw_q8 == 0);
    s = fresh(1);
    s.settings.fire_rate = 10;
    s.enemies[0] = (tempest_enemy){.active = 1, .landed = 0, .timer = 60000};
    for (unsigned i = 0; i < 120; i++) {
        in = (engine_input){.fire = true};
        tempest_step(&s, &in);
        assert(shots(&s) <= 8);
    }
    assert(s.phase == TEMPEST_PLAY);
    /* First zap excludes spikes, enemy bullets and waiting enemies; tanker payload vanishes. */
    s = fresh(20);
    s.spikes[2] = 20000;
    s.hostile[0] = (tempest_shot){1000, 2, 1};
    s.enemies[0] = (tempest_enemy){
        .active = 1, .landed = 1, .hp = 1, .kind = TEMPEST_TANKER, .depth = 20000, .lane_q8 = 256};
    s.enemies[1] = (tempest_enemy){.active = 1, .landed = 0, .timer = 100, .kind = TEMPEST_FLIPPER};
    in = (engine_input){.special = true};
    tempest_step(&s, &in);
    assert(s.zaps == 1 && !s.enemies[0].active && s.enemies[1].active);
    assert(s.hostile[0].active && s.spikes[2] == 20000 && s.score == 100);
    /* Each true special is an already-debounced one-tick engine pulse. */
    for (unsigned i = 2; i < 5; i++)
        s.enemies[i] = (tempest_enemy){.active = 1,
                                       .landed = 1,
                                       .hp = 1,
                                       .kind = TEMPEST_FLIPPER,
                                       .depth = 20000,
                                       .lane_q8 = (int16_t)(i * 256)};
    in.special = true;
    tempest_step(&s, &in);
    assert(s.zaps == 0);
    unsigned active = 0;
    for (unsigned i = 2; i < 5; i++)
        active += s.enemies[i].active;
    assert(active == 2);
    /* A pulse is dangerous even when the enemy is deep. */
    s = fresh(20);
    s.invulnerable = 0;
    s.claw_q8 = 3 * 256;
    s.enemies[0] = (tempest_enemy){.active = 1,
                                   .landed = 1,
                                   .hp = 1,
                                   .kind = TEMPEST_PULSAR,
                                   .depth = 1000,
                                   .lane_q8 = 3 * 256,
                                   .age = 119};
    in = (engine_input){0};
    tempest_step(&s, &in);
    assert(s.lives == 2);
    /* A surviving spike kills on dive; death must not double advance. */
    s = fresh(4);
    s.phase = TEMPEST_DIVE;
    s.dive_depth = 50000;
    s.spikes[0] = 50000;
    s.claw_q8 = 0;
    s.invulnerable = 0;
    tempest_step(&s, &in);
    assert(s.lives == 2 && s.phase == TEMPEST_DEATH);
    for (unsigned i = 0; i < 180; i++)
        tempest_step(&s, &in);
    assert(s.stage == 5);
    /* Effects and UI are excluded, all gameplay state remains identical. */
    tempest_state a = fresh(40), b = a;
    b.effects = true;
    b.ui.tilt_preview = 999;
    for (unsigned i = 0; i < 900; i++) {
        in = (engine_input){.turn = (i / 80) % 2 ? 1 : -1, .fire = true, .special = i == 200};
        tempest_step(&a, &in);
        tempest_step(&b, &in);
        assert(tempest_hash(&a) == tempest_hash(&b));
    }
    uint32_t h = tempest_hash(&a);
    assert(!tempest_set_stage(&a, 0));
    assert(!tempest_set_stage(&a, 4096));
    assert(h == tempest_hash(&a));
    puts("Tempest classic contracts passed");
    return 0;
}
