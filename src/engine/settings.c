#include "engine/input.h"
bool engine_settings_set(game_settings *s, game_setting setting, int32_t value) {
    switch (setting) {
    case GAME_SETTING_FIRE_RATE:
        if (value < 1 || value > 10)
            return false;
        s->fire_rate = (uint8_t)value;
        return true;
    case GAME_SETTING_ENEMY_SPEED:
        if (value < 25 || value > 200 || value % 25)
            return false;
        s->enemy_speed_percent = (uint16_t)value;
        return true;
    case GAME_SETTING_TILT_SENSITIVITY:
        if (value < 50 || value > 200 || value % 25)
            return false;
        s->tilt_sensitivity_percent = (uint16_t)value;
        return true;
    }
    return false;
}
bool engine_settings_menu(game_settings *s, game_menu_action action) {
    switch (action) {
    case GAME_MENU_FIRE_DOWN:
        return engine_settings_set(s, GAME_SETTING_FIRE_RATE, (int32_t)s->fire_rate - 1);
    case GAME_MENU_FIRE_UP:
        return engine_settings_set(s, GAME_SETTING_FIRE_RATE, (int32_t)s->fire_rate + 1);
    case GAME_MENU_ENEMY_DOWN:
        return engine_settings_set(s, GAME_SETTING_ENEMY_SPEED,
                                   (int32_t)s->enemy_speed_percent - 25);
    case GAME_MENU_ENEMY_UP:
        return engine_settings_set(s, GAME_SETTING_ENEMY_SPEED,
                                   (int32_t)s->enemy_speed_percent + 25);
    case GAME_MENU_TILT_DOWN:
        return engine_settings_set(s, GAME_SETTING_TILT_SENSITIVITY,
                                   (int32_t)s->tilt_sensitivity_percent - 25);
    case GAME_MENU_TILT_UP:
        return engine_settings_set(s, GAME_SETTING_TILT_SENSITIVITY,
                                   (int32_t)s->tilt_sensitivity_percent + 25);
    default:
        return false;
    }
}
