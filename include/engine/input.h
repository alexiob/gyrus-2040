#ifndef ENGINE_INPUT_H
#define ENGINE_INPUT_H
#include <stdbool.h>
#include <stdint.h>
typedef enum {
    GAME_MENU_NONE,
    GAME_MENU_SETTINGS,
    GAME_MENU_BACK,
    GAME_MENU_FIRE_DOWN,
    GAME_MENU_FIRE_UP,
    GAME_MENU_ENEMY_DOWN,
    GAME_MENU_ENEMY_UP,
    GAME_MENU_TILT_DOWN,
    GAME_MENU_TILT_UP,
    GAME_MENU_EFFECTS
} game_menu_action;
typedef enum {
    GAME_SETTING_FIRE_RATE,
    GAME_SETTING_ENEMY_SPEED,
    GAME_SETTING_TILT_SENSITIVITY
} game_setting;
typedef struct {
    uint8_t fire_rate;
    uint16_t enemy_speed_percent, tilt_sensitivity_percent;
} game_settings;
typedef struct {
    int8_t turn;
    int16_t angle_delta;
    bool fire, start, pause, quit;
    game_menu_action menu_action;
    int8_t navigate;    /* Launcher -1/0/+1, one pulse. */
    bool special, home; /* One-tick game action and return-to-launcher. */
} engine_input;
typedef engine_input game_input; /* Gyrus source compatibility. */
bool engine_settings_set(game_settings *, game_setting, int32_t value);
bool engine_settings_menu(game_settings *, game_menu_action);
#endif
