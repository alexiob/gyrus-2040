#include "gyrus/assets.h"
#include "gyrus/controls.h"
#include "gyrus/game.h"
#include <string.h>

enum {
    BLACK = GYRUS_COLOR_BLACK,
    WHITE = GYRUS_COLOR_WHITE,
    IVORY = GYRUS_COLOR_IVORY,
    CYAN = GYRUS_COLOR_CYAN,
    BLUE = GYRUS_COLOR_BLUE,
    DEEP_BLUE = GYRUS_COLOR_DEEP_BLUE,
    AMBER = GYRUS_COLOR_AMBER,
    ORANGE = GYRUS_COLOR_ORANGE,
    RED = GYRUS_COLOR_RED,
    MAGENTA = GYRUS_COLOR_MAGENTA,
    GREEN = GYRUS_COLOR_GREEN,
    STEEL = GYRUS_COLOR_STEEL,
    DARK = GYRUS_COLOR_DARK_STEEL,
    YELLOW = GYRUS_COLOR_YELLOW
};

static void pixel(uint8_t *fb, int x, int y, uint8_t color) {
    if ((unsigned)x < GAME_WIDTH && (unsigned)y < GAME_HEIGHT)
        fb[(unsigned)y * GAME_WIDTH + (unsigned)x] = color;
}
static void fill(uint8_t *fb, int x, int y, int width, int height, uint8_t color) {
    for (int yy = y; yy < y + height; ++yy)
        for (int xx = x; xx < x + width; ++xx)
            pixel(fb, xx, yy, color);
}
static void line(uint8_t *fb, int x0, int y0, int x1, int y1, uint8_t color) {
    const int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    const int dy = y1 > y0 ? y0 - y1 : y1 - y0;
    const int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    for (;;) {
        pixel(fb, x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;
        const int twice = 2 * error;
        if (twice >= dy) {
            error += dy;
            x0 += sx;
        }
        if (twice <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}
static int text_width(const char *string, int scale) {
    int count = 0;
    while (*string != '\0') {
        ++count;
        ++string;
    }
    return count > 0 ? (count * 6 - 1) * scale : 0;
}
static void label(uint8_t *fb, const char *string, int x, int y, uint8_t color, int scale) {
    while (*string != '\0') {
        const uint8_t *glyph = gyrus_glyph(*string++);
        for (unsigned row = 0; row < 7u; ++row)
            for (unsigned col = 0; col < 5u; ++col)
                if ((glyph[row] & (1u << (4u - col))) != 0u)
                    fill(fb, x + (int)col * scale, y + (int)row * scale, scale, scale, color);
        x += 6 * scale;
    }
}
static void centered(uint8_t *fb, const char *string, int y, uint8_t color, int scale) {
    label(fb, string, 120 - text_width(string, scale) / 2, y, color, scale);
}
static void decimal(char result[11], uint32_t number, unsigned digits) {
    char reversed[10];
    unsigned used = 0;
    do {
        reversed[used++] = (char)('0' + number % 10u);
        number /= 10u;
    } while (number != 0u && used < 10u);
    while (used < digits && used < 10u)
        reversed[used++] = '0';
    for (unsigned i = 0; i < used; ++i)
        result[i] = reversed[used - 1u - i];
    result[used] = '\0';
}
static void sprite_scaled(uint8_t *fb, gyrus_sprite_id id, unsigned frame, int cx, int cy,
                          int scale) {
    const gyrus_asset *asset = gyrus_sprite(id, frame);
    if (asset == NULL)
        return;
    const int left = cx - (int)asset->width * scale / 2;
    const int top = cy - (int)asset->height * scale / 2;
    for (unsigned y = 0; y < asset->height; ++y)
        for (unsigned x = 0; x < asset->width; ++x) {
            const uint8_t color = asset->pixels[y * (unsigned)asset->width + x];
            if (color != 0u)
                fill(fb, left + (int)x * scale, top + (int)y * scale, scale, scale, color);
        }
}
static void sprite_resized(uint8_t *fb, gyrus_sprite_id id, unsigned frame, int cx, int cy,
                           unsigned width) {
    const gyrus_asset *asset = gyrus_sprite(id, frame);
    if (asset == NULL || width == 0u)
        return;
    const unsigned height = (width * asset->height + asset->width / 2u) / asset->width;
    if (height == 0u)
        return;
    for (unsigned y = 0; y < height; ++y) {
        const unsigned sy = ((2u * y + 1u) * asset->height) / (2u * height);
        for (unsigned x = 0; x < width; ++x) {
            const unsigned sx = ((2u * x + 1u) * asset->width) / (2u * width);
            const uint8_t color = asset->pixels[sy * asset->width + sx];
            if (color != 0u)
                pixel(fb, cx - (int)width / 2 + (int)x, cy - (int)height / 2 + (int)y, color);
        }
    }
}
static unsigned transition_progress(const game_state *g, unsigned duration) {
    const unsigned elapsed = g->phase_ticks < duration ? g->phase_ticks : duration;
    return elapsed * 1000u / duration;
}
static void sprite(uint8_t *fb, gyrus_sprite_id id, unsigned frame, int cx, int cy) {
    sprite_scaled(fb, id, frame, cx, cy, 1);
}
static gyrus_sprite_id planet_sprite(unsigned planet) {
    static const gyrus_sprite_id route[] = {
        GYRUS_SPRITE_PLANET_NEPTUNE, GYRUS_SPRITE_PLANET_URANUS, GYRUS_SPRITE_PLANET_SATURN,
        GYRUS_SPRITE_PLANET_JUPITER, GYRUS_SPRITE_PLANET_MARS,   GYRUS_SPRITE_PLANET_EARTH};
    return route[planet % 6u];
}
static void stars(const game_state *g, uint8_t *fb, bool streaks) {
    static const uint8_t colors[] = {DARK, DEEP_BLUE, STEEL, DARK};
    for (unsigned i = 0; i < GAME_STARS; ++i) {
        const game_star *star = &g->stars[i];
        int16_t x, y;
        game_polar(star->angle, (int32_t)star->radius, &x, &y);
        uint8_t color = colors[star->color % 4u];
        if ((g->tick / 24u + i) % 17u == 0u)
            color = STEEL;
        if (streaks && star->radius > 40u * 256u) {
            int16_t tx, ty;
            game_polar(star->angle, (int32_t)star->radius - (int32_t)star->speed * 768, &tx, &ty);
            line(fb, tx, ty, x, y, color);
        } else
            pixel(fb, x, y, color);
    }
}
static void travel_stars(const game_state *g, uint8_t *fb, game_phase phase) {
    const unsigned progress =
        transition_progress(g, phase == GAME_WARP ? GAME_WARP_TICKS : GAME_ARRIVAL_TICKS);
    const unsigned length =
        phase == GAME_WARP ? 2u + progress * progress * 24u / 1000000u : 4u - progress * 3u / 1000u;
    for (unsigned i = 0; i < GAME_STARS; ++i) {
        const game_star *star = &g->stars[i];
        const int32_t tail = (int32_t)star->radius - (int32_t)(length * 256u);
        int16_t x, y, tx, ty;
        game_polar(star->angle, star->radius, &x, &y);
        game_polar(star->angle, tail > 0 ? tail : 0, &tx, &ty);
        const uint8_t color = i % 11u == 0u ? CYAN : (i % 3u == 0u ? STEEL : DEEP_BLUE);
        line(fb, tx, ty, x, y, color);
    }
}
static void orbit_ticks(uint8_t *fb, uint8_t color) {
    for (unsigned i = 0; i < 64u; ++i) {
        int16_t x, y;
        game_polar((uint16_t)(i * 1024u), 113 * 256, &x, &y);
        pixel(fb, x, y, color);
    }
}
static void route(const game_state *g, uint8_t *fb, int y) {
    static const char *const letters[] = {"N", "U", "S", "J", "M", "E"};
    const unsigned current = (unsigned)g->planet % 6u;
    line(fb, 65, y, 175, y, DARK);
    for (unsigned i = 0; i < 6u; ++i) {
        const int x = 65 + (int)i * 22;
        const uint8_t color = i == current ? CYAN : (i < current ? STEEL : DARK);
        fill(fb, x - 2, y - 2, 5, 5, BLACK);
        if (i == current) {
            line(fb, x, y - 3, x + 3, y, CYAN);
            line(fb, x + 3, y, x, y + 3, CYAN);
            line(fb, x, y + 3, x - 3, y, CYAN);
            line(fb, x - 3, y, x, y - 3, CYAN);
        } else
            fill(fb, x - 1, y - 1, 3, 3, color);
        label(fb, letters[i], x - 2, y + 9, color, 1);
    }
}
static void button(uint8_t *fb, int x0, int y0, int x1, int y1, const char *word, uint8_t color,
                   int scale) {
    fill(fb, x0, y0, x1 - x0, y1 - y0, BLACK);
    /* The entire visual border matches the controls module's half-open bounds. */
    line(fb, x0, y0, x1 - 1, y0, DARK);
    line(fb, x0, y1 - 1, x1 - 1, y1 - 1, DARK);
    line(fb, x0, y0, x0, y1 - 1, DARK);
    line(fb, x1 - 1, y0, x1 - 1, y1 - 1, DARK);
    line(fb, x0, y0, x0 + 8, y0, color);
    line(fb, x0, y0, x0, y0 + 5, color);
    line(fb, x1 - 9, y1 - 1, x1 - 1, y1 - 1, color);
    line(fb, x1 - 1, y1 - 6, x1 - 1, y1 - 1, color);
    label(fb, word, (x0 + x1 - text_width(word, scale)) / 2, (y0 + y1 - 7 * scale) / 2, color,
          scale);
}
static const char *mode_name(const game_state *g) {
    if (g->control_mode == (uint8_t)CONTROLS_TILT)
        return "TILT";
    return g->control_mode == (uint8_t)CONTROLS_RELATIVE_DRAG ? "BORDER DRAG" : "LEFT / RIGHT";
}
static void settings_mode(const game_state *g, uint8_t *fb) {
    const int y0 = (int)CONTROLS_SETTINGS_MODE_Y0;
    const int y1 = (int)CONTROLS_SETTINGS_MODE_Y1;
    const bool tilt = g->control_mode == (uint8_t)CONTROLS_TILT;
    centered(fb,
             tilt ? (g->tilt_ready ? "TILT READY - BANK TO TEST" : "HOLD STILL TO CENTRE")
                  : "STEERING",
             42, STEEL, 1);
    button(fb, (int)CONTROLS_SETTINGS_MODE_X0, y0, (int)CONTROLS_SETTINGS_MODE_X1, y1, mode_name(g),
           CYAN, 1);
    label(fb, "<", (int)CONTROLS_SETTINGS_MODE_X0 + 6, (y0 + y1 - 7) / 2, STEEL, 1);
    label(fb, ">", (int)CONTROLS_SETTINGS_MODE_X1 - 11, (y0 + y1 - 7) / 2, STEEL, 1);
    if (tilt) {
        const int bar_y = y1 - 5;
        fill(fb, 84, bar_y, 72, 2, DARK);
        if (g->tilt_ready) {
            int preview = g->tilt_preview;
            if (preview < -1000)
                preview = -1000;
            if (preview > 1000)
                preview = 1000;
            const int marker = 120 + preview * 32 / 1000;
            fill(fb, 116, bar_y - 2, 9, 5, DEEP_BLUE);
            line(fb, 120, bar_y, marker, bar_y, CYAN);
            fill(fb, marker - 1, bar_y - 2, 3, 5, marker >= 117 && marker <= 123 ? GREEN : CYAN);
            label(fb, "L", 72, y1 - 8, STEEL, 1);
            label(fb, "R", 163, y1 - 8, STEEL, 1);
        } else {
            const unsigned progress = g->tilt_progress > 1000u ? 1000u : g->tilt_progress;
            fill(fb, 84, bar_y, (int)(progress * 72u / 1000u), 2, AMBER);
        }
    }
}
static void title(const game_state *g, uint8_t *fb, bool over) {
    char number[11];
    orbit_ticks(fb, DARK);
    decimal(number, g->high_score, 6);
    centered(fb, "HIGH SCORE", 13, RED, 1);
    centered(fb, number, 24, IVORY, 1);
    if (over) {
        centered(fb, "GAME OVER", 46, AMBER, 2);
        centered(fb, "FINAL SCORE", 67, STEEL, 1);
        decimal(number, g->score, 6);
        centered(fb, number, 79, IVORY, 1);
    } else {
        /* An offset shadow adds a metallic arcade-title edge at native resolution. */
        label(fb, "GYRUS", 64, 43, DARK, 4);
        centered(fb, "GYRUS", 40, IVORY, 4);
        centered(fb, "2040", 72, CYAN, 2);
        sprite(fb, GYRUS_SPRITE_SCOUT, 2, 49, 74);
        sprite(fb, GYRUS_SPRITE_FIGHTER, 14, 191, 74);
    }
    button(fb, (int)CONTROLS_START_X0, (int)CONTROLS_START_Y0, (int)CONTROLS_START_X1,
           (int)CONTROLS_START_Y1, over ? "RETRY" : "START",
           ((g->tick / 30u) & 1u) != 0u ? IVORY : CYAN, 2);
    button(fb, (int)CONTROLS_TITLE_SETTINGS_X0, (int)CONTROLS_TITLE_SETTINGS_Y0,
           (int)CONTROLS_TITLE_SETTINGS_X1, (int)CONTROLS_TITLE_SETTINGS_Y1, "SETTINGS", CYAN, 2);
    centered(fb, g->autofire ? "AUTO FIRE ON" : "BOOT FIRES", 185, IVORY, 1);
    const bool tilt = g->control_mode == (uint8_t)CONTROLS_TILT;
    const char *instruction =
        tilt ? "TILT TO ORBIT"
             : (g->control_mode == (uint8_t)CONTROLS_RELATIVE_DRAG ? "DRAG RIM TO ORBIT"
                                                                   : "HOLD SIDES TO ORBIT");
    centered(fb, instruction, 197, STEEL, 1);
    centered(fb, tilt ? "HOLD STILL TO CENTRE" : "TAP CENTRE TO PAUSE", 208, STEEL, 1);
    sprite(fb, GYRUS_SPRITE_LIFE, 0, 120, 228);
}
static void hud(const game_state *g, uint8_t *fb) {
    char number[11];
    const bool flying = g->phase == GAME_PLAY || g->phase == GAME_CHANCE ||
                        g->phase == GAME_PAUSED || g->phase == GAME_CLEAR ||
                        g->phase == GAME_WARP || g->phase == GAME_ARRIVAL;
    const int top = flying ? 39 : 9, bottom = flying ? 199 : 219, status = flying ? 64 : 33;
    /* Keep flight HUD inside the orbit; the ship is also composited last. */
    fill(fb, 74, top, 92, 23, BLACK);
    decimal(number, g->score, 6);
    label(fb, "1UP", 76, top + 1, AMBER, 1);
    label(fb, number, 102, top + 1, IVORY, 1);
    decimal(number, g->high_score, 6);
    label(fb, "HI", 79, top + 13, RED, 1);
    label(fb, number, 102, top + 13, STEEL, 1);
    decimal(number, g->stage, 2);
    const int width = 41 + text_width(number, 1);
    fill(fb, 116 - width / 2, bottom, width + 8, 11, BLACK);
    label(fb, "STAGE", 120 - width / 2, bottom + 2, STEEL, 1);
    label(fb, number, 156 - width / 2, bottom + 2, IVORY, 1);
    const unsigned shown = g->lives > 5u ? 5u : g->lives;
    if (g->phase != GAME_INTRO && g->phase != GAME_ARRIVAL && g->phase != GAME_WARP)
        for (unsigned i = 0; i < shown; ++i)
            sprite(fb, GYRUS_SPRITE_LIFE, 0, 97 + (int)i * 11, 187);
    if (g->phase == GAME_PLAY || g->phase == GAME_CHANCE || g->phase == GAME_PAUSED) {
        if (g->autofire)
            label(fb, "AUTO", 108, status, STEEL, 1);
        if (g->double_fire)
            label(fb, "DUAL", 149, status, CYAN, 1);
        if (g->invincible)
            label(fb, "+", 69, status, GREEN, 1);
    }
    if (g->phase == GAME_CHANCE) {
        centered(fb, "CHANCE", 76, AMBER, 1);
        decimal(number, g->destroyed, 2);
        label(fb, number, 108, 174, CYAN, 1);
        label(fb, "/40", 120, 174, STEEL, 1);
    }
}
static void projectiles(const game_state *g, uint8_t *fb) {
    for (unsigned i = 0; i < GAME_SHOTS; ++i) {
        const game_shot *shot = &g->shots[i];
        if (shot->active == 0u)
            continue;
        int16_t x0, y0, x1, y1;
        game_polar(shot->angle, shot->radius - 3 * 256, &x0, &y0);
        game_polar(shot->angle, shot->radius + 256, &x1, &y1);
        line(fb, x0, y0, x1, y1, CYAN);
        pixel(fb, x0, y0, WHITE);
    }
    for (unsigned i = 0; i < GAME_HOSTILE_SHOTS; ++i) {
        const game_shot *shot = &g->hostile[i];
        if (shot->active == 0u)
            continue;
        int16_t x, y;
        game_polar(shot->angle, shot->radius, &x, &y);
        sprite(fb, GYRUS_SPRITE_ENEMY_BULLET, 0, x, y);
    }
}
static void enemies(const game_state *g, uint8_t *fb) {
    const game_enemy *previous = NULL;
    for (unsigned i = 0; i < GAME_ENEMIES; ++i) {
        const game_enemy *enemy = &g->enemies[i];
        if (enemy->active == 0u || enemy->kind != 5u)
            continue;
        if (previous != NULL) {
            int16_t ax, ay, bx, by;
            game_polar(previous->angle, previous->radius, &ax, &ay);
            game_polar(enemy->angle, enemy->radius, &bx, &by);
            line(fb, ax - 1, ay, bx - 1, by, RED);
            line(fb, ax + 1, ay, bx + 1, by, ORANGE);
            line(fb, ax, ay, bx, by, ((g->tick / 3u) & 1u) != 0u ? YELLOW : IVORY);
        }
        previous = enemy;
    }
    for (unsigned i = 0; i < GAME_ENEMIES; ++i) {
        const game_enemy *enemy = &g->enemies[i];
        if (enemy->active == 0u)
            continue;
        int16_t x, y;
        game_polar(enemy->angle, enemy->radius, &x, &y);
        unsigned frame = (unsigned)enemy->age / 5u;
        gyrus_sprite_id id;
        if (enemy->kind <= 2u || enemy->kind == 6u) {
            static const gyrus_sprite_id ships[] = {GYRUS_SPRITE_SCOUT, GYRUS_SPRITE_FIGHTER,
                                                    GYRUS_SPRITE_MOTH};
            id = ships[enemy->kind % 3u];
            uint16_t heading = enemy->angle;
            if (enemy->mode == 3u)
                heading = (uint16_t)((unsigned)heading + 32768u);
            else if (enemy->mode != 2u && enemy->mode != 4u)
                heading =
                    (uint16_t)((int32_t)heading + (enemy->angular_speed >= 0 ? 16384 : -16384));
            frame = ((unsigned)heading + 2048u) >> 12u;
        } else if (enemy->kind == 3u)
            id = GYRUS_SPRITE_SATELLITE;
        else if (enemy->kind == 4u)
            id = GYRUS_SPRITE_ASTEROID;
        else {
            id = GYRUS_SPRITE_LASER_EMITTER;
            frame = 4u + ((unsigned)g->tick / 6u) % 4u;
        }
        sprite(fb, id, frame, x, y);
        if (enemy->kind == 3u) {
            if ((g->tick / 16u) % 2u == 0u)
                pixel(fb, (int)x, (int)y - 9, WHITE);
        }
    }
}
static void particles(const game_state *g, uint8_t *fb) {
    for (unsigned i = 0; i < GAME_PARTICLES; ++i) {
        const game_particle *p = &g->particles[i];
        if (p->life == 0u)
            continue;
        const uint8_t color = p->color < GYRUS_COLOR_COUNT ? p->color : AMBER;
        if (i % 6u == 0u && p->life > 3u) {
            const unsigned age = p->life < 27u ? 27u - (unsigned)p->life : 0u;
            sprite(fb, GYRUS_SPRITE_EXPLOSION, age / 5u, p->x, p->y);
        } else {
            pixel(fb, p->x, p->y, p->life > 18u ? WHITE : color);
            if (p->life > 12u) {
                pixel(fb, (int)p->x - 1, p->y, color);
                pixel(fb, p->x, (int)p->y - 1, color);
            }
        }
    }
}
static void ship(const game_state *g, uint8_t *fb) {
    if (g->lives == 0u || (g->invulnerable != 0u && (g->tick / 5u) % 2u != 0u))
        return;
    int16_t x, y;
    game_polar(g->ship_angle, 104 * 256, &x, &y);
    const unsigned heading = ((unsigned)g->ship_angle + 32768u) >> 11u;
    const gyrus_asset *hull = gyrus_sprite(GYRUS_SPRITE_PLAYER, heading);
    int left = x - (int)hull->width / 2, top = y - (int)hull->height / 2;
    /* Dark outer halo separates the bright hull border from text and enemies. */
    for (unsigned pass = 0; pass < 2; ++pass) {
        int radius = pass == 0 ? 2 : 1;
        uint8_t color = pass == 0 ? BLACK : IVORY;
        for (unsigned sy = 0; sy < hull->height; ++sy)
            for (unsigned sx = 0; sx < hull->width; ++sx)
                if (hull->pixels[sy * hull->width + sx])
                    fill(fb, left + (int)sx - radius, top + (int)sy - radius, 2 * radius + 1,
                         2 * radius + 1, color);
    }
    sprite(fb, GYRUS_SPRITE_PLAYER, heading, x, y);
    if (g->double_fire != 0u) {
        int16_t lx, ly, rx, ry;
        game_polar((uint16_t)((unsigned)g->ship_angle - 2100u), 104 * 256, &lx, &ly);
        game_polar((uint16_t)((unsigned)g->ship_angle + 2100u), 104 * 256, &rx, &ry);
        pixel(fb, lx, ly, CYAN);
        pixel(fb, rx, ry, CYAN);
    }
}
static void intermission(const game_state *g, uint8_t *fb) {
    char number[11];
    orbit_ticks(fb, DARK);
    if (g->warp == 0u) {
        centered(fb, "CHANCE STAGE", 57, AMBER, 2);
        centered(fb, "DESTROY ALL 40", 86, IVORY, 1);
        centered(fb, "PERFECT +10000", 101, CYAN, 1);
        sprite_scaled(fb, planet_sprite(g->planet), 0, 120, 139, 2);
    } else {
        centered(fb, "NEXT DESTINATION", 53, STEEL, 1);
        centered(fb, game_planet_name(g->planet), 75, IVORY, 2);
        sprite_scaled(fb, planet_sprite(g->planet), 0, 120, 121, 2);
        decimal(number, g->warp, 1);
        label(fb, number, 87, 158, AMBER, 1);
        label(fb, g->warp == 1u ? "WARP TO GO" : "WARPS TO GO", 99, 158, STEEL, 1);
    }
    route(g, fb, 181);
    if ((g->phase_ticks / 15u) % 2u == 0u)
        centered(fb, "READY", 207, IVORY, 1);
}
static void chance_result(const game_state *g, uint8_t *fb) {
    char number[11];
    centered(fb, "BONUS", 86, AMBER, 2);
    const bool perfect = g->destroyed == g->stage_target && g->stage_target != 0u;
    if (perfect) {
        centered(fb, "PERFECT", 116, CYAN, 1);
    } else {
        decimal(number, g->destroyed, 2);
        const int left = 120 - (36 + text_width(number, 1)) / 2;
        label(fb, "100 X", left, 116, IVORY, 1);
        label(fb, number, left + 36, 116, IVORY, 1);
    }
    decimal(number, g->transition_bonus, 1);
    const int width = text_width(number, 2) + 12 + text_width("PTS", 2);
    label(fb, number, 120 - width / 2, 139, IVORY, 2);
    label(fb, "PTS", 132 - width / 2 + text_width(number, 2), 139, AMBER, 2);
}
static void warp_caption(const game_state *g, uint8_t *fb) {
    const unsigned remaining = g->clear_chance ? 3u : (g->warp > 1u ? (unsigned)g->warp - 1u : 0u);
    const unsigned destination = g->clear_chance ? (unsigned)g->planet + 1u : g->planet;
    char number[11];
    if (remaining != 0u) {
        decimal(number, remaining, 1);
        label(fb, number, 87, 82, AMBER, 1);
        label(fb, remaining == 1u ? "WARP TO" : "WARPS TO", 99, 82, IVORY, 1);
    }
    centered(fb, game_planet_name(destination), remaining != 0u ? 95 : 82, AMBER, 1);
}
static void planet_approach(const game_state *g, uint8_t *fb) {
    const unsigned progress = transition_progress(g, GAME_ARRIVAL_TICKS);
    sprite_resized(fb, planet_sprite(g->planet), 0, 120, 120, 14u + progress * 26u / 1000u);
    centered(fb, game_planet_name(g->planet), 169, AMBER, 1);
}
static void travelling_ship(const game_state *g, uint8_t *fb, game_phase phase) {
    const unsigned progress =
        transition_progress(g, phase == GAME_WARP ? GAME_WARP_TICKS : GAME_ARRIVAL_TICKS);
    if (phase == GAME_ARRIVAL && progress > 900u)
        return;
    const int32_t radius = phase == GAME_WARP ? 104 * 256 - (int32_t)(progress * 80u * 256u / 1000u)
                                              : (int32_t)((1000u - progress) * 24u * 256u / 1000u);
    const unsigned width =
        phase == GAME_WARP ? 17u - progress * 10u / 1000u : 7u - progress * 6u / 1000u;
    int16_t x, y;
    game_polar(g->ship_angle, radius, &x, &y);
    sprite_resized(fb, GYRUS_SPRITE_PLAYER, ((unsigned)g->ship_angle + 32768u) >> 11u, x, y, width);
}
typedef struct {
    int x0, y0, x1, y1;
} settings_rect;
static void settings_row(uint8_t *fb, const char *name, uint32_t value, const char *unit,
                         int label_y, settings_rect down, settings_rect up, uint32_t minimum,
                         uint32_t maximum) {
    char number[11];
    char display[14];
    decimal(number, value, 1);
    size_t length = strlen(number);
    memcpy(display, number, length);
    for (size_t i = 0; unit[i] != '\0' && length + 1u < sizeof display; ++i)
        display[length++] = unit[i];
    display[length] = '\0';
    centered(fb, name, label_y, STEEL, 1);
    button(fb, down.x0, down.y0, down.x1, down.y1, "-", value <= minimum ? STEEL : CYAN, 2);
    button(fb, up.x0, up.y0, up.x1, up.y1, "+", value >= maximum ? STEEL : CYAN, 2);
    centered(fb, display, (down.y0 + down.y1 - 14) / 2, IVORY, 2);
}
static void settings_page(const game_state *g, uint8_t *fb) {
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
    centered(fb, "SETTINGS", 21, IVORY, 2);
    settings_mode(g, fb);
    settings_row(fb, "FIRE RATE", g->settings.fire_rate, "/S", 83, rows[0][0], rows[0][1], 1, 10);
    settings_row(fb, "ENEMY SPEED", g->settings.enemy_speed_percent, "%", 126, rows[1][0],
                 rows[1][1], 25, 200);
    settings_row(fb, "TILT SENS", g->settings.tilt_sensitivity_percent, "%", 169, rows[2][0],
                 rows[2][1], 50, 200);
    button(fb, (int)CONTROLS_BACK_X0, (int)CONTROLS_BACK_Y0, (int)CONTROLS_BACK_X1,
           (int)CONTROLS_BACK_Y1, "BACK", CYAN, 2);
}
static void pause_menu(const game_state *g, uint8_t *fb) {
    static const uint8_t dim[GYRUS_COLOR_COUNT] = {
        BLACK, DARK, DARK, DEEP_BLUE, DEEP_BLUE, DEEP_BLUE, DARK,      DARK,
        DARK,  DARK, DARK, DEEP_BLUE, DARK,      DARK,      DEEP_BLUE, DARK,
        BLACK, DARK, DARK, DARK,      DARK,      DARK,      DARK,      DEEP_BLUE};
    for (unsigned i = 0; i < GAME_WIDTH * GAME_HEIGHT; ++i)
        fb[i] = fb[i] < GYRUS_COLOR_COUNT ? dim[fb[i]] : BLACK;
    fill(fb, 35, 27, 170, 207, BLACK);
    centered(fb, "PAUSED", 38, IVORY, 2);
    centered(fb, "FLIGHT ON HOLD", 61, STEEL, 1);
    button(fb, (int)CONTROLS_RESUME_X0, (int)CONTROLS_RESUME_Y0, (int)CONTROLS_RESUME_X1,
           (int)CONTROLS_RESUME_Y1, "RESUME", CYAN, 2);
    button(fb, (int)CONTROLS_SETTINGS_X0, (int)CONTROLS_SETTINGS_Y0, (int)CONTROLS_SETTINGS_X1,
           (int)CONTROLS_SETTINGS_Y1, "SETTINGS", IVORY, 2);
    button(fb, (int)CONTROLS_QUIT_X0, (int)CONTROLS_QUIT_Y0, (int)CONTROLS_QUIT_X1,
           (int)CONTROLS_QUIT_Y1, "QUIT", IVORY, 2);
    button(fb, (int)CONTROLS_REBOOT_X0, (int)CONTROLS_REBOOT_Y0, (int)CONTROLS_REBOOT_X1,
           (int)CONTROLS_REBOOT_Y1, "REBOOT HOLD 5S", AMBER, 1);
    const unsigned progress = g->reboot_progress > 1000u ? 1000u : g->reboot_progress;
    if (progress > 0u) {
        const int width = (int)(progress * (CONTROLS_REBOOT_X1 - CONTROLS_REBOOT_X0 - 4u) / 1000u);
        fill(fb, (int)CONTROLS_REBOOT_X0 + 2, (int)CONTROLS_REBOOT_Y1 - 4, width, 2, AMBER);
    }
    centered(fb, "BOOT ALSO RESUMES", 219, STEEL, 1);
}
void game_render(const game_state *g, uint8_t framebuffer[GAME_WIDTH * GAME_HEIGHT]) {
    const game_phase scene = g->phase == GAME_PAUSED ? g->resume_phase : g->phase;
    memset(framebuffer, BLACK, GAME_WIDTH * GAME_HEIGHT);
    if (scene == GAME_WARP || scene == GAME_ARRIVAL)
        travel_stars(g, framebuffer, scene);
    else
        stars(g, framebuffer, scene == GAME_INTRO);
    if (g->pause_settings &&
        (g->phase == GAME_TITLE || g->phase == GAME_OVER || g->phase == GAME_PAUSED)) {
        fill(framebuffer, 35, 15, 170, 219, BLACK);
        settings_page(g, framebuffer);
    } else if (g->phase == GAME_TITLE || g->phase == GAME_OVER) {
        title(g, framebuffer, g->phase == GAME_OVER);
    } else {
        if (scene == GAME_INTRO)
            intermission(g, framebuffer);
        else if (scene == GAME_ARRIVAL)
            planet_approach(g, framebuffer);
        else if (scene == GAME_WARP)
            warp_caption(g, framebuffer);
        else if (scene == GAME_CLEAR) {
            if (g->clear_chance)
                chance_result(g, framebuffer);
        } else {
            projectiles(g, framebuffer);
            enemies(g, framebuffer);
            particles(g, framebuffer);
        }
        hud(g, framebuffer);
        if (g->control_mode == (uint8_t)CONTROLS_TILT && !g->tilt_ready &&
            (g->phase == GAME_PLAY || g->phase == GAME_CHANCE)) {
            fill(framebuffer, 84, 106, 72, 30, BLACK);
            centered(framebuffer, "HOLD STILL", 111, AMBER, 1);
            centered(framebuffer, "CENTRING", 123, STEEL, 1);
        }
        if (scene == GAME_WARP || scene == GAME_ARRIVAL)
            travelling_ship(g, framebuffer, scene);
        else if (scene == GAME_PLAY || scene == GAME_CHANCE || scene == GAME_CLEAR)
            ship(g, framebuffer); /* Foreground priority over HUD and messages. */
        if (g->phase == GAME_PAUSED)
            pause_menu(g, framebuffer);
    }
    /* A round LCD hides these pixels physically. Masking also makes host captures
       show the exact playable aperture and prevents corner-only HUD content. */
    for (int y = 0; y < 240; ++y)
        for (int x = 0; x < 240; ++x)
            if ((x - 120) * (x - 120) + (y - 120) * (y - 120) > 120 * 120)
                framebuffer[(unsigned)y * GAME_WIDTH + (unsigned)x] = BLACK;
}
