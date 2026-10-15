#include "../src/games/gyrus/waves.h"
#include "gyrus/game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void combat(game_state *g) {
    game_init(g, 1983);
    game_start(g);
    g->phase = GAME_PLAY;
    g->invincible = true;
    g->autofire = false;
}
static unsigned ordinary(const game_state *g) {
    unsigned count = 0;
    for (unsigned i = 0; i < GAME_ENEMIES; ++i)
        count += g->enemies[i].active && g->enemies[i].kind <= 2u;
    return count;
}
static void no_fire_preserves_survivors(void) {
    game_state g;
    combat(&g);
    for (unsigned t = 0; t < 100000; ++t) {
        game_step(&g, NULL);
        assert(g.phase == GAME_PLAY);
        assert(g.escaped == 0 && g.destroyed == 0);
        if (t > 1500u)
            assert(ordinary(&g) == 40);
    }
}
static void four_formations_before_attack(void) {
    game_state g;
    combat(&g);
    bool attacked = false;
    for (unsigned t = 0; t < 2000; ++t) {
        game_step(&g, NULL);
        for (unsigned i = 0; i < GAME_ENEMIES; ++i) {
            const game_enemy *e = &g.enemies[i];
            if (!e->active || e->kind > 2u || e->mode != 2u)
                continue;
            assert(g.spawned == 40);
            for (unsigned j = 0; j < GAME_ENEMIES; ++j)
                assert(!g.enemies[j].active || g.enemies[j].kind > 2u || g.enemies[j].mode != 0u);
            attacked = true;
        }
    }
    assert(attacked);
}
static void offscreen_attacker_returns(void) {
    game_state g;
    combat(&g);
    g.spawned = 40;
    g.enemies[0] = (game_enemy){
        .active = 1, .kind = 0, .hp = 1, .mode = 2, .radius = 126 * 256, .angle = 0, .age = 779};
    /* Three survivors cannot use the special last-two escape. */
    g.enemies[1] = (game_enemy){.active = 1, .kind = 0, .hp = 1, .mode = 1, .radius = 30 * 256};
    g.enemies[2] = g.enemies[1];
    bool outside = false, returned = false;
    for (unsigned t = 0; t < 350; ++t) {
        game_step(&g, NULL);
        assert(g.enemies[0].active && g.escaped == 0 && g.phase == GAME_PLAY);
        outside |= g.enemies[0].radius > 126 * 256;
        returned |= outside && g.enemies[0].radius < 50 * 256;
    }
    assert(outside && returned);
}
static void chance_flights_exit(void) {
    game_state g;
    combat(&g);
    assert(game_set_stage(&g, 3));
    g.phase = GAME_CHANCE;
    g.invulnerable = 0;
    for (unsigned t = 0; t < 2000 && g.phase == GAME_CHANCE; ++t)
        game_step(&g, NULL);
    assert(g.phase == GAME_CLEAR && g.clear_chance);
    assert(g.stage_target == 40 && g.spawned == 40 && g.escaped == 40 && g.destroyed == 0);
    assert(g.lives == 3 && g.transition_bonus == 0);
}
static const game_enemy *member_at(const game_state *g, unsigned formation, unsigned member,
                                   unsigned *slot) {
    for (unsigned i = 0; i < GAME_ENEMIES; ++i)
        if (g->enemies[i].active && g->enemies[i].kind <= 2u &&
            g->enemies[i].formation == formation && g->enemies[i].member == member) {
            *slot = i;
            return &g->enemies[i];
        }
    return NULL;
}
static void recycled_pool_does_not_change_path(void) {
    game_state a, b;
    combat(&a);
    combat(&b);
    game_step(&a, NULL);
    game_step(&b, NULL);
    b.enemies[0].active = 0; /* Reuse a killed member's pool slot. */
    b.destroyed = 1;
    bool compared = false;
    for (unsigned t = 0; t < 180; ++t) {
        game_step(&a, NULL);
        game_step(&b, NULL);
        unsigned slot_a = 0, slot_b = 0;
        const game_enemy *ea = member_at(&a, 0, 7, &slot_a);
        const game_enemy *eb = member_at(&b, 0, 7, &slot_b);
        if (!ea || !eb)
            continue;
        assert(slot_a != slot_b);
        assert(ea->angle == eb->angle && ea->radius == eb->radius && ea->mode == eb->mode);
        compared = true;
    }
    assert(compared);
}
static void last_two_get_a_sortie_and_escape(void) {
    game_state g;
    combat(&g);
    g.spawned = 40;
    g.destroyed = 38;
    for (unsigned i = 0; i < 2; ++i)
        g.enemies[i] = (game_enemy){.active = 1,
                                    .hp = 1,
                                    .kind = 0,
                                    .mode = GAME_ENEMY_HOLD,
                                    .radius = 30 * 256,
                                    .member = (uint8_t)i,
                                    .angular_speed = 170};
    bool fleeing = false;
    for (unsigned t = 0; t < 1600 && g.phase == GAME_PLAY; ++t) {
        game_step(&g, NULL);
        if (t < 300)
            assert(g.escaped == 0);
        for (unsigned i = 0; i < 2; ++i)
            if (g.enemies[i].active && g.enemies[i].mode == GAME_ENEMY_ESCAPE) {
                assert(g.enemies[i].attacks > 0);
                fleeing = true;
            }
    }
    assert(fleeing && g.phase == GAME_CLEAR);
    assert(g.destroyed == 38 && g.escaped == 2 && g.score == 0);
}
static void fleeing_enemy_is_worth_one_thousand(void) {
    game_state g;
    combat(&g);
    g.spawned = 40;
    g.destroyed = 39;
    g.enemies[0] = (game_enemy){
        .active = 1, .hp = 1, .kind = 2, .mode = GAME_ENEMY_ESCAPE, .radius = 60 * 256, .angle = 0};
    g.shots[0] = (game_shot){.active = 1, .radius = 63 * 256, .angle = 0};
    game_step(&g, NULL);
    assert(g.phase == GAME_CLEAR && g.destroyed == 40 && g.escaped == 0);
    assert(g.score == 1000);
}
static void pause_freezes_wave_lifecycle(void) {
    game_state g;
    combat(&g);
    for (unsigned t = 0; t < 1100; ++t)
        game_step(&g, NULL);
    const game_state before = g;
    game_step(&g, &(game_input){.pause = true});
    for (unsigned t = 0; t < 100; ++t)
        game_step(&g, NULL);
    assert(g.enemy_ticks == before.enemy_ticks && g.combat_ticks == before.combat_ticks);
    assert(g.final_escape_ticks == before.final_escape_ticks);
    assert(memcmp(g.enemies, before.enemies, sizeof g.enemies) == 0);
    game_step(&g, &(game_input){.pause = true});
    assert(g.phase == before.phase && g.tick == before.tick && g.phase_ticks == before.phase_ticks);
    assert(g.score == before.score && g.spawned == before.spawned && g.escaped == before.escaped);
    assert(memcmp(g.enemies, before.enemies, sizeof g.enemies) == 0);
}
static void missing_entities_do_not_complete_combat(void) {
    game_state g;
    combat(&g);
    g.spawned = 40;
    g.destroyed = 39;
    for (unsigned t = 0; t < 1000; ++t)
        game_step(&g, NULL);
    assert(g.phase == GAME_PLAY && g.destroyed == 39 && g.escaped == 0);
}
static void shoot_entrants(game_state *g, unsigned limit) {
    unsigned shot = 0;
    memset(g->shots, 0, sizeof g->shots);
    for (unsigned i = 0; i < GAME_ENEMIES && shot < limit; ++i) {
        const game_enemy *e = &g->enemies[i];
        if (!e->active || e->kind > 2u || e->mode != GAME_ENEMY_ENTER)
            continue;
        g->shots[shot++] =
            (game_shot){.active = 1, .radius = e->radius + 3 * 256, .angle = e->angle};
    }
}
static void perfect_entrances_require_fifth_group(void) {
    game_state g;
    combat(&g);
    assert(game_set_setting(&g, GAME_SETTING_ENEMY_SPEED, 25));
    for (unsigned t = 0; t < 5000 && g.spawned < 50; ++t) {
        shoot_entrants(&g, 48);
        game_step(&g, NULL);
        assert(g.phase == GAME_PLAY);
    }
    assert(g.stage_target == 50 && g.spawned == 50);
    for (unsigned t = 0; t < 1000 && g.phase == GAME_PLAY; ++t) {
        shoot_entrants(&g, 48);
        game_step(&g, NULL);
    }
    assert(g.phase == GAME_CLEAR && g.destroyed == 50 && g.escaped == 0);
    /* Per-plane100/150/200 plus five whole-entrance awards1000..3000. */
    assert(g.score == 17000);
    assert(g.fifth_decided && g.entrance_kills[4] == 10);
}
static void kill_stationary_plane(game_state *g, unsigned formation, unsigned member,
                                  game_enemy_mode mode) {
    g->enemies[0] = (game_enemy){.active = 1,
                                 .kind = 0,
                                 .hp = 1,
                                 .formation = (uint8_t)formation,
                                 .member = (uint8_t)member,
                                 .mode = (uint8_t)mode,
                                 .radius = 60 * 256};
    g->shots[0] = (game_shot){.active = 1, .radius = 63 * 256};
    /* Isolate the shot on an idle enemy-clock tick: the player still moves60Hz. */
    g->enemy_time_remainder = 0;
    game_step(g, NULL);
    assert(!g->enemies[0].active);
}
static void whole_formation_bonus_requires_ten_entry_kills(void) {
    game_state g;
    combat(&g);
    assert(game_set_setting(&g, GAME_SETTING_ENEMY_SPEED, 25));
    for (unsigned member = 0; member < 10; ++member)
        kill_stationary_plane(&g, 0, member, GAME_ENEMY_ENTER);
    assert(g.destroyed == 10 && g.entrance_kills[0] == 10 && g.score == 2000);
    for (unsigned member = 0; member < 9; ++member)
        kill_stationary_plane(&g, 1, member, GAME_ENEMY_ENTER);
    kill_stationary_plane(&g, 1, 9, GAME_ENEMY_HOLD);
    assert(g.destroyed == 20 && g.entrance_kills[1] == 9 && g.score == 3000);
}
static void depleted_fixture(game_state *g, unsigned kills) {
    combat(g);
    assert(game_set_setting(g, GAME_SETTING_ENEMY_SPEED, 25));
    for (unsigned member = 0; member < kills; ++member)
        kill_stationary_plane(g, member / 10u, member % 10u, GAME_ENEMY_ENTER);
    g->spawned = 40;
    for (unsigned i = 0; i < 40u - kills; ++i)
        g->enemies[i] = (game_enemy){.active = 1,
                                     .kind = 0,
                                     .hp = 1,
                                     .formation = 3,
                                     .member = (uint8_t)i,
                                     .mode = GAME_ENEMY_HOLD,
                                     .radius = 30 * 256};
    assert(game_set_setting(g, GAME_SETTING_ENEMY_SPEED, 100));
    game_step(g, NULL);
}
static void fifth_threshold_once_and_replay(void) {
    game_state low, a, b;
    depleted_fixture(&low, 30);
    assert(low.fifth_decided && low.stage_target == 40 && low.entrance_complete);
    depleted_fixture(&a, 31);
    depleted_fixture(&b, 31);
    assert(a.fifth_decided && a.stage_target == 50 && !a.entrance_complete);
    assert(a.fifth_path_planet < 5 && a.fifth_path_formation < 4);
    const uint8_t path_planet = a.fifth_path_planet, path_formation = a.fifth_path_formation;
    for (unsigned t = 0; t < 2000; ++t) {
        game_step(&a, NULL);
        game_step(&b, NULL);
        assert(game_state_hash(&a) == game_state_hash(&b));
        assert(a.stage_target == 50 && a.fifth_path_planet == path_planet &&
               a.fifth_path_formation == path_formation);
    }
    assert(a.phase == GAME_PLAY && a.spawned == 50 && a.destroyed == 31 && a.escaped == 0);
    /* The normal last-two escape remains available after reinforcements settle. */
    unsigned kept = 0;
    for (unsigned i = 0; i < GAME_ENEMIES; ++i)
        if (a.enemies[i].active && a.enemies[i].kind <= 2u && kept++ >= 2u)
            a.enemies[i].active = 0;
    a.destroyed = 48;
    for (unsigned t = 0; t < 1500 && a.phase == GAME_PLAY; ++t)
        game_step(&a, NULL);
    assert(a.phase == GAME_CLEAR && a.destroyed == 48 && a.escaped == 2);
}
static void late_fifth_keeps_its_authored_launch_spacing(void) {
    bool saw_single = false, saw_paired = false;
    for (uint32_t seed = 1; seed <= 24 && (!saw_single || !saw_paired); ++seed) {
        game_state g;
        combat(&g);
        assert(game_set_stage(&g, 8)); /* Saturn: final single-file entrant settles after900. */
        g.phase = GAME_PLAY;
        g.rng = seed;
        assert(game_set_setting(&g, GAME_SETTING_ENEMY_SPEED, 25));
        for (unsigned t = 0; t < 5000 && !g.fifth_decided; ++t) {
            if (g.destroyed < 31)
                shoot_entrants(&g, 31u - g.destroyed);
            else
                memset(g.shots, 0, sizeof g.shots);
            game_step(&g, NULL);
        }
        assert(g.destroyed == 31 && g.stage_target == 50 && g.enemy_ticks > 900);
        const unsigned decision_tick = g.enemy_ticks;
        const bool single = g.fifth_path_planet == 2 || g.fifth_path_planet == 4;
        saw_single |= single;
        saw_paired |= !single;
        memset(g.shots, 0, sizeof g.shots);
        for (unsigned t = 0; t < 500 && g.spawned < 50; ++t) {
            const unsigned before = g.spawned;
            game_step(&g, NULL);
            if (g.spawned == before)
                continue;
            const unsigned ordinal = before - 40u;
            const unsigned expected =
                decision_tick + 1u + (single ? ordinal * 9u : ordinal / 2u * 9u + ordinal % 2u);
            assert(g.enemy_ticks == expected);
        }
        assert(g.spawned == 50);
    }
    assert(saw_single && saw_paired);
}
static void explicit_debug_completion_uses_normal_result(void) {
    game_state g;
    game_init(&g, 1);
    for (unsigned phase = GAME_TITLE; phase <= GAME_WARP; ++phase) {
        if (phase == GAME_PLAY || phase == GAME_CHANCE)
            continue;
        g.phase = (game_phase)phase;
        const game_state before = g;
        assert(!game_debug_complete_stage(&g));
        assert(memcmp(&g, &before, sizeof g) == 0);
    }
    combat(&g);
    assert(game_debug_complete_stage(&g));
    assert(g.phase == GAME_PLAY && g.spawned == 40 && g.destroyed == 40);
    game_step(&g, NULL);
    assert(g.phase == GAME_CLEAR && !g.clear_chance && g.transition_bonus == 0);
    const game_state clear = g;
    assert(!game_debug_complete_stage(&g));
    assert(memcmp(&g, &clear, sizeof g) == 0);
    assert(game_set_stage(&g, 3));
    g.phase = GAME_CHANCE;
    assert(game_debug_complete_stage(&g));
    assert(g.phase == GAME_CHANCE && g.score == 0);
    game_step(&g, NULL);
    assert(g.phase == GAME_CLEAR && g.transition_bonus == 10000 && g.score == 10000);
}
static void mars_peels_into_individual_attacks(void) {
    game_state g;
    combat(&g);
    assert(game_set_stage(&g, 16));
    assert(g.planet == 4);
    g.phase = GAME_PLAY;
    bool early_attack = false;
    for (unsigned t = 0; t < 700; ++t) {
        game_step(&g, NULL);
        for (unsigned i = 0; i < GAME_ENEMIES; ++i)
            if (g.enemies[i].active && g.enemies[i].kind <= 2u &&
                g.enemies[i].mode == GAME_ENEMY_ATTACK && g.spawned < 40)
                early_attack = true;
    }
    assert(early_attack && g.phase == GAME_PLAY && g.escaped == 0);
}
static void later_destinations_have_distinct_entry_shapes(void) {
    uint16_t a, b;
    int32_t r, s;
    gyrus_wave_entry(2, 0, 0, 2, 0, 72, &a, &r);
    gyrus_wave_entry(2, 0, 5, 2, 0, 72, &b, &s);
    assert(a == b && r == s); /* Saturn's ten-plane train is single file. */
    gyrus_wave_entry(1, 0, 0, 1, 0, 48, &a, &r);
    gyrus_wave_entry(3, 0, 0, 3, 0, 48, &b, &s);
    assert(r != s); /* Jupiter does not reuse Uranus's hook. */
    gyrus_wave_entry(0, 0, 0, 0, 0, 72, &a, &r);
    gyrus_wave_entry(4, 0, 0, 4, 0, 72, &b, &s);
    assert(r != s); /* Mars's serial circuit differs from Neptune's crossing. */
    gyrus_wave_entry(3, 0, 0, 3, 0, 48, &a, &r);
    gyrus_wave_entry(5, 0, 0, 5, 0, 48, &b, &s);
    assert(r != s); /* Earth uses an explicitly authored foldback. */
}
int main(void) {
    mars_peels_into_individual_attacks();
    later_destinations_have_distinct_entry_shapes();
    no_fire_preserves_survivors();
    four_formations_before_attack();
    offscreen_attacker_returns();
    chance_flights_exit();
    recycled_pool_does_not_change_path();
    last_two_get_a_sortie_and_escape();
    fleeing_enemy_is_worth_one_thousand();
    pause_freezes_wave_lifecycle();
    missing_entities_do_not_complete_combat();
    perfect_entrances_require_fifth_group();
    whole_formation_bonus_requires_ten_entry_kills();
    fifth_threshold_once_and_replay();
    late_fifth_keeps_its_authored_launch_spacing();
    explicit_debug_completion_uses_normal_result();
    puts("Gyruss wave contracts passed");
    return 0;
}
