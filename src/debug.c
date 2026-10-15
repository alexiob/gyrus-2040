#include "gyrus/debug.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static void emit(gyrus_debug *d, const void *p, size_t n) {
    if (d->callbacks.write)
        d->callbacks.write(d->callbacks.context, p, n);
}
static void reply(gyrus_debug *d, uint32_t id, const char *error) {
    char out[160];
    int n =
        error ? snprintf(out, sizeof out,
                         "{\"v\":1,\"id\":%" PRIu32 ",\"ok\":false,\"error\":\"%s\"}\n", id, error)
              : snprintf(out, sizeof out, "{\"v\":1,\"id\":%" PRIu32 ",\"ok\":true}\n", id);
    if (n > 0 && (size_t)n < sizeof out)
        emit(d, out, (size_t)n);
}
static bool unsigned_number(const char *s, uint32_t max, uint32_t *out) {
    if (!s || !*s)
        return false;
    uint32_t n = 0;
    for (; *s; s++) {
        if (*s < '0' || *s > '9')
            return false;
        unsigned digit = (unsigned)(*s - '0');
        if (n > max / 10u || (n == max / 10u && digit > max % 10u))
            return false;
        n = n * 10u + digit;
    }
    *out = n;
    return true;
}
static bool signed_number(const char *s, int32_t min, int32_t max, int32_t *out) {
    if (!s)
        return false;
    bool negative = *s == '-';
    if (negative)
        s++;
    uint32_t n, limit = negative ? (uint32_t)(-(int64_t)min) : (uint32_t)max;
    if (!unsigned_number(s, limit, &n))
        return false;
    int64_t value = negative ? -(int64_t)n : (int64_t)n;
    if (value < min || value > max)
        return false;
    *out = (int32_t)value;
    return true;
}
static uint32_t crc_update(uint32_t crc, const uint8_t *p, size_t n) {
    while (n--) {
        crc ^= *p++;
        for (unsigned bit = 0; bit < 8; bit++)
            crc = (crc >> 1) ^ ((0u - (crc & 1u)) & 0xedb88320u);
    }
    return crc;
}
uint32_t gyrus_debug_crc32(const void *p, size_t n) {
    return crc_update(UINT32_MAX, p, n) ^ UINT32_MAX;
}
static void frame_reply(gyrus_debug *d, uint32_t id) {
    gyrus_debug_frame f = {0};
    if (!d->callbacks.frame || !d->callbacks.frame(d->callbacks.context, &f)) {
        reply(d, id, "unavailable");
        return;
    }
    if (!f.width || !f.height || f.width > 240 || f.height > 240 || !f.palette_count ||
        f.palette_count > 256 || !f.palette || !f.pixels ||
        f.pixels_len != (size_t)f.width * f.height || f.pixels_len > GYRUS_DEBUG_FRAME_MAX) {
        reply(d, id, "invalid_frame");
        return;
    }
    for (size_t i = 0; i < f.pixels_len; i++)
        if (f.pixels[i] >= f.palette_count) {
            reply(d, id, "invalid_frame");
            return;
        }
    uint8_t palette[512];
    size_t palette_len = (size_t)f.palette_count * 2u;
    for (size_t i = 0; i < f.palette_count; i++) {
        palette[2 * i] = (uint8_t)f.palette[i];
        palette[2 * i + 1] = (uint8_t)(f.palette[i] >> 8);
    }
    uint32_t crc = crc_update(UINT32_MAX, palette, palette_len);
    crc = crc_update(crc, f.pixels, f.pixels_len) ^ UINT32_MAX;
    char out[256];
    int n = snprintf(out, sizeof out,
                     "{\"v\":1,\"id\":%" PRIu32
                     ",\"ok\":true,\"format\":\"indexed8\",\"width\":%u,\"height\":%u,\"palette_"
                     "bytes\":%zu,\"pixel_bytes\":%zu,\"crc32\":%" PRIu32 "}\n",
                     id, (unsigned)f.width, (unsigned)f.height, palette_len, f.pixels_len, crc);
    if (n <= 0 || (size_t)n >= sizeof out) {
        reply(d, id, "internal");
        return;
    }
    emit(d, out, (size_t)n);
    emit(d, palette, palette_len);
    for (size_t offset = 0; offset < f.pixels_len; offset += 256u) {
        size_t count = f.pixels_len - offset;
        if (count > 256u)
            count = 256u;
        emit(d, f.pixels + offset, count);
    }
    emit(d, "\n", 1);
}
static bool parse_input(char **t, size_t count, gyrus_debug_input *out) {
    *out = (gyrus_debug_input){0};
    if (count == 0)
        return false;
    for (size_t i = 0; i < count; i++) {
        char *eq = strchr(t[i], '=');
        if (!eq)
            return false;
        *eq++ = 0;
        uint8_t mask;
        int32_t v;
        if (!strcmp(t[i], "turn")) {
            mask = GYRUS_INPUT_TURN;
            if (!signed_number(eq, -1, 1, &v))
                return false;
            out->turn = (int8_t)v;
        } else if (!strcmp(t[i], "angle_delta")) {
            mask = GYRUS_INPUT_ANGLE;
            if (!signed_number(eq, -32768, 32767, &v))
                return false;
            out->angle_delta = (int16_t)v;
        } else {
            if (!signed_number(eq, 0, 1, &v))
                return false;
            if (!strcmp(t[i], "fire")) {
                mask = GYRUS_INPUT_FIRE;
                out->fire = v != 0;
            } else if (!strcmp(t[i], "start")) {
                mask = GYRUS_INPUT_START;
                out->start = v != 0;
            } else if (!strcmp(t[i], "pause")) {
                mask = GYRUS_INPUT_PAUSE;
                out->pause = v != 0;
            } else if (!strcmp(t[i], "quit")) {
                mask = GYRUS_INPUT_QUIT;
                out->quit = v != 0;
            } else
                return false;
        }
        if (out->mask & mask)
            return false;
        out->mask |= mask;
    }
    return true;
}
static void process(gyrus_debug *d) {
    char *tokens[12];
    size_t count = 0;
    char *p = d->line;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\r')
            p++;
        if (!*p)
            break;
        if (count == 12) {
            reply(d, 0, "syntax");
            return;
        }
        tokens[count++] = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\r')
            p++;
        if (*p)
            *p++ = 0;
    }
    if (count == 0)
        return;
    uint32_t id = 0;
    if (count < 3 || strcmp(tokens[0], "G1") || !unsigned_number(tokens[1], UINT32_MAX, &id)) {
        reply(d, 0, "syntax");
        return;
    }
    const char *cmd = tokens[2];
    void *ctx = d->callbacks.context;
    bool ok = false;
    if (!strcmp(cmd, "HELLO") && count == 3) {
        char out[200];
        int n = snprintf(out, sizeof out,
                         "{\"v\":1,\"id\":%" PRIu32
                         ",\"ok\":true,\"protocol\":\"G1\",\"max_line\":255,\"max_step\":3600}\n",
                         id);
        if (n > 0 && (size_t)n < sizeof out)
            emit(d, out, (size_t)n);
        return;
    }
    if (!strcmp(cmd, "STATE") && count == 3) {
        char state[GYRUS_DEBUG_STATE_MAX] = {0};
        if (!d->callbacks.state || !d->callbacks.state(ctx, state, sizeof state)) {
            reply(d, id, "unavailable");
            return;
        }
        size_t length = 0;
        while (length < sizeof state && state[length])
            length++;
        if (length == sizeof state || length < 2 || state[0] != '{' || state[length - 1] != '}' ||
            memchr(state, '\n', length) || memchr(state, '\r', length)) {
            reply(d, id, "invalid_state");
            return;
        }
        char header[80];
        int n = snprintf(header, sizeof header,
                         "{\"v\":1,\"id\":%" PRIu32 ",\"ok\":true,\"state\":", id);
        if (n > 0 && (size_t)n < sizeof header) {
            emit(d, header, (size_t)n);
            emit(d, state, length);
            emit(d, "}\n", 2);
        }
        return;
    }
    if (!strcmp(cmd, "FRAME") && count == 3) {
        frame_reply(d, id);
        return;
    }
    if (!strcmp(cmd, "SET") && count == 5) {
        int32_t n;
        if (!signed_number(tokens[4], INT32_MIN, INT32_MAX, &n)) {
            reply(d, id, "range");
            return;
        }
        ok = d->callbacks.set && d->callbacks.set(ctx, tokens[3], n);
    } else if (!strcmp(cmd, "INPUT") && count >= 4) {
        gyrus_debug_input input;
        if (!parse_input(tokens + 3, count - 3, &input)) {
            reply(d, id, "range");
            return;
        }
        ok = d->callbacks.input && d->callbacks.input(ctx, &input);
    } else {
        gyrus_debug_action action;
        uint32_t n = 0;
        if (!strcmp(cmd, "STEP") && count == 4) {
            action = GYRUS_DEBUG_STEP;
            if (!unsigned_number(tokens[3], 3600, &n) || !n) {
                reply(d, id, "range");
                return;
            }
        } else if (!strcmp(cmd, "LEVEL") && count == 4) {
            action = GYRUS_DEBUG_LEVEL;
            if (!unsigned_number(tokens[3], 999, &n) || !n) {
                reply(d, id, "range");
                return;
            }
        } else if (!strcmp(cmd, "RESET") && count == 4) {
            action = GYRUS_DEBUG_RESET;
            if (!unsigned_number(tokens[3], UINT32_MAX, &n)) {
                reply(d, id, "range");
                return;
            }
        } else if (!strcmp(cmd, "PAUSE") && count == 3)
            action = GYRUS_DEBUG_PAUSE;
        else if (!strcmp(cmd, "RESUME") && count == 3)
            action = GYRUS_DEBUG_RESUME;
        else {
            reply(d, id, "syntax");
            return;
        }
        ok = d->callbacks.action && d->callbacks.action(ctx, action, n);
    }
    reply(d, id, ok ? NULL : "rejected");
}
void gyrus_debug_init(gyrus_debug *d, const gyrus_debug_callbacks *cb) {
    memset(d, 0, sizeof *d);
    if (cb)
        d->callbacks = *cb;
}
void gyrus_debug_feed(gyrus_debug *d, const void *data, size_t size) {
    const unsigned char *p = data;
    for (size_t i = 0; i < size; i++) {
        unsigned char c = p[i];
        if (c == '\n') {
            if (d->dropping)
                reply(d, 0, d->dropping == 1 ? "line_too_long" : "invalid_ascii");
            else {
                d->line[d->used] = 0;
                process(d);
            }
            d->used = 0;
            d->dropping = 0;
        } else if (!d->dropping) {
            if (d->used == sizeof d->line - 1)
                d->dropping = 1;
            else if ((c >= 32 && c <= 126) || c == '\r' || c == '\t')
                d->line[d->used++] = (char)c;
            else {
                d->used = 0;
                d->dropping = 2;
            }
        }
    }
}
