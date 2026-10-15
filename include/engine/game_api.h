#ifndef ENGINE_GAME_API_H
#define ENGINE_GAME_API_H
#include "engine/display.h"
#include "engine/input.h"
#include "engine/ui.h"
#include "gyrus/controls.h"
#include <stddef.h>
typedef enum { ENGINE_GAME_GYRUS, ENGINE_GAME_TEMPEST, ENGINE_GAME_COUNT } engine_game_id;
enum { ENGINE_CAP_SPECIAL = 1u, ENGINE_CAP_EFFECTS = 2u };
typedef struct {
    controls_screen screen;
    game_settings settings;
    uint32_t score, high_score;
    uint16_t level;
    uint8_t lives;
    bool title, over, paused, settings_open;
} engine_game_view;
typedef struct engine_game_ops {
    engine_game_id id;
    const char *slug, *title;
    uint16_t year;
    unsigned capabilities;
    size_t state_size;
    const uint16_t *palette;
    void (*init)(void *, uint32_t);
    void (*step)(void *, const engine_input *);
    void (*render)(const void *, uint8_t *);
    void (*draw_title)(uint8_t *, int, int, uint32_t, bool);
    engine_game_view (*view)(const void *);
    void (*sync_ui)(void *, const engine_ui *);
    bool (*set)(void *, const char *, int32_t);
    bool (*level)(void *, unsigned);
    uint32_t (*hash)(const void *);
    bool (*state_json)(const void *, bool frozen, char *, size_t);
} engine_game_ops;
extern const engine_game_ops engine_gyrus_game, engine_tempest_game;
const engine_game_ops *const *engine_catalogue(size_t *count);
#endif
