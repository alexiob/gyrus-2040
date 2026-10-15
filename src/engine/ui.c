#include "engine/ui.h"
#include "engine/draw.h"
#include "gyrus/controls.h"
#include <string.h>

static void button(uint8_t *fb, int x0, int y0, int x1, int y1, const char *word, uint8_t color,
                   int scale) {
    draw_fill(fb, x0, y0, x1 - x0, y1 - y0, DRAW_BLACK);
    /* The entire visual border matches the controls module's half-open bounds. */
    draw_line(fb, x0, y0, x1 - 1, y0, DRAW_DARK);
    draw_line(fb, x0, y1 - 1, x1 - 1, y1 - 1, DRAW_DARK);
    draw_line(fb, x0, y0, x0, y1 - 1, DRAW_DARK);
    draw_line(fb, x1 - 1, y0, x1 - 1, y1 - 1, DRAW_DARK);
    draw_line(fb, x0, y0, x0 + 8, y0, color);
    draw_line(fb, x0, y0, x0, y0 + 5, color);
    draw_line(fb, x1 - 9, y1 - 1, x1 - 1, y1 - 1, color);
    draw_line(fb, x1 - 1, y1 - 6, x1 - 1, y1 - 1, color);
    draw_label(fb, word, (x0 + x1 - draw_text_width(word, scale)) / 2, (y0 + y1 - 7 * scale) / 2,
               color, scale);
}
static const char *mode_name(const engine_menu_view *g) {
    if (g->ui.control_mode == (uint8_t)CONTROLS_TILT)
        return "TILT";
    return g->ui.control_mode == (uint8_t)CONTROLS_RELATIVE_DRAG ? "BORDER DRAG" : "LEFT / RIGHT";
}
static void settings_mode(const engine_menu_view *g, uint8_t *fb) {
    const int y0 = (int)CONTROLS_SETTINGS_MODE_Y0;
    const int y1 = (int)CONTROLS_SETTINGS_MODE_Y1;
    const bool tilt = g->ui.control_mode == (uint8_t)CONTROLS_TILT;
    draw_centered(fb,
                  tilt ? (g->ui.tilt_ready ? "TILT READY - BANK TO TEST" : "HOLD STILL TO CENTRE")
                       : "STEERING",
                  42, DRAW_STEEL, 1);
    button(fb, (int)CONTROLS_SETTINGS_MODE_X0, y0, (int)CONTROLS_SETTINGS_MODE_X1, y1, mode_name(g),
           DRAW_CYAN, 1);
    draw_label(fb, "<", (int)CONTROLS_SETTINGS_MODE_X0 + 6, (y0 + y1 - 7) / 2, DRAW_STEEL, 1);
    draw_label(fb, ">", (int)CONTROLS_SETTINGS_MODE_X1 - 11, (y0 + y1 - 7) / 2, DRAW_STEEL, 1);
    if (tilt) {
        const int bar_y = y1 - 5;
        draw_fill(fb, 84, bar_y, 72, 2, DRAW_DARK);
        if (g->ui.tilt_ready) {
            int preview = g->ui.tilt_preview;
            if (preview < -1000)
                preview = -1000;
            if (preview > 1000)
                preview = 1000;
            const int marker = 120 + preview * 32 / 1000;
            draw_fill(fb, 116, bar_y - 2, 9, 5, DRAW_DEEP_BLUE);
            draw_line(fb, 120, bar_y, marker, bar_y, DRAW_CYAN);
            draw_fill(fb, marker - 1, bar_y - 2, 3, 5,
                      marker >= 117 && marker <= 123 ? DRAW_GREEN : DRAW_CYAN);
            draw_label(fb, "L", 72, y1 - 8, DRAW_STEEL, 1);
            draw_label(fb, "R", 163, y1 - 8, DRAW_STEEL, 1);
        } else {
            const unsigned progress = g->ui.tilt_progress > 1000u ? 1000u : g->ui.tilt_progress;
            draw_fill(fb, 84, bar_y, (int)(progress * 72u / 1000u), 2, DRAW_AMBER);
        }
    }
}
typedef struct {
    int x0, y0, x1, y1;
} settings_rect;
static void settings_row(uint8_t *fb, const char *name, uint32_t value, const char *unit,
                         int label_y, settings_rect down, settings_rect up, uint32_t minimum,
                         uint32_t maximum) {
    char number[11];
    char display[14];
    draw_decimal(number, value, 1);
    size_t length = strlen(number);
    memcpy(display, number, length);
    for (size_t i = 0; unit[i] != '\0' && length + 1u < sizeof display; ++i)
        display[length++] = unit[i];
    display[length] = '\0';
    draw_centered(fb, name, label_y, DRAW_STEEL, 1);
    button(fb, down.x0, down.y0, down.x1, down.y1, "-", value <= minimum ? DRAW_STEEL : DRAW_CYAN,
           2);
    button(fb, up.x0, up.y0, up.x1, up.y1, "+", value >= maximum ? DRAW_STEEL : DRAW_CYAN, 2);
    draw_centered(fb, display, (down.y0 + down.y1 - 14) / 2, DRAW_IVORY, 2);
}
static void settings_page(const engine_menu_view *g, uint8_t *fb) {
    static const settings_rect rows[3][2] = {
        {{(int)CONTROLS_FIRE_DOWN_X0, (int)CONTROLS_FIRE_DOWN_Y0, (int)CONTROLS_FIRE_DOWN_X1,
          (int)CONTROLS_FIRE_DOWN_Y1},
         {(int)CONTROLS_FIRE_UP_X0, (int)CONTROLS_FIRE_UP_Y0, (int)CONTROLS_FIRE_UP_X1,
          (int)CONTROLS_FIRE_UP_Y1}},
        {{(int)CONTROLS_ENEMY_DOWN_X0, (int)CONTROLS_ENEMY_DOWN_Y0, (int)CONTROLS_ENEMY_DOWN_X1,
          (int)CONTROLS_ENEMY_DOWN_Y1},
         {(int)CONTROLS_ENEMY_UP_X0, (int)CONTROLS_ENEMY_UP_Y0, (int)CONTROLS_ENEMY_UP_X1,
          (int)CONTROLS_ENEMY_UP_Y1}},
        {{(int)CONTROLS_TILT_DOWN_X0, (int)CONTROLS_TILT_DOWN_Y0, (int)CONTROLS_TILT_DOWN_X1,
          (int)CONTROLS_TILT_DOWN_Y1},
         {(int)CONTROLS_TILT_UP_X0, (int)CONTROLS_TILT_UP_Y0, (int)CONTROLS_TILT_UP_X1,
          (int)CONTROLS_TILT_UP_Y1}},
    };
    draw_centered(fb, "SETTINGS", 21, DRAW_IVORY, 2);
    settings_mode(g, fb);
    settings_row(fb, "FIRE RATE", g->values.fire_rate, "/S", 83, rows[0][0], rows[0][1], 1, 10);
    settings_row(fb, "ENEMY SPEED", g->values.enemy_speed_percent, "%", 126, rows[1][0], rows[1][1],
                 25, 200);
    settings_row(fb, "TILT SENS", g->values.tilt_sensitivity_percent, "%", 169, rows[2][0],
                 rows[2][1], 50, 200);
    button(fb, (int)CONTROLS_BACK_X0, (int)CONTROLS_BACK_Y0, (int)CONTROLS_BACK_X1,
           (int)CONTROLS_BACK_Y1, "BACK", DRAW_CYAN, 2);
}
static void pause_menu(const engine_menu_view *g, uint8_t *fb) {
    static const uint8_t dim[DRAW_COLOR_COUNT] = {
        DRAW_BLACK, DRAW_DARK, DRAW_DARK,      DRAW_DEEP_BLUE, DRAW_DEEP_BLUE, DRAW_DEEP_BLUE,
        DRAW_DARK,  DRAW_DARK, DRAW_DARK,      DRAW_DARK,      DRAW_DARK,      DRAW_DEEP_BLUE,
        DRAW_DARK,  DRAW_DARK, DRAW_DEEP_BLUE, DRAW_DARK,      DRAW_BLACK,     DRAW_DARK,
        DRAW_DARK,  DRAW_DARK, DRAW_DARK,      DRAW_DARK,      DRAW_DARK,      DRAW_DEEP_BLUE};
    for (unsigned i = 0; i < ENGINE_WIDTH * ENGINE_HEIGHT; ++i)
        fb[i] = fb[i] < DRAW_COLOR_COUNT ? dim[fb[i]] : DRAW_BLACK;
    draw_fill(fb, 35, 27, 170, 207, DRAW_BLACK);
    draw_centered(fb, "PAUSED", 38, DRAW_IVORY, 2);
    draw_centered(fb, "FLIGHT ON HOLD", 61, DRAW_STEEL, 1);
    button(fb, (int)CONTROLS_RESUME_X0, (int)CONTROLS_RESUME_Y0, (int)CONTROLS_RESUME_X1,
           (int)CONTROLS_RESUME_Y1, "RESUME", DRAW_CYAN, 2);
    button(fb, (int)CONTROLS_SETTINGS_X0, (int)CONTROLS_SETTINGS_Y0, (int)CONTROLS_SETTINGS_X1,
           (int)CONTROLS_SETTINGS_Y1, "SETTINGS", DRAW_IVORY, 2);
    button(fb, (int)CONTROLS_QUIT_X0, (int)CONTROLS_QUIT_Y0, (int)CONTROLS_QUIT_X1,
           (int)CONTROLS_QUIT_Y1, "QUIT", DRAW_IVORY, 2);
    button(fb, (int)CONTROLS_REBOOT_X0, (int)CONTROLS_REBOOT_Y0, (int)CONTROLS_REBOOT_X1,
           (int)CONTROLS_REBOOT_Y1, "REBOOT HOLD 5S", DRAW_AMBER, 1);
    const unsigned progress = g->ui.reboot_progress > 1000u ? 1000u : g->ui.reboot_progress;
    if (progress > 0u) {
        const int width = (int)(progress * (CONTROLS_REBOOT_X1 - CONTROLS_REBOOT_X0 - 4u) / 1000u);
        draw_fill(fb, (int)CONTROLS_REBOOT_X0 + 2, (int)CONTROLS_REBOOT_Y1 - 4, width, 2,
                  DRAW_AMBER);
    }
    draw_centered(fb, "BOOT ALSO RESUMES", 219, DRAW_STEEL, 1);
}
void engine_ui_menu(const engine_menu_view *view, uint8_t *pixels) {
    if (view->settings) {
        draw_fill(pixels, 35, 15, 170, 219, DRAW_BLACK);
        settings_page(view, pixels);
    } else {
        pause_menu(view, pixels);
    }
}

void engine_ui_home_button(uint8_t *pixels) {
    button(pixels, 14, 94, 48, 128, "<", DRAW_CYAN, 2);
}

/* Authored stroke wordmarks, independent of the small menu bitmap font. A
   three-tone bevel makes Gyrus read as chrome without a stored bitmap. */
typedef struct {
    int8_t x, y;
} title_point;
typedef struct {
    const title_point *points;
    unsigned count;
} title_path;
#define PATH(a) {a, sizeof(a) / sizeof((a)[0])}
static void title_stroke(uint8_t *pixels, int x, int y, const title_point *points, unsigned count,
                         bool chrome, uint8_t accent) {
    for (unsigned i = 1; i < count; ++i) {
        const int ax = x + points[i - 1u].x, ay = y + points[i - 1u].y;
        const int bx = x + points[i].x, by = y + points[i].y;
        draw_line(pixels, ax + 2, ay + 3, bx + 2, by + 3, DRAW_DEEP_BLUE);
        if (chrome) {
            draw_line(pixels, ax, ay + 1, bx, by + 1, DRAW_BLUE);
            draw_line(pixels, ax - 1, ay, bx - 1, by, DRAW_STEEL);
            draw_line(pixels, ax + 1, ay, bx + 1, by, DRAW_PALE_BLUE);
            draw_line(pixels, ax, ay - 1, bx, by - 1, DRAW_WHITE);
        }
        draw_line(pixels, ax, ay, bx, by, accent);
    }
}
void engine_title_gyrus(uint8_t *pixels, int cx, int cy, uint32_t tick, bool neon) {
    static const title_point g[] = {{22, 0}, {4, 0},   {0, 4},   {0, 24},
                                    {4, 28}, {22, 28}, {22, 15}, {12, 15}};
    static const title_point y[] = {{0, 0}, {11, 13}, {22, 0}};
    static const title_point y_stem[] = {{11, 13}, {11, 28}};
    static const title_point r[] = {{0, 28}, {0, 0}, {18, 0}, {22, 4}, {22, 10}, {18, 14}, {0, 14}};
    static const title_point r_leg[] = {{12, 14}, {23, 28}};
    static const title_point u[] = {{0, 0}, {0, 24}, {4, 28}, {18, 28}, {22, 24}, {22, 0}};
    static const title_point s[] = {{22, 0},  {4, 0},   {0, 4},   {0, 10},  {4, 14},
                                    {18, 14}, {22, 18}, {22, 24}, {18, 28}, {0, 28}};
    static const title_path paths[] = {PATH(g),     PATH(y), PATH(y_stem), PATH(r),
                                       PATH(r_leg), PATH(u), PATH(s)};
    static const int8_t offsets[] = {0, 29, 29, 58, 58, 87, 116};
    const int left = cx - 69, top = cy - 16;
    const uint8_t accent = neon && ((tick / 24u) & 1u) ? DRAW_CYAN : DRAW_IVORY;
    for (unsigned i = 0; i < sizeof paths / sizeof paths[0]; ++i)
        title_stroke(pixels, left + offsets[i], top, paths[i].points, paths[i].count, true, accent);
    /* Small vector wings make the silhouette recognizable even in a slide. */
    draw_line(pixels, cx - 79, cy + 12, cx - 72, cy + 7, DRAW_AMBER);
    draw_line(pixels, cx + 75, cy + 12, cx + 82, cy + 7, DRAW_AMBER);
    draw_line(pixels, cx - 60, cy + 18, cx + 61, cy + 18, DRAW_BLUE);
}
void engine_title_tempest(uint8_t *pixels, int cx, int cy, uint32_t tick, bool neon) {
    static const title_point t[] = {{0, 0}, {16, 0}, {8, 0}, {8, 28}};
    static const title_point e[] = {{16, 0}, {0, 0}, {0, 28}, {16, 28}};
    static const title_point e_mid[] = {{0, 14}, {13, 14}};
    static const title_point m[] = {{0, 28}, {0, 0}, {8, 12}, {16, 0}, {16, 28}};
    static const title_point p[] = {{0, 28}, {0, 0}, {13, 0}, {16, 4}, {16, 10}, {12, 14}, {0, 14}};
    static const title_point s[] = {{16, 0},  {3, 0},   {0, 3},   {0, 11},  {3, 14},
                                    {13, 14}, {16, 17}, {16, 25}, {13, 28}, {0, 28}};
    const title_path letters[] = {PATH(t), PATH(e), PATH(m), PATH(p), PATH(e), PATH(s), PATH(t)};
    const int left = cx - 71, top = cy - 16;
    const uint8_t accent = neon && ((tick / 18u) & 1u) ? DRAW_AMBER : DRAW_CYAN;
    /* An angular perspective cage is decoration, never a gameplay web. */
    const int vx[] = {-80, -57, 57, 80, 57, -57}, vy[] = {-2, -23, -23, -2, 24, 24};
    for (unsigned i = 0; i < 6; ++i) {
        unsigned j = (i + 1u) % 6u;
        draw_line(pixels, cx + vx[i], cy + vy[i], cx + vx[j], cy + vy[j], DRAW_DEEP_BLUE);
        draw_line(pixels, cx + vx[i], cy + vy[i], cx + vx[i] / 4, cy + vy[i] / 4, DRAW_DEEP_BLUE);
    }
    for (unsigned i = 0; i < 7; ++i) {
        title_stroke(pixels, left + (int)i * 21, top, letters[i].points, letters[i].count, false,
                     accent);
        if (i == 1u || i == 4u)
            title_stroke(pixels, left + (int)i * 21, top, e_mid, sizeof e_mid / sizeof e_mid[0],
                         false, accent);
        /* One-pixel highlight is deliberate: airy angular forms, not bitmap type. */
        draw_line(pixels, left + (int)i * 21, top - 1, left + (int)i * 21 + 12, top - 1,
                  DRAW_PALE_BLUE);
    }
}
#undef PATH
