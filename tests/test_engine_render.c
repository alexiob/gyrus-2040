#include "engine/draw.h"
#include "engine/hub.h"
#include "engine/math.h"
#include "gyrus/assets.h"
#include "gyrus/game.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint8_t before[32], pixels[240 * 240], after[32];
} guarded_frame;
static guarded_frame frame;
static uint8_t repeated[240 * 240];
static int title_x[3], title_y[3], calls;
static void remember_title(int x, int y) {
    if (calls < 3) {
        title_x[calls] = x;
        title_y[calls] = y;
    }
    ++calls;
}
static void gyrus_probe(uint8_t *p, int x, int y, uint32_t tick, bool neon) {
    remember_title(x, y);
    engine_title_gyrus(p, x, y, tick, neon);
}
static void title_probe(uint8_t *p, int x, int y, uint32_t tick, bool neon) {
    remember_title(x, y);
    engine_title_tempest(p, x, y, tick, neon);
}
static const engine_game_ops a = {
    .id = ENGINE_GAME_GYRUS, .title = "GYRUS", .year = 1983, .draw_title = gyrus_probe};
static const engine_game_ops b = {.id = ENGINE_GAME_TEMPEST,
                                  .title = "TEMPEST",
                                  .year = 1981,
                                  .capabilities = ENGINE_CAP_EFFECTS,
                                  .draw_title = title_probe};
static const engine_game_ops *const catalogue[] = {&a, &b};
static void guard(void) {
    memset(&frame, 0xa5, sizeof frame);
}
static void check(void) {
    unsigned lit = 0;
    for (unsigned i = 0; i < 32; ++i)
        assert(frame.before[i] == 0xa5 && frame.after[i] == 0xa5);
    for (int y = 0; y < 240; ++y)
        for (int x = 0; x < 240; ++x) {
            uint8_t c = frame.pixels[y * 240 + x];
            assert(c < DRAW_COLOR_COUNT);
            if ((x - 120) * (x - 120) + (y - 120) * (y - 120) > 14400)
                assert(c == DRAW_BLACK);
            lit += c != DRAW_BLACK;
        }
    assert(lit > 100);
}
static void primitives(void) {
    memset(frame.pixels, 0, sizeof frame.pixels);
    guard();
    memset(frame.pixels, 0, sizeof frame.pixels);
    draw_fill(frame.pixels, -4, -4, 8, 8, DRAW_CYAN);
    assert(frame.pixels[0] == DRAW_CYAN);
    draw_line(frame.pixels, -10, 120, 250, 120, DRAW_AMBER);
    assert(frame.pixels[120 * 240] == DRAW_AMBER);
    draw_label(frame.pixels, "GYRUS", 15, 50, DRAW_WHITE, 3);
    draw_mask_round(frame.pixels);
    check();
    char n[11];
    draw_decimal(n, UINT32_MAX, 30);
    assert(strcmp(n, "4294967295") == 0);
    draw_decimal(n, 9, 4);
    assert(strcmp(n, "0009") == 0);
    assert(draw_text_width("", 2) == 0 && draw_text_width("ABC", 2) == 34);
    for (int i = -128; i < 128; ++i)
        assert(memcmp(engine_font_glyph((char)i), gyrus_glyph((char)i), 7) == 0);
    for (unsigned i = 0; i < 256; ++i)
        assert(engine_palette[i] == gyrus_palette[i]);
    int16_t x, y;
    engine_polar(0, INT32_MAX, &x, &y);
    assert(x == 120 && y == -79);
    engine_polar(16384, INT32_MIN, &x, &y);
    assert(x == -79 && y == 120);
    for (unsigned i = 0; i < 65536; i += 127) {
        assert(engine_sin_q15((uint16_t)i) == game_sin_q15((uint16_t)i));
        assert(engine_cos_q15((uint16_t)i) == game_cos_q15((uint16_t)i));
    }
}
/* These references use per-pixel inequalities, not the renderer's span table
   or clipping algorithm. They also protect every original boundary pixel. */
static void geometry_equivalence(void) {
    for (unsigned y = 0; y < 240u; ++y)
        for (unsigned x = 0; x < 240u; ++x)
            frame.pixels[y * 240u + x] = (uint8_t)(1u + (x * 19u + y * 61u) % 255u);
    memset(frame.before, 0xa5, sizeof frame.before);
    memset(frame.after, 0xa5, sizeof frame.after);
    memcpy(repeated, frame.pixels, sizeof repeated);
    draw_mask_round(frame.pixels);
    for (int y = 0; y < 240; ++y)
        for (int x = 0; x < 240; ++x) {
            const int dx = x - 120, dy = y - 120;
            const unsigned offset = (unsigned)y * 240u + (unsigned)x;
            assert(frame.pixels[offset] ==
                   (dx * dx + dy * dy > 14400 ? DRAW_BLACK : repeated[offset]));
        }
    for (unsigned i = 0; i < 32u; ++i)
        assert(frame.before[i] == 0xa5u && frame.after[i] == 0xa5u);
    memcpy(repeated, frame.pixels, sizeof repeated);
    draw_mask_round(frame.pixels);
    assert(memcmp(repeated, frame.pixels, sizeof repeated) == 0);

    static const int rectangles[][4] = {{-4, -4, 8, 8},
                                        {0, 0, 240, 240},
                                        {239, 239, 20, 20},
                                        {240, 240, 30, 30},
                                        {-280, -280, 560, 560},
                                        {30, 40, 0, 10},
                                        {30, 40, 10, 0},
                                        {30, 40, -10, 20},
                                        {30, 40, 20, -10},
                                        {-30, 20, 30, 40},
                                        {20, -40, 30, 40},
                                        {INT_MAX, 0, 1, 1},
                                        {0, INT_MAX, 1, 1},
                                        {INT_MIN, 0, INT_MAX, 1},
                                        {0, INT_MIN, 1, INT_MAX},
                                        {20, 30, INT_MAX, INT_MAX},
                                        {-2, -3, INT_MAX, INT_MAX}};
    for (unsigned r = 0; r < sizeof rectangles / sizeof rectangles[0]; ++r) {
        const int *rect = rectangles[r];
        guard();
        memset(frame.pixels, DRAW_BLUE, sizeof frame.pixels);
        draw_fill(frame.pixels, rect[0], rect[1], rect[2], rect[3], DRAW_CYAN);
        for (int y = 0; y < 240; ++y)
            for (int x = 0; x < 240; ++x) {
                const bool inside = rect[2] > 0 && rect[3] > 0 && (int64_t)x >= rect[0] &&
                                    (int64_t)x < (int64_t)rect[0] + rect[2] &&
                                    (int64_t)y >= rect[1] &&
                                    (int64_t)y < (int64_t)rect[1] + rect[3];
                assert(frame.pixels[y * 240 + x] == (inside ? DRAW_CYAN : DRAW_BLUE));
            }
        for (unsigned i = 0; i < 32u; ++i)
            assert(frame.before[i] == 0xa5u && frame.after[i] == 0xa5u);
    }
}
static unsigned title_ink(const uint8_t *pixels) {
    unsigned count = 0;
    for (unsigned y = 59; y < 98; ++y)
        for (unsigned x = 42; x < 199; ++x) {
            const uint8_t c = pixels[y * 240 + x];
            count += c == DRAW_WHITE || c == DRAW_IVORY || c == DRAW_CYAN || c == DRAW_PALE_BLUE;
        }
    return count;
}
static void hub_cases(void) {
    engine_hub_view view = {.catalogue = catalogue,
                            .count = 2,
                            .high_score = 123456,
                            .settings = {5, 100, 100},
                            .ui = {.control_mode = 2, .tilt_ready = true, .tilt_preview = 32767}};
    const int slides[] = {0, 220, 110, 55, -55, -110, -220};
    for (unsigned selected = 0; selected < 2; ++selected)
        for (unsigned i = 0; i < sizeof slides / sizeof slides[0]; ++i) {
            view.selected = selected;
            view.slide_pixels = (int16_t)slides[i];
            view.tick = i * 9u;
            view.neon = selected != 0;
            calls = 0;
            const engine_hub_view saved = view;
            guard();
            engine_hub_render(&view, frame.pixels);
            check();
            assert(memcmp(&saved, &view, sizeof view) == 0);
            engine_hub_render(&view, repeated);
            assert(memcmp(repeated, frame.pixels, sizeof repeated) == 0);
            assert(calls == 6);
            assert(title_x[0] == 120 + slides[i]);
            assert(title_y[0] == 78);
            assert(title_x[1] == title_x[0] - 220);
            assert(title_x[2] == title_x[0] + 220);
        }
    view.selected = 0;
    view.slide_pixels = 0;
    view.tick = 0;
    guard();
    engine_hub_render(&view, frame.pixels);
    assert(title_ink(frame.pixels) > 500u);
    assert(frame.pixels[80 * 240 + 24] == DRAW_DEEP_BLUE);
    view.selected = 1;
    guard();
    engine_hub_render(&view, frame.pixels);
    assert(title_ink(frame.pixels) > 500u);
    view.settings_open = true;
    engine_hub_render(&view, frame.pixels);
    assert(frame.pixels[102 * 240 + 29] == DRAW_BLACK);
    view.settings_open = false;
    view.selected = UINT_MAX;
    guard();
    engine_hub_render(&view, frame.pixels);
    check();
    view.catalogue = NULL;
    view.count = 0;
    guard();
    engine_hub_render(&view, frame.pixels);
    check();
    view.catalogue = catalogue;
    view.count = 2;
    view.selected = 0;
    view.slide_pixels = 0;
    for (unsigned settings = 0; settings < 2; ++settings) {
        view.settings_open = settings != 0;
        guard();
        engine_hub_render(&view, frame.pixels);
        check();
    }
}
static void menu_cases(void) {
    engine_menu_view view = {.values = {5, 100, 100}};
    for (unsigned s = 0; s < 2; ++s)
        for (unsigned mode = 0; mode < 3; ++mode) {
            view.settings = s != 0;
            view.ui.control_mode = (uint8_t)mode;
            view.ui.tilt_ready = mode == 2;
            view.ui.tilt_preview = -32768;
            view.ui.reboot_progress = 65535;
            const engine_menu_view saved = view;
            guard();
            memset(frame.pixels, DRAW_CYAN, sizeof frame.pixels);
            engine_ui_menu(&view, frame.pixels);
            draw_mask_round(frame.pixels);
            check();
            memset(repeated, DRAW_CYAN, sizeof repeated);
            engine_ui_menu(&view, repeated);
            draw_mask_round(repeated);
            assert(memcmp(repeated, frame.pixels, sizeof repeated) == 0);
            assert(memcmp(&saved, &view, sizeof view) == 0);
        }
}
int main(void) {
    primitives();
    geometry_equivalence();
    hub_cases();
    menu_cases();
    puts("engine graphics contracts passed");
    return 0;
}
