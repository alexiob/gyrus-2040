#include "gyrus/debug_game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t framebuffer[GAME_WIDTH * GAME_HEIGHT];
static char output[1024];
static size_t used;
static size_t total;
static void write_bytes(void *ctx, const void *p, size_t n) {
    (void)ctx;
    total += n;
    size_t copy = n;
    if (copy > sizeof output - 1 - used)
        copy = sizeof output - 1 - used;
    memcpy(output + used, p, copy);
    used += copy;
    output[used] = 0;
}
static void request(gyrus_debug *d, const char *line) {
    used = total = 0;
    gyrus_debug_feed(d, line, strlen(line));
}
int main(void) {
    game_state game;
    game_init(&game, 123);
    gyrus_debug_game adapter;
    gyrus_debug protocol;
    gyrus_debug_game_init(&adapter, &game, framebuffer, write_bytes, NULL);
    gyrus_debug_callbacks callbacks = gyrus_debug_game_callbacks(&adapter);
    gyrus_debug_init(&protocol, &callbacks);
    uint32_t initial = game_state_hash(&game);
    controls_state controls;
    controls_init(&controls);
    adapter.controls = &controls;
    request(&protocol, "G1 1 SET control_mode 2\n");
    assert(game.control_mode == 2 && controls.mode == CONTROLS_TILT);
    request(&protocol, "G1 1 SET control_mode 3\n");
    assert(strstr(output, "\"ok\":false"));
    assert(game.control_mode == 2 && controls.mode == CONTROLS_TILT);
    request(&protocol, "G1 1 SET control_mode -1\n");
    assert(strstr(output, "\"ok\":false"));
    request(&protocol, "G1 1 SET control_mode 0\n");
    assert(game.control_mode == 0 && controls.mode == CONTROLS_SPLIT_HOLD);
    request(&protocol, "G1 1 SET stage 9\n");
    assert(strstr(output, "\"ok\":false"));
    assert(game_state_hash(&game) == initial);
    request(&protocol, "G1 2 SET lives 100\n");
    assert(strstr(output, "\"ok\":false"));
    assert(game_state_hash(&game) == initial);
    request(&protocol, "G1 3 SET score 100000000\n");
    assert(strstr(output, "\"ok\":false"));
    assert(game_state_hash(&game) == initial);
    request(&protocol, "G1 3 SET high_score 100000000\n");
    assert(strstr(output, "\"ok\":false"));
    assert(game_state_hash(&game) == initial);
    request(&protocol, "G1 3 SET invincible 1\n");
    assert(game.invincible);
    request(&protocol, "G1 4 INPUT start=1 fire=1 turn=1\n");
    request(&protocol, "G1 5 STEP 1\n");
    assert(game.phase == GAME_INTRO);
    request(&protocol, "G1 5 STEP 120\n");
    assert(!adapter.input.start && adapter.input.fire && adapter.input.turn == 1);
    assert(game.tick == 120);
    uint32_t advanced = game_state_hash(&game);
    request(&protocol, "G1 6 FRAME\n");
    assert(strstr(output, "\"format\":\"indexed8\""));
    assert(total > 57600);
    assert(game_state_hash(&game) == advanced);
    request(&protocol, "G1 7 PAUSE\n");
    assert(adapter.frozen);
    request(&protocol, "G1 8 STEP 1\n");
    assert(game.tick == 121);
    assert(adapter.frozen);
    request(&protocol, "G1 9 RESUME\n");
    assert(!adapter.frozen);
    request(&protocol, "G1 10 RESET 123\n");
    assert(game_state_hash(&game) == initial);
    assert(!adapter.input.fire && !adapter.input.turn);
    request(&protocol, "G1 11 SET invincible 1\n");
    request(&protocol, "G1 12 INPUT start=1 fire=1 turn=1\n");
    request(&protocol, "G1 13 STEP 1\n");
    request(&protocol, "G1 13 STEP 120\n");
    assert(game_state_hash(&game) == advanced);
    request(&protocol, "G1 14 LEVEL 12\n");
    assert(game.stage == 12);
    request(&protocol, "G1 15 STATE\n");
    assert(strstr(output, "\"stage\":12"));
    assert(strstr(output, "\"hash\":"));
    puts("game debug adapter tests passed");
    return 0;
}
