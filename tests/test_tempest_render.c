#include "engine/draw.h"
#include "tempest/game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void transition_and_event_feedback(void) {
    static uint8_t first[ENGINE_WIDTH * ENGINE_HEIGHT], second[ENGINE_WIDTH * ENGINE_HEIGHT];
    tempest_state s;
    tempest_init(&s, 22);
    tempest_start(&s);
    s.phase_ticks = 0;
    tempest_render(&s, first);
    s.phase_ticks = 30;
    tempest_render(&s, second);
    assert(memcmp(first, second, sizeof first)); /* An entry is visible motion, not a timer only. */
    s.phase = TEMPEST_FLIGHT;
    s.phase_ticks = 5;
    tempest_render(&s, first);
    s.phase_ticks = 45;
    tempest_render(&s, second);
    assert(memcmp(first, second, sizeof first));
    /* Stage4095 exits Fan into Circle; the next-web preview must honor wrap. */
    assert(tempest_set_stage(&s, 15));
    s.phase = TEMPEST_FLIGHT;
    s.phase_ticks = 30;
    tempest_render(&s, first);
    s.stage = 4095;
    tempest_render(&s, second);
    /* Flight contains only the receding current Fan, not a first zoom of the
       upcoming web. Different next-web names appear below the playfield. */
    assert(!memcmp(first + 40 * ENGINE_WIDTH, second + 40 * ENGINE_WIDTH, 135 * ENGINE_WIDTH));
    assert(memcmp(first + 177 * ENGINE_WIDTH, second + 177 * ENGINE_WIDTH, 34 * ENGINE_WIDTH));
    assert(tempest_set_stage(&s, 20));
    s.autofire = false;
    s.enemies[0] = (tempest_enemy){
        .active = 1, .landed = 1, .kind = TEMPEST_TANKER, .depth = 20000, .lane_q8 = 512};
    s.enemies[1] = (tempest_enemy){.active = 1, .landed = 0, .timer = 120};
    engine_input in = {.special = true};
    tempest_step(&s, &in);
    assert(s.zap_flash);
    tempest_render(&s, first);
    tempest_state bare = s;
    memset(bare.visual, 0, sizeof bare.visual);
    bare.zap_flash = 0;
    assert(tempest_hash(&bare) == tempest_hash(&s));
    tempest_render(&bare, second);
    assert(memcmp(first, second, sizeof first));
    s = bare;
    s.enemies[2] = (tempest_enemy){
        .active = 1, .landed = 1, .kind = TEMPEST_FLIPPER, .depth = 25000, .lane_q8 = 1024};
    s.shots[0] = (tempest_shot){27000, 4, 1};
    in = (engine_input){0};
    tempest_step(&s, &in);
    unsigned hits = 0;
    for (unsigned i = 0; i < TEMPEST_VISUAL_EVENTS; i++)
        hits += s.visual[i].life && s.visual[i].kind == TEMPEST_FX_HIT;
    assert(hits);
    s.visual[0].age = 8;
    tempest_render(&s, first);
    bare = s;
    memset(bare.visual, 0, sizeof bare.visual);
    tempest_render(&bare, second);
    assert(memcmp(first, second, sizeof first));
}
static void crossing_pulse_matches_highlight(void) {
    static uint8_t pixels[ENGINE_WIDTH * ENGINE_HEIGHT];
    for (unsigned player_lane = 2; player_lane <= 3; player_lane++) {
        tempest_state s;
        tempest_init(&s, 10);
        assert(tempest_set_stage(&s, 17));
        s.autofire = false;
        s.invulnerable = 0;
        s.claw_q8 = (int32_t)player_lane * 256;
        s.enemies[0] = (tempest_enemy){.active = 1,
                                       .landed = 1,
                                       .hp = 1,
                                       .kind = TEMPEST_PULSAR,
                                       .depth = 1000,
                                       .lane_q8 = 512,
                                       .direction = 1,
                                       .crossing = 1,
                                       .timer = 144,
                                       .age = 125};
        tempest_render(&s, pixels);
        int source_x, source_y, dest_x, dest_y;
        tempest_project(tempest_current_web(&s), 2 * 256, TEMPEST_DEPTH, &source_x, &source_y);
        tempest_project(tempest_current_web(&s), 4 * 256, TEMPEST_DEPTH, &dest_x, &dest_y);
        assert(pixels[(unsigned)source_y * ENGINE_WIDTH + (unsigned)source_x] == DRAW_YELLOW);
        assert(pixels[(unsigned)dest_y * ENGINE_WIDTH + (unsigned)dest_x] != DRAW_YELLOW);
        tempest_step(&s, NULL);
        if (player_lane == 2)
            assert(s.phase == TEMPEST_DEATH && s.lives == 2);
        else
            assert(s.phase == TEMPEST_PLAY && s.lives == 3);
    }
}
int main(void) {
    transition_and_event_feedback();
    crossing_pulse_matches_highlight();
    static uint8_t guarded[ENGINE_WIDTH * ENGINE_HEIGHT + 32];
    tempest_state s;
    tempest_init(&s, 99);
    for (unsigned stage = 1; stage <= 96; stage++) {
        assert(tempest_set_stage(&s, stage));
        s.effects = (stage & 1) != 0;
        for (unsigned i = 0; i < 5; i++)
            s.enemies[i] = (tempest_enemy){.active = 1,
                                           .landed = 1,
                                           .kind = (uint8_t)i,
                                           .depth = (int32_t)(10000 + i * 10000),
                                           .lane_q8 = (int16_t)(i * 256),
                                           .crossing = 1,
                                           .timer = 120};
        for (unsigned phase = 0; phase < 9; phase++) {
            s.phase = (tempest_phase)phase;
            s.phase_ticks = 30;
            s.dive_depth = 32000;
            memset(guarded, 0xa5, sizeof guarded);
            uint32_t before = tempest_hash(&s);
            tempest_render(&s, guarded + 16);
            assert(before == tempest_hash(&s));
            for (unsigned i = 0; i < 16; i++) {
                assert(guarded[i] == 0xa5);
                assert(guarded[sizeof guarded - 1 - i] == 0xa5);
            }
            for (unsigned y = 0; y < 240; y++)
                for (unsigned x = 0; x < 240; x++) {
                    unsigned c = guarded[16 + y * 240 + x];
                    assert(c < DRAW_COLOR_COUNT);
                    int dx = (int)x - 120, dy = (int)y - 120;
                    if (dx * dx + dy * dy > 120 * 120)
                        assert(c == DRAW_BLACK);
                }
        }
    }
    /* The bottom claw sits directly above ZAP. Its bright emitter must remain
     * foreground, with a black separation from the selected lane beneath it. */
    tempest_init(&s, 99);
    assert(tempest_set_stage(&s, 1));
    s.claw_q8 = 7 * 256 + 128;
    s.invulnerable = 0;
    tempest_render(&s, guarded + 16);
    assert(guarded[16 + 174 * ENGINE_WIDTH + 120] == DRAW_YELLOW);
    assert(guarded[16 + 167 * ENGINE_WIDTH + 120] == DRAW_BLACK);
    assert(guarded[16 + 176 * ENGINE_WIDTH + 120] == DRAW_CYAN);
    puts("Tempest rendering pure, bounded and round");
    return 0;
}
