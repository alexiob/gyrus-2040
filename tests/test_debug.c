#include "gyrus/debug.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned char out[70000];
    size_t used;
    unsigned steps;
    int32_t score;
    gyrus_debug_input input;
    uint8_t pixels[4];
    uint16_t palette[2];
    bool invalid_frame;
} fixture;
static void write_bytes(void *ctx, const void *p, size_t n) {
    fixture *f = ctx;
    assert(f->used + n < sizeof f->out);
    memcpy(f->out + f->used, p, n);
    f->used += n;
    f->out[f->used] = 0;
}
static bool state(void *ctx, char *out, size_t cap) {
    fixture *f = ctx;
    return snprintf(out, cap, "{\"score\":%ld}", (long)f->score) > 0;
}
static bool set(void *ctx, const char *key, int32_t n) {
    if (strcmp(key, "score"))
        return false;
    ((fixture *)ctx)->score = n;
    return true;
}
static bool action(void *ctx, gyrus_debug_action a, uint32_t n) {
    if (a == GYRUS_DEBUG_STEP)
        ((fixture *)ctx)->steps += n;
    return true;
}
static bool input(void *ctx, const gyrus_debug_input *i) {
    ((fixture *)ctx)->input = *i;
    return true;
}
static bool frame(void *ctx, gyrus_debug_frame *out) {
    fixture *f = ctx;
    *out = (gyrus_debug_frame){2, 2, 2, f->palette, f->pixels, f->invalid_frame ? 5u : 4u};
    return true;
}
static void request(gyrus_debug *d, fixture *f, const char *line) {
    f->used = 0;
    gyrus_debug_feed(d, line, strlen(line));
}
int main(void) {
    fixture f = {0};
    f.palette[0] = 0x1234;
    f.palette[1] = 0xabcd;
    f.pixels[0] = 0;
    f.pixels[1] = 1;
    f.pixels[2] = 1;
    f.pixels[3] = 0;
    gyrus_debug d;
    gyrus_debug_callbacks cb = {&f, write_bytes, state, set, action, input, frame};
    gyrus_debug_init(&d, &cb);
    request(&d, &f, "G1 1 HELLO\n");
    assert(strstr((char *)f.out, "\"id\":1,\"ok\":true"));
    request(&d, &f, "G1 7 SET score 123\nG1 8 STATE\n");
    assert(f.score == 123);
    assert(strstr((char *)f.out, "\"score\":123"));
    request(&d, &f, "G1 9 STEP 5\n");
    assert(f.steps == 5);
    request(&d, &f, "G1 10 INPUT turn=-1 angle_delta=123 fire=1\n");
    assert(f.input.turn == -1 && f.input.fire && f.input.angle_delta == 123);
    assert(f.input.mask == (GYRUS_INPUT_TURN | GYRUS_INPUT_ANGLE | GYRUS_INPUT_FIRE));
    request(&d, &f, "G1 11 INPUT turn=2\n");
    assert(strstr((char *)f.out, "\"ok\":false"));
    request(&d, &f, "G1 11 INPUT fire=1 fire=0\n");
    assert(strstr((char *)f.out, "\"ok\":false"));
    const char *bad[] = {"G1 12 STEP -1\n",         "G1 12 STEP 0\n",
                         "G1 12 STEP 3601\n",       "G1 12 SET score 2147483648\n",
                         "G1 12 SET forbidden 1\n", "G1 12 STATE extra\n",
                         "G1 4294967296 STATE\n",   "G2 12 STATE\n",
                         "G1 12 INPUT wat=1\n"};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
        request(&d, &f, bad[i]);
        assert(strstr((char *)f.out, "\"ok\":false"));
    }
    request(&d, &f, "G1 21 FRAME\n");
    unsigned char *newline = memchr(f.out, '\n', f.used);
    assert(newline);
    assert(strstr((char *)f.out, "\"format\":\"indexed8\""));
    const unsigned char expected[] = {0x34, 0x12, 0xcd, 0xab, 0, 1, 1, 0, '\n'};
    assert((size_t)(f.out + f.used - newline - 1) == sizeof expected);
    assert(!memcmp(newline + 1, expected, sizeof expected));
    f.invalid_frame = true;
    request(&d, &f, "G1 22 FRAME\n");
    assert(strstr((char *)f.out, "invalid_frame"));
    char big[400];
    memset(big, 'x', sizeof big);
    f.used = 0;
    gyrus_debug_feed(&d, big, sizeof big);
    gyrus_debug_feed(&d, "\nG1 23 STATE\n", 13);
    assert(strstr((char *)f.out, "line_too_long"));
    assert(strstr((char *)f.out, "\"id\":23"));
    request(&d, &f, "G1 24 STA");
    assert(f.used == 0);
    gyrus_debug_feed(&d, "TE\r\n", 4);
    assert(strstr((char *)f.out, "\"id\":24,\"ok\":true"));
    f.used = 0;
    const unsigned char invalid_ascii[] = {'G', '1', ' ', '2', '5', ' ', 'S',
                                           'T', 'A', 'T', 'E', 0,   'x', '\n'};
    gyrus_debug_feed(&d, invalid_ascii, sizeof invalid_ascii);
    assert(strstr((char *)f.out, "invalid_ascii"));
    unsigned lines = 0;
    for (size_t i = 0; i < f.used; i++)
        lines += f.out[i] == '\n';
    assert(lines == 1);
    assert(gyrus_debug_crc32("123456789", 9) == 0xcbf43926u);
    puts("debug protocol tests passed");
    return 0;
}
