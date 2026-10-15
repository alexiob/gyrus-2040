#include "engine/draw.h"
#include "engine/math.h"
#include "tempest/game.h"
#include <string.h>
static void stroke(uint8_t *p, int x, int y, int xx, int yy, uint8_t c, bool neon) {
    if (neon) {
        draw_line(p, x + 1, y, xx + 1, yy, DRAW_DEEP_BLUE);
        draw_line(p, x, y + 1, xx, yy + 1, DRAW_DEEP_BLUE);
    }
    draw_line(p, x, y, xx, yy, c);
}
static uint8_t web_color(const tempest_state *s) {
    static const uint8_t colors[] = {DRAW_BLUE, DRAW_RED,   DRAW_YELLOW,
                                     DRAW_CYAN, DRAW_BLACK, DRAW_GREEN};
    unsigned cycle = (s->stage - 1) / 16u;
    if (cycle > 5)
        cycle = 5;
    if (s->zap_flash)
        return s->zap_flash & 2u ? DRAW_WHITE : DRAW_YELLOW;
    if (s->phase == TEMPEST_CLEAR && s->phase_ticks % 8u < 4u)
        return DRAW_WHITE;
    return colors[cycle];
}
static void ring(uint8_t *p, const tempest_web *w, int32_t depth, uint8_t c, bool neon) {
    for (unsigned i = 0; i < w->lane_count; i++) {
        int x, y, xx, yy;
        tempest_project(w, (int)i * 256, depth, &x, &y);
        tempest_project(w, (int)(i + 1) * 256, depth, &xx, &yy);
        stroke(p, x, y, xx, yy, c, neon);
    }
}
/* Transition transforms are display-only; collision remains in lane/depth space. */
static void scaled_web(uint8_t *p, const tempest_web *w, int scale, uint8_t color, bool neon) {
    for (unsigned i = 0; i < w->rail_count; i++) {
        int x, y, xx, yy;
        tempest_project(w, (int)i * 256, 0, &x, &y);
        tempest_project(w, (int)i * 256, TEMPEST_DEPTH, &xx, &yy);
        stroke(p, 120 + (x - 120) * scale / 256, 104 + (y - 104) * scale / 256,
               120 + (xx - 120) * scale / 256, 104 + (yy - 104) * scale / 256, color, neon);
    }
    for (unsigned i = 0; i < w->lane_count; i++) {
        int x, y, xx, yy;
        tempest_project(w, (int)i * 256, TEMPEST_DEPTH, &x, &y);
        tempest_project(w, (int)(i + 1) * 256, TEMPEST_DEPTH, &xx, &yy);
        stroke(p, 120 + (x - 120) * scale / 256, 104 + (y - 104) * scale / 256,
               120 + (xx - 120) * scale / 256, 104 + (yy - 104) * scale / 256, color, neon);
    }
}
static void travel_streaks(uint8_t *p, const tempest_state *s, bool exiting) {
    const tempest_web *w = tempest_current_web(s);
    unsigned count = s->effects ? 12u : 6u;
    for (unsigned i = 0; i < count; i++) {
        int lane = (int)(i * w->lane_count * 256u / count);
        int32_t d = (int32_t)((s->phase_ticks * 1700u + i * 5300u) % 65536u);
        if (exiting)
            d = TEMPEST_DEPTH - d;
        int x, y, xx, yy;
        tempest_project(w, lane, d, &x, &y);
        tempest_project(w, lane, d + 9000, &xx, &yy);
        stroke(p, x, y, xx, yy, i & 1u ? DRAW_STEEL : DRAW_CYAN, s->effects);
    }
}
static void event_feedback(uint8_t *p, const tempest_state *s) {
    const tempest_web *w = tempest_current_web(s);
    for (unsigned i = 0; i < TEMPEST_VISUAL_EVENTS; i++) {
        const tempest_visual_event *v = &s->visual[i];
        if (!v->life)
            continue;
        int x, y;
        tempest_project(w, v->lane_q8, v->depth, &x, &y);
        uint8_t color = v->kind == TEMPEST_FX_SPLIT   ? DRAW_MAGENTA
                        : v->kind == TEMPEST_FX_LAND  ? DRAW_CYAN
                        : v->kind == TEMPEST_FX_PULSE ? DRAW_YELLOW
                                                      : DRAW_ORANGE;
        if (v->age < 4)
            color = DRAW_WHITE;
        unsigned count = s->effects ? 6u : 4u;
        int radius = 2 + v->age / 2;
        if (v->kind == TEMPEST_FX_MUZZLE) {
            radius = 3;
            count = 4;
            color = DRAW_YELLOW;
        }
        if (v->kind == TEMPEST_FX_ZAP) {
            radius = 5 + v->age * 3;
            count = s->effects ? 8u : 4u;
            color = DRAW_YELLOW;
        }
        for (unsigned j = 0; j < count; j++) {
            uint16_t angle =
                (uint16_t)(j * 65536u / count + (s->effects ? (unsigned)v->age * 1000u : 0u));
            int dx = engine_sin_q15(angle) * radius / 32768,
                dy = engine_cos_q15(angle) * radius / 32768;
            int inner = v->kind == TEMPEST_FX_LAND ? radius - 1 : radius / 2;
            int ix = engine_sin_q15(angle) * inner / 32768,
                iy = engine_cos_q15(angle) * inner / 32768;
            stroke(p, x + ix, y + iy, x + dx, y + dy, color, s->effects);
            if (s->effects)
                draw_pixel(p, x + dx * 3 / 2, y + dy * 3 / 2, DRAW_CYAN);
        }
    }
    if (s->zap_flash) {
        int32_t depth = (18 - (int32_t)s->zap_flash) * TEMPEST_DEPTH / 18;
        ring(p, w, depth, DRAW_WHITE, s->effects);
    }
}
static void web(uint8_t *p, const tempest_state *s) {
    const tempest_web *w = tempest_current_web(s);
    uint8_t c = web_color(s);
    if (s->phase == TEMPEST_TITLE || s->phase == TEMPEST_OVER)
        c = DRAW_DEEP_BLUE;
    ring(p, w, TEMPEST_DEPTH, c, s->effects);
    ring(p, w, 0, c, false);
    for (unsigned i = 0; i < w->rail_count; i++) {
        int x, y, xx, yy;
        tempest_project(w, (int)i * 256, 0, &x, &y);
        tempest_project(w, (int)i * 256, TEMPEST_DEPTH, &xx, &yy);
        stroke(p, x, y, xx, yy, c, s->effects);
    }
    unsigned selected = (unsigned)s->claw_q8 / 256;
    for (unsigned k = 0; k < 2; k++) {
        int x, y, xx, yy;
        tempest_project(w, (int)(selected + k) * 256, 0, &x, &y);
        tempest_project(w, (int)(selected + k) * 256, TEMPEST_DEPTH, &xx, &yy);
        draw_line(p, x, y, xx, yy, DRAW_STEEL);
    }
    for (unsigned i = 0; i < w->lane_count; i++)
        if (s->spikes[i] > 0) {
            int x, y, xx, yy;
            tempest_project(w, (int)i * 256 + 128, 0, &x, &y);
            tempest_project(w, (int)i * 256 + 128, s->spikes[i], &xx, &yy);
            stroke(p, x, y, xx, yy, DRAW_GREEN, s->effects);
            draw_pixel(p, xx + 1, yy, DRAW_LIME);
        }
}
/* Small authored vector glyphs in lane-tangent / outward coordinates. */
typedef struct {
    uint8_t *pixels;
    int x, y, tx, ty, nx, ny;
    bool neon;
    uint8_t color;
} glyph;
static void segment(const glyph *g, int x, int y, int xx, int yy) {
    stroke(g->pixels, g->x + (g->tx * x + g->nx * y) / 256, g->y + (g->ty * x + g->ny * y) / 256,
           g->x + (g->tx * xx + g->nx * yy) / 256, g->y + (g->ty * xx + g->ny * yy) / 256, g->color,
           g->neon);
}
static int abs_int(int n) {
    return n < 0 ? -n : n;
}
static glyph glyph_at(uint8_t *p, const tempest_state *s, int lane, int32_t depth, uint8_t c) {
    const tempest_web *w = tempest_current_web(s);
    int x, y, a, b, aa, bb;
    tempest_project(w, lane, depth, &x, &y);
    tempest_project(w, lane - 80, TEMPEST_DEPTH, &a, &b);
    tempest_project(w, lane + 80, TEMPEST_DEPTH, &aa, &bb);
    int tx = aa - a, ty = bb - b, scale = abs_int(tx) > abs_int(ty) ? abs_int(tx) : abs_int(ty);
    if (!scale)
        scale = 1;
    tx = tx * 256 / scale;
    ty = ty * 256 / scale;
    return (glyph){p, x, y, tx, ty, -ty, tx, s->effects, c};
}
static void enemy(uint8_t *p, const tempest_state *s, const tempest_enemy *e) {
    int lane = e->lane_q8 + (e->kind == TEMPEST_FUSEBALL ? 0 : 128);
    if (e->crossing)
        lane += e->direction * e->timer;
    static const uint8_t colors[] = {DRAW_RED, DRAW_MAGENTA, DRAW_GREEN, DRAW_WHITE, DRAW_YELLOW};
    if (!e->landed) {
        int x, y;
        tempest_project(tempest_current_web(s), lane, 0, &x, &y);
        draw_pixel(p, x, y, DRAW_RED);
        return;
    }
    int size = 3 + e->depth / 18000;
    glyph g = glyph_at(p, s, lane, e->depth, colors[e->kind < 5 ? e->kind : 0]);
    switch (e->kind) {
    case TEMPEST_FLIPPER:
        segment(&g, -size, -size / 2, size, size / 2);
        segment(&g, size, size / 2, size, -size / 2);
        segment(&g, size, -size / 2, -size, size / 2);
        segment(&g, -size, size / 2, -size, -size / 2);
        break;
    case TEMPEST_TANKER:
        segment(&g, 0, -size, -size, 0);
        segment(&g, -size, 0, 0, size);
        segment(&g, 0, size, size, 0);
        segment(&g, size, 0, 0, -size);
        segment(&g, -size / 2, 0, size / 2, 0);
        segment(&g, 0, -size / 2, 0, size / 2);
        break;
    case TEMPEST_SPIKER:
        segment(&g, -size, -size / 2, 0, -size);
        segment(&g, 0, -size, size, -size / 2);
        segment(&g, size, -size / 2, size / 2, size);
        segment(&g, size / 2, size, -size / 2, size);
        segment(&g, -size / 2, size, -size, -size / 2);
        segment(&g, -size, -size / 2, size / 2, 0);
        segment(&g, size / 2, 0, 0, size / 2);
        break;
    case TEMPEST_FUSEBALL:
        for (unsigned i = 0; i < 8; i++) {
            int angle = (int)(i * 8192u + s->tick * 700u);
            int dx = engine_sin_q15((uint16_t)angle) * size / 32768,
                dy = engine_cos_q15((uint16_t)angle) * size / 32768;
            segment(&g, dx / 3, dy / 3, dx, dy);
        }
        draw_pixel(p, g.x, g.y, DRAW_YELLOW);
        break;
    case TEMPEST_PULSAR:
        if (e->age % 150u >= 120u) {
            int x, y, xx, yy;
            int l = e->lane_q8 / 256;
            tempest_project(tempest_current_web(s), l * 256, 0, &x, &y);
            tempest_project(tempest_current_web(s), l * 256, TEMPEST_DEPTH, &xx, &yy);
            draw_line(p, x, y, xx, yy, DRAW_YELLOW);
            tempest_project(tempest_current_web(s), (l + 1) * 256, 0, &x, &y);
            tempest_project(tempest_current_web(s), (l + 1) * 256, TEMPEST_DEPTH, &xx, &yy);
            draw_line(p, x, y, xx, yy, DRAW_YELLOW);
        }
        segment(&g, -size, 0, -size / 2, -size / 2);
        segment(&g, -size / 2, -size / 2, 0, size / 2);
        segment(&g, 0, size / 2, size / 2, -size / 2);
        segment(&g, size / 2, -size / 2, size, 0);
        break;
    }
    if (s->effects) {
        /* Cosmetic sparks derived from tick/entity coordinates, never gameplay RNG. */
        unsigned phase = (s->tick + (unsigned)lane) % 13u;
        for (unsigned i = 0; i < 3; i++)
            draw_pixel(p, g.x + (int)(i * 7u + phase) % 15 - 7,
                       g.y + (int)(i * 11u + phase) % 13 - 6, DRAW_TEAL);
    }
}
static void claw_shape(const glyph *g, int size) {
    segment(g, -size, 2, -size, -5);
    segment(g, -size, -5, -2, 0);
    segment(g, -2, 0, 2, 0);
    segment(g, 2, 0, size, -5);
    segment(g, size, -5, size, 2);
    segment(g, -size, 2, 0, 5);
    segment(g, 0, 5, size, 2);
    segment(g, 0, 0, 0, -6);
}
static void claw(uint8_t *p, const tempest_state *s) {
    int32_t depth = s->phase == TEMPEST_DIVE ? s->dive_depth
                    : s->phase == TEMPEST_INTRO
                        ? (int32_t)s->phase_ticks * TEMPEST_DEPTH / (int32_t)TEMPEST_INTRO_TICKS
                        : TEMPEST_DEPTH;
    glyph g = glyph_at(p, s, s->claw_q8 + 128, depth, DRAW_YELLOW);
    if (s->invulnerable && s->tick % 8u < 3)
        g.color = DRAW_WHITE;
    int size = s->phase == TEMPEST_DIVE || s->phase == TEMPEST_INTRO ? 3 + depth / 14000 : 7;
    /* Render the player above the HUD/messages. A one-pixel black silhouette
     * separates its silhouette from the well, effects and overlapping text. */
    glyph outline = g;
    outline.color = DRAW_BLACK;
    outline.neon = false;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            if (!dx && !dy)
                continue;
            outline.x = g.x + dx;
            outline.y = g.y + dy;
            claw_shape(&outline, size);
        }
    g.neon = false;
    claw_shape(&g, size);
}
static void hud(uint8_t *p, const tempest_state *s) {
    char text[11];
    draw_decimal(text, s->score, 6);
    draw_centered(p, text, 14, DRAW_WHITE, 1);
    draw_label(p, "WEB", 52, 29, DRAW_STEEL, 1);
    draw_decimal(text, s->stage, 2);
    draw_label(p, text, 77, 29, DRAW_CYAN, 1);
    for (unsigned i = 0; i < s->lives; i++) {
        int x = 144 + (int)i * 9;
        draw_line(p, x, 30, x + 3, 34, DRAW_YELLOW);
        draw_line(p, x + 3, 34, x + 6, 30, DRAW_YELLOW);
    }
    if (s->phase == TEMPEST_PLAY || s->phase == TEMPEST_DIVE) {
        uint8_t c = s->zaps ? DRAW_CYAN : DRAW_STEEL;
        draw_fill(p, 90, 176, 60, 26, DRAW_DARK);
        draw_line(p, 90, 176, 149, 176, c);
        draw_line(p, 90, 201, 149, 201, c);
        draw_line(p, 90, 176, 90, 201, c);
        draw_line(p, 149, 176, 149, 201, c);
        draw_label(p, "ZAP", 100, 185, c, 1);
        draw_decimal(text, s->zaps, 1);
        draw_label(p, text, 130, 185, DRAW_YELLOW, 1);
        draw_centered(p, tempest_current_web(s)->name, 212, DRAW_STEEL, 1);
    }
}
void tempest_render(const tempest_state *s, uint8_t *p) {
    memset(p, DRAW_BLACK, ENGINE_WIDTH * ENGINE_HEIGHT);
    if (s->settings_open || s->phase == TEMPEST_PAUSED) {
        engine_menu_view menu = {.settings = s->settings_open, .values = s->settings, .ui = s->ui};
        engine_ui_menu(&menu, p);
        draw_mask_round(p);
        return;
    }
    if (s->phase == TEMPEST_INTRO) {
        const tempest_web *w = tempest_current_web(s);
        int t = s->phase_ticks;
        if (t > 60)
            t = 60;
        int ease = t * t * (180 - 2 * t) / 844;
        scaled_web(p, w, 40 + 216 * ease / 256, web_color(s), s->effects);
        travel_streaks(p, s, false);
        if (t > 45)
            ring(p, w, TEMPEST_DEPTH, DRAW_CYAN, s->effects);
        hud(p, s);
        draw_centered(p, "ENTERING WEB", 180, DRAW_WHITE, 1);
        draw_centered(p, w->name, 195, DRAW_CYAN, 1);
        claw(p, s);
        draw_mask_round(p);
        return;
    }
    if (s->phase == TEMPEST_FLIGHT) {
        const tempest_web *w = tempest_current_web(s);
        unsigned next_stage = s->stage == 4095 ? 1u : s->stage + 1u;
        const tempest_web *next = &tempest_webs[(next_stage - 1u) % 16u];
        int t = s->phase_ticks;
        if (t > 60)
            t = 60;
        /* Exit only recedes the current web. The next web grows exactly once
         * during INTRO, starting at the same40/256 scale as this flight ends. */
        scaled_web(p, w, 256 - 216 * t / 60, DRAW_BLUE, s->effects);
        travel_streaks(p, s, true);
        draw_fill(p, 40, 177, 160, 30, DRAW_BLACK);
        draw_centered(p, "SUPERZAPPER RECHARGE", 180, DRAW_YELLOW, 1);
        draw_centered(p, next->name, 195, DRAW_CYAN, 1);
        hud(p, s);
        draw_mask_round(p);
        return;
    }
    web(p, s);
    if (s->phase == TEMPEST_TITLE || s->phase == TEMPEST_OVER) {
        draw_fill(p, 28, 42, 184, 139, DRAW_BLACK);
        engine_title_tempest(p, 120, 66, s->tick, s->effects);
        draw_centered(p, s->phase == TEMPEST_OVER ? "GAME OVER" : "CLASSIC VECTOR ARCADE", 83,
                      DRAW_STEEL, 1);
        draw_fill(p, 60, 96, 120, 36, DRAW_DEEP_BLUE);
        draw_centered(p, "PLAY", 107, DRAW_CYAN, 2);
        draw_centered(p, "SETTINGS", 154, DRAW_WHITE, 1);
        engine_ui_home_button(p);
        char score[11];
        draw_decimal(score, s->high_score, 6);
        draw_centered(p, "HIGH SCORE", 185, DRAW_STEEL, 1);
        draw_centered(p, score, 197, DRAW_YELLOW, 1);
        if (s->phase == TEMPEST_OVER) {
            draw_decimal(score, s->score, 6);
            draw_centered(p, score, 28, DRAW_WHITE, 1);
        }
        draw_mask_round(p);
        return;
    }
    for (unsigned i = 0; i < TEMPEST_ENEMIES; i++)
        if (s->enemies[i].active)
            enemy(p, s, &s->enemies[i]);
    const tempest_web *w = tempest_current_web(s);
    if (s->phase == TEMPEST_DIVE)
        travel_streaks(p, s, true);
    for (unsigned i = 0; i < TEMPEST_SHOTS; i++)
        if (s->shots[i].active) {
            int x, y, xx, yy;
            const tempest_shot *b = &s->shots[i];
            tempest_project(w, b->lane * 256 + 128, b->depth, &x, &y);
            tempest_project(w, b->lane * 256 + 128, b->depth + 2400, &xx, &yy);
            if (s->effects) {
                int tx, ty;
                tempest_project(w, b->lane * 256 + 128, b->depth + 9000, &tx, &ty);
                draw_line(p, xx, yy, tx, ty, DRAW_DEEP_BLUE);
            }
            stroke(p, x, y, xx, yy, DRAW_WHITE, s->effects);
        }
    for (unsigned i = 0; i < TEMPEST_HOSTILE; i++)
        if (s->hostile[i].active) {
            int x, y;
            const tempest_shot *b = &s->hostile[i];
            tempest_project(w, b->lane * 256 + 128, b->depth, &x, &y);
            draw_fill(p, x - 1, y - 1, 3, 3, DRAW_RED);
        }
    if (s->phase == TEMPEST_DEATH) {
        int x, y;
        tempest_project(w, s->claw_q8 + 128, s->dive_depth, &x, &y);
        int radius = (int)s->phase_ticks / 3 + 3;
        for (unsigned i = 0; i < 8; i++) {
            int dx = engine_sin_q15((uint16_t)(i * 8192u)) * radius / 32768,
                dy = engine_cos_q15((uint16_t)(i * 8192u)) * radius / 32768;
            draw_line(p, x + dx / 2, y + dy / 2, x + dx, y + dy, DRAW_ORANGE);
        }
    }
    event_feedback(p, s);
    hud(p, s);
    if (s->phase == TEMPEST_CLEAR)
        draw_centered(p, "WEB CLEAR", 90, DRAW_WHITE, 1);
    if (s->phase == TEMPEST_DIVE)
        draw_centered(p, "AVOID SPIKES", 163, DRAW_YELLOW, 1);
    if (s->phase != TEMPEST_DEATH)
        claw(p, s);
    draw_mask_round(p);
}
