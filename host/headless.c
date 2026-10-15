#include "gyrus/debug_game.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static game_state game;
static uint8_t framebuffer[GAME_WIDTH * GAME_HEIGHT];
static void write_bytes(void *ctx, const void *p, size_t n) {
    (void)ctx;
    if (fwrite(p, 1, n, stdout) != n)
        exit(1);
}
int main(int argc, char **argv) {
    uint32_t seed = 1;
    if (argc == 3 && !strcmp(argv[1], "--seed")) {
        char *end = NULL;
        errno = 0;
        unsigned long n = strtoul(argv[2], &end, 10);
        if (errno || !argv[2][0] || *end || argv[2][0] == '-' || n > UINT32_MAX) {
            fputs("Invalid seed\n", stderr);
            return 2;
        }
        seed = (uint32_t)n;
    } else if (argc != 1) {
        fputs("Usage: gyrus_headless [--seed UINT32]\nProtocol requests on stdin; responses on "
              "stdout.\n",
              stderr);
        return 2;
    }
    game_init(&game, seed);
    gyrus_debug_game adapter;
    gyrus_debug protocol;
    gyrus_debug_game_init(&adapter, &game, framebuffer, write_bytes, NULL);
    gyrus_debug_callbacks callbacks = gyrus_debug_game_callbacks(&adapter);
    gyrus_debug_init(&protocol, &callbacks);
    /* Unbuffered byte reads also work over a long-lived subprocess pipe. */
    int c;
    while ((c = fgetc(stdin)) != EOF) {
        unsigned char byte = (unsigned char)c;
        gyrus_debug_feed(&protocol, &byte, 1);
        if (byte == '\n')
            fflush(stdout);
    }
    return ferror(stdin) ? 1 : 0;
}
