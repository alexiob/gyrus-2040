#ifndef ENGINE_UI_H
#define ENGINE_UI_H
#include "engine/input.h"
#include <stdint.h>
typedef struct {
    uint8_t control_mode;
    uint16_t reboot_progress;
    bool tilt_ready;
    uint16_t tilt_progress;
    int16_t tilt_preview;
} engine_ui;
typedef struct {
    bool settings;
    game_settings values;
    engine_ui ui;
} engine_menu_view;
void engine_ui_menu(const engine_menu_view *, uint8_t *pixels);
void engine_ui_home_button(uint8_t *pixels);
void engine_title_gyrus(uint8_t *, int cx, int cy, uint32_t tick, bool neon);
void engine_title_tempest(uint8_t *, int cx, int cy, uint32_t tick, bool neon);
#endif
