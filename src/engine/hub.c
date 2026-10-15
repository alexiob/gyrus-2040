#include "engine/hub.h"
#include "engine/draw.h"
#include <string.h>

/* The launcher is a pure projection of its view: no retained animation state,
   game selection, input reads or wall-clock work lives in this renderer. */
static void at_center(uint8_t *pixels, const char *text, int cx, int y, uint8_t color, int scale) {
    draw_label(pixels, text, cx - draw_text_width(text, scale) / 2, y, color, scale);
}
static void bracket_button(uint8_t *pixels, int x0, int y0, int x1, int y1, const char *text,
                           uint8_t color, int scale) {
    draw_fill(pixels, x0, y0, x1 - x0, y1 - y0, DRAW_BLACK);
    draw_line(pixels, x0, y0, x1 - 1, y0, DRAW_DARK);
    draw_line(pixels, x0, y1 - 1, x1 - 1, y1 - 1, DRAW_DARK);
    draw_line(pixels, x0, y0, x0, y1 - 1, DRAW_DARK);
    draw_line(pixels, x1 - 1, y0, x1 - 1, y1 - 1, DRAW_DARK);
    draw_line(pixels, x0, y0, x0 + 14, y0, color);
    draw_line(pixels, x0, y0, x0, y0 + 8, color);
    draw_line(pixels, x1 - 15, y1 - 1, x1 - 1, y1 - 1, color);
    draw_line(pixels, x1 - 1, y1 - 9, x1 - 1, y1 - 1, color);
    at_center(pixels, text, (x0 + x1) / 2, (y0 + y1 - 7 * scale) / 2, color, scale);
}
static void background(uint8_t *pixels, uint32_t tick) {
    for (unsigned i = 0; i < 42u; ++i) {
        const int x = (int)((i * 73u + 19u) % 240u), y = (int)((i * 47u + 53u) % 240u);
        uint8_t color = (i + tick / 40u) % 9u == 0u ? DRAW_STEEL : DRAW_DEEP_BLUE;
        draw_pixel(pixels, x, y, color);
    }
    draw_line(pixels, 63, 37, 176, 37, DRAW_DARK);
    draw_line(pixels, 69, 39, 96, 39, DRAW_DEEP_BLUE);
    draw_line(pixels, 144, 39, 170, 39, DRAW_DEEP_BLUE);
    draw_centered(pixels, "ROUND ARCADE", 16, DRAW_CYAN, 1);
    draw_centered(pixels, "SELECT YOUR GAME", 29, DRAW_STEEL, 1);
}
static void card(uint8_t *pixels, const engine_game_ops *ops, int cx, const engine_hub_view *view,
                 bool selected) {
    if (!ops)
        return;
    /* At rest only a neighbor's outer rail peeks into the aperture. */
    for (int side = -1; side <= 1; side += 2) {
        const int edge = cx + side * 124;
        draw_line(pixels, edge, 76, edge, 138, DRAW_DEEP_BLUE);
        draw_line(pixels, edge, 76, edge - side * 7, 69, DRAW_DEEP_BLUE);
        draw_line(pixels, edge, 138, edge - side * 7, 145, DRAW_DEEP_BLUE);
    }
    if (ops->draw_title)
        ops->draw_title(pixels, cx, 78, view->tick, view->neon);
    else
        at_center(pixels, ops->title ? ops->title : "UNTITLED", cx, 72, DRAW_IVORY, 2);
    char year[11];
    draw_decimal(year, ops->year, 4);
    const char *name = ops->title ? ops->title : "UNTITLED";
    const int width = draw_text_width(name, 1) + 12 + draw_text_width(year, 1);
    draw_label(pixels, name, cx - width / 2, 103, DRAW_STEEL, 1);
    draw_label(pixels, year, cx - width / 2 + draw_text_width(name, 1) + 12, 103, DRAW_AMBER, 1);
    draw_pixel(pixels, cx - width / 2 + draw_text_width(name, 1) + 5, 106, DRAW_DARK);
    if (selected) {
        char high[11];
        draw_decimal(high, view->high_score, 6);
        const int score_width = 18 + draw_text_width(high, 1);
        draw_label(pixels, "HI", cx - score_width / 2, 45, DRAW_AMBER, 1);
        draw_label(pixels, high, cx - score_width / 2 + 18, 45, DRAW_IVORY, 1);
        if ((ops->capabilities & ENGINE_CAP_EFFECTS) != 0u)
            bracket_button(pixels, cx - 54, 112, cx + 54, 138,
                           view->neon ? "STYLE  NEON" : "STYLE CLASSIC", DRAW_CYAN, 1);
        else {
            at_center(pixels, "ORBITAL SHOOTER", cx, 120, DRAW_BLUE, 1);
            draw_line(pixels, cx - 28, 134, cx + 28, 134, DRAW_DEEP_BLUE);
        }
    }
}
static void arrows(uint8_t *pixels, bool enabled) {
    const uint8_t c = enabled ? DRAW_AMBER : DRAW_DARK;
    draw_line(pixels, 38, 93, 29, 102, c);
    draw_line(pixels, 29, 102, 38, 111, c);
    draw_line(pixels, 201, 93, 210, 102, c);
    draw_line(pixels, 210, 102, 201, 111, c);
    draw_line(pixels, 39, 93, 30, 102, DRAW_DARK);
    draw_line(pixels, 30, 102, 39, 111, DRAW_DARK);
    draw_line(pixels, 200, 93, 209, 102, DRAW_DARK);
    draw_line(pixels, 209, 102, 200, 111, DRAW_DARK);
}
static void tilt_marker(uint8_t *pixels, const engine_ui *ui) {
    int bank = ui->tilt_preview;
    if (bank < -1000)
        bank = -1000;
    if (bank > 1000)
        bank = 1000;
    int x = 120;
    if (ui->control_mode == 2u && ui->tilt_ready)
        x += bank * 36 / 1000;
    draw_centered(pixels,
                  ui->control_mode == 2u
                      ? (ui->tilt_ready ? "TAP SHIP TO CENTRE" : "HOLD STILL TO CENTRE")
                      : "SWIPE TO BROWSE",
                  213, DRAW_STEEL, 1);
    const uint8_t color = ui->control_mode == 2u && !ui->tilt_ready ? DRAW_AMBER : DRAW_CYAN;
    draw_line(pixels, x, 222, x - 5, 231, color);
    draw_line(pixels, x - 5, 231, x, 228, DRAW_BLUE);
    draw_line(pixels, x, 228, x + 5, 231, DRAW_BLUE);
    draw_line(pixels, x + 5, 231, x, 222, color);
    draw_line(pixels, x, 225, x, 228, DRAW_IVORY);
}
void engine_hub_render(const engine_hub_view *view, uint8_t *pixels) {
    static const engine_hub_view empty = {0};
    if (!view)
        view = &empty;
    memset(pixels, DRAW_BLACK, ENGINE_WIDTH * ENGINE_HEIGHT);
    background(pixels, view->tick);
    if (view->settings_open) {
        const engine_menu_view menu = {.settings = true, .values = view->settings, .ui = view->ui};
        engine_ui_menu(&menu, pixels);
        draw_mask_round(pixels);
        return;
    }
    if (view->catalogue && view->count) {
        const size_t index = view->selected < view->count ? view->selected : 0u;
        const int cx = 120 + view->slide_pixels;
        card(pixels, view->catalogue[index], cx, view, true);
        if (view->count > 1u) {
            const size_t previous = index == 0u ? view->count - 1u : index - 1u;
            const size_t next = index + 1u < view->count ? index + 1u : 0u;
            card(pixels, view->catalogue[previous], cx - 220, view, false);
            card(pixels, view->catalogue[next], cx + 220, view, false);
        }
    } else
        draw_centered(pixels, "NO GAMES", 78, DRAW_IVORY, 2);
    arrows(pixels, view->count > 1u);
    bracket_button(pixels, 60, 145, 180, 177, "PLAY", DRAW_CYAN, 2);
    bracket_button(pixels, 60, 184, 180, 208, "SETTINGS", DRAW_IVORY, 1);
    tilt_marker(pixels, &view->ui);
    draw_mask_round(pixels);
}
