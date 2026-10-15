#include "gyrus/game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "FAIL %s:%d: %s\n", __func__, __LINE__, #c);                           \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)
static int initial(void) {
    game_state g;
    game_init(&g, 42);
    CHECK(g.phase == GAME_TITLE && g.lives == 3 && g.stage == 1);
    game_start(&g);
    CHECK(g.phase == GAME_INTRO && g.score == 0 && g.ship_angle == 32768);
    game_input in = {.quit = true};
    game_step(&g, &in);
    CHECK(g.phase == GAME_TITLE);
    return 0;
}
static int route(void) {
    game_state g;
    game_init(&g, 1);
    const unsigned stages[] = {1, 2, 3, 4, 7, 11, 19, 23, 24, 27, 47};
    const unsigned planets[] = {0, 0, 0, 1, 1, 2, 4, 5, 0, 0, 5};
    const unsigned warps[] = {2, 1, 0, 3, 0, 0, 0, 0, 3, 0, 0};
    for (unsigned i = 0; i < sizeof stages / sizeof stages[0]; ++i) {
        CHECK(game_set_stage(&g, stages[i]));
        CHECK(g.planet == planets[i] && g.warp == warps[i]);
    }
    CHECK(!game_set_stage(&g, 0));
    CHECK(!game_set_stage(&g, 65536));
    return 0;
}
static int replay(void) {
    game_state a, b;
    game_init(&a, 123);
    game_init(&b, 123);
    game_start(&a);
    game_start(&b);
    a.invincible = b.invincible = true;
    for (unsigned i = 0; i < 12000; ++i) {
        game_input in = {.turn = (i / 180u) % 2u ? -1 : 1, .fire = true};
        game_step(&a, &in);
        game_step(&b, &in);
        CHECK(game_state_hash(&a) == game_state_hash(&b));
        CHECK(a.lives > 0 && a.lives <= 9);
    }
    CHECK(a.stage >= 4);
    return 0;
}
static int chance(void) {
    game_state g;
    game_init(&g, 4);
    CHECK(game_set_stage(&g, 3));
    g.phase = GAME_CHANCE;
    g.phase_ticks = 0;
    g.invulnerable = 0;
    g.enemies[0] = (game_enemy){
        .radius = 104 * 256, .angle = g.ship_angle, .active = 1, .kind = 0, .hp = 1, .age = 200};
    const unsigned lives = g.lives;
    game_input in = {0};
    game_step(&g, &in);
    CHECK(g.lives == lives);
    return 0;
}
static int hits(void) {
    game_state g;
    game_init(&g, 4);
    game_start(&g);
    g.phase = GAME_PLAY;
    g.phase_ticks = 1;
    g.invulnerable = 0;
    g.enemies[0] = (game_enemy){
        .radius = 60 * 256, .angle = 0, .active = 1, .kind = 0, .hp = 1, .age = 120, .mode = 1};
    g.shots[0] = (game_shot){.radius = 63 * 256, .angle = 0, .active = 1};
    game_input in = {0};
    game_step(&g, &in);
    CHECK(!g.enemies[0].active && g.score > 0 && g.destroyed == 1);
    g.hostile[0] = (game_shot){.radius = 102 * 256, .angle = g.ship_angle, .active = 1};
    game_step(&g, &in);
    CHECK(g.lives == 2 && g.invulnerable > 0);
    game_step(&g, &in);
    CHECK(g.lives == 2);
    g.lives = 1;
    g.invulnerable = 0;
    g.hostile[0] = (game_shot){.radius = 102 * 256, .angle = g.ship_angle, .active = 1};
    game_step(&g, &in);
    CHECK(g.phase == GAME_OVER && g.lives == 0);
    return 0;
}
static int pause_test(void) {
    game_state g;
    game_init(&g, 1);
    game_start(&g);
    game_input in = {.pause = true};
    game_step(&g, &in);
    CHECK(g.phase == GAME_PAUSED);
    uint16_t angle = g.ship_angle;
    in = (game_input){.turn = 1, .fire = true};
    for (unsigned i = 0; i < 20; ++i)
        game_step(&g, &in);
    CHECK(g.ship_angle == angle);
    in = (game_input){.pause = true};
    game_step(&g, &in);
    CHECK(g.phase == GAME_INTRO);
    g.ship_angle = 65500;
    g.phase = GAME_PLAY;
    in = (game_input){.angle_delta = 100};
    game_step(&g, &in);
    CHECK(g.ship_angle == 64);
    return 0;
}
static int hazards(void) {
    game_state g;
    game_init(&g, 7);
    game_start(&g);
    g.phase = GAME_PLAY;
    g.phase_ticks = 1;
    g.invulnerable = 0;
    g.ship_angle = 0;
    /* A matched emitter pair makes a chord crossing the player's orbit. */
    g.enemies[0] =
        (game_enemy){.radius = 108 * 256, .angle = 3000, .active = 1, .kind = 5, .hp = 1};
    g.enemies[1] =
        (game_enemy){.radius = 108 * 256, .angle = 62536, .active = 1, .kind = 5, .hp = 1};
    game_step(&g, NULL);
    CHECK(g.lives == 2);
    /* A lone emitter cannot create an invisible residual beam. */
    g.invulnerable = 0;
    g.ship_angle = 0;
    g.enemies[1].active = 0;
    game_step(&g, NULL);
    CHECK(g.lives == 2);
    /* Meteors survive direct projectile hits. */
    g.enemies[0] =
        (game_enemy){.radius = 60 * 256, .angle = 16384, .active = 1, .kind = 4, .hp = 1};
    g.shots[0] = (game_shot){.radius = 65 * 256, .angle = 16384, .active = 1};
    game_step(&g, NULL);
    CHECK(g.enemies[0].active);
    return 0;
}
static int bonus(void) {
    game_state g;
    game_init(&g, 9);
    game_start(&g);
    g.phase = GAME_PLAY;
    g.phase_ticks = 1;
    g.score = 29500;
    g.enemies[0] = (game_enemy){.radius = 60 * 256, .angle = 0, .active = 1, .kind = 3, .hp = 1};
    g.shots[0] = (game_shot){.radius = 63 * 256, .angle = 0, .active = 1};
    game_step(&g, NULL);
    CHECK(g.double_fire && g.score == 30500 && g.lives == 4);
    CHECK(g.high_score == g.score);
    g.score = 99999990;
    g.enemies[0] = (game_enemy){.radius = 60 * 256, .angle = 0, .active = 1, .kind = 3, .hp = 1};
    g.shots[0] = (game_shot){.radius = 63 * 256, .angle = 0, .active = 1};
    game_step(&g, NULL);
    CHECK(g.score == 99999999);
    return 0;
}
static void finish_fixture(game_state *g, unsigned stage, unsigned destroyed) {
    game_init(g, 2040);
    (void)game_set_stage(g, stage);
    g->phase = g->warp == 0u ? GAME_CHANCE : GAME_PLAY;
    g->spawned = g->stage_target;
    g->destroyed = (uint16_t)destroyed;
    g->escaped = (uint16_t)(g->stage_target - destroyed);
    g->enemy_ticks = 501;
    g->autofire = false;
    g->double_fire = 1;
    g->ship_angle = 16384;
    game_step(g, NULL);
}
static int transitions(void) {
    game_state g;
    for (unsigned stage = 1; stage <= 2; ++stage) {
        finish_fixture(&g, stage, 39);
        CHECK(g.phase == GAME_CLEAR && !g.clear_chance && g.phase_ticks == 0);
        CHECK(g.stage == stage && g.double_fire && g.lives == 3);
        game_step(&g, &(game_input){.pause = true});
        CHECK(g.phase == GAME_PAUSED && g.resume_phase == GAME_CLEAR);
        game_step(&g, NULL);
        CHECK(g.phase_ticks == 0);
        game_step(&g, &(game_input){.pause = true});
        for (unsigned t = 0; t < GAME_CLEAR_TICKS; ++t)
            game_step(&g, &(game_input){.turn = 1, .fire = true});
        CHECK(g.phase == GAME_WARP && g.phase_ticks == 0 && g.ship_angle == 16384);
        for (unsigned t = 0; t < GAME_WARP_TICKS - 1; ++t)
            game_step(&g, NULL);
        CHECK(g.phase == GAME_WARP && g.stage == stage);
        game_step(&g, NULL);
        if (stage == 2) {
            CHECK(g.phase == GAME_ARRIVAL && g.phase_ticks == 0 && g.planet == 0);
            for (unsigned t = 0; t < GAME_ARRIVAL_TICKS; ++t)
                game_step(&g, NULL);
        }
        CHECK(g.phase == GAME_INTRO && g.stage == stage + 1 && g.phase_ticks == 0);
        CHECK(g.ship_angle == 32768 && g.double_fire && g.lives == 3);
        for (unsigned i = 0; i < GAME_SHOTS; ++i)
            CHECK(!g.shots[i].active && !g.hostile[i].active);
        for (unsigned t = 0; t < 90; ++t)
            game_step(&g, NULL);
        CHECK(g.phase == (stage == 2 ? GAME_CHANCE : GAME_PLAY));
    }
    for (unsigned destroyed = 39; destroyed <= 40; ++destroyed) {
        finish_fixture(&g, 3, destroyed);
        const unsigned bonus_score = destroyed == 40 ? 10000 : 3900;
        CHECK(g.phase == GAME_CLEAR && g.clear_chance);
        CHECK(g.score == bonus_score && g.transition_bonus == bonus_score);
        for (unsigned t = 0; t < GAME_CHANCE_RESULT_TICKS - 1; ++t)
            game_step(&g, NULL);
        CHECK(g.phase == GAME_CLEAR && g.score == bonus_score && g.stage == 3);
        game_step(&g, NULL);
        CHECK(g.phase == GAME_WARP && g.stage == 3 && g.score == bonus_score);
        for (unsigned t = 0; t < GAME_WARP_TICKS; ++t)
            game_step(&g, NULL);
        CHECK(g.phase == GAME_INTRO && g.stage == 4 && g.score == bonus_score);
        CHECK(!g.clear_chance && g.transition_bonus == 0);
    }
    return 0;
}
static int autofire_test(void) {
    game_state g;
    game_init(&g, 42);
    CHECK(g.autofire);
    game_start(&g);
    g.phase = GAME_PLAY;
    game_step(&g, NULL);
    CHECK(g.shots[0].active && g.fire_cooldown > 0);
    unsigned active = 0;
    for (unsigned i = 0; i < GAME_SHOTS; ++i)
        active += g.shots[i].active != 0;
    CHECK(active == 1);
    game_step(&g, NULL);
    CHECK(!g.shots[1].active);
    for (unsigned i = 0; i < 10; ++i)
        game_step(&g, NULL);
    CHECK(!g.shots[1].active);
    game_step(&g, NULL);
    CHECK(g.shots[1].active); /* Next volley after0.2seconds. */
    g.autofire = false;
    memset(g.shots, 0, sizeof g.shots);
    g.fire_cooldown = 0;
    game_step(&g, NULL);
    CHECK(!g.shots[0].active);
    game_input fire = {.fire = true};
    game_step(&g, &fire);
    CHECK(g.shots[0].active);
    return 0;
}
int main(int argc, char **argv) {
    if (argc != 2)
        return 2;
    if (strcmp(argv[1], "transitions") == 0)
        return transitions();
    if (strcmp(argv[1], "autofire") == 0)
        return autofire_test();
    if (strcmp(argv[1], "initial") == 0)
        return initial();
    if (strcmp(argv[1], "route") == 0)
        return route();
    if (strcmp(argv[1], "replay") == 0)
        return replay();
    if (strcmp(argv[1], "chance") == 0)
        return chance();
    if (strcmp(argv[1], "hits") == 0)
        return hits();
    if (strcmp(argv[1], "hazards") == 0)
        return hazards();
    if (strcmp(argv[1], "bonus") == 0)
        return bonus();
    if (strcmp(argv[1], "pause") == 0)
        return pause_test();
    return 2;
}
