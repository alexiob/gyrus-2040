#include "engine/session.h"
#include "engine/hub.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
const engine_game_ops *engine_current_game(const engine_state *s) {
    size_t count;
    const engine_game_ops *const *games = engine_catalogue(&count);
    return (unsigned)s->selected < count ? games[s->selected] : NULL;
}
engine_game_view engine_current_view(const engine_state *s) {
    const engine_game_ops *ops = engine_current_game(s);
    return ops ? ops->view(&s->game) : (engine_game_view){0};
}
controls_screen engine_controls_screen(const engine_state *s) {
    return s->hub ? (s->hub_settings ? CONTROLS_SETTINGS : CONTROLS_HUB)
                  : engine_current_view(s).screen;
}
static void save_record(engine_state *s) {
    engine_game_view view = engine_current_view(s);
    engine_game_record *record = &s->records[s->selected];
    record->high_score = view.high_score;
    record->settings = view.settings;
    s->tilt_sensitivity = view.settings.tilt_sensitivity_percent;
}
static void restore_record(engine_state *s) {
    const engine_game_ops *ops = engine_current_game(s);
    const engine_game_record *record = &s->records[s->selected];
    (void)ops->set(&s->game, "high_score", (int32_t)record->high_score);
    (void)ops->set(&s->game, "fire_rate", record->settings.fire_rate);
    (void)ops->set(&s->game, "enemy_speed", record->settings.enemy_speed_percent);
    (void)ops->set(&s->game, "tilt_sensitivity", s->tilt_sensitivity);
    if (ops->capabilities & ENGINE_CAP_EFFECTS)
        (void)ops->set(&s->game, "effects", record->neon ? 1 : 0);
    ops->sync_ui(&s->game, &s->ui);
}
void engine_init(engine_state *s, uint32_t seed) {
    memset(s, 0, sizeof *s);
    controls_init(&s->controls);
    s->seed = seed;
    s->tilt_sensitivity = 100;
    for (unsigned i = 0; i < ENGINE_GAME_COUNT; ++i)
        s->records[i].settings = (game_settings){5, 100, 100};
    s->selected = ENGINE_GAME_GYRUS;
    engine_current_game(s)->init(&s->game, seed);
    s->hub = true;
}
bool engine_select_game(engine_state *s, engine_game_id id, uint32_t seed) {
    size_t count;
    const engine_game_ops *const *games = engine_catalogue(&count);
    if ((unsigned)id >= count)
        return false;
    save_record(s);
    s->selected = id;
    s->seed = seed;
    s->hub = false;
    s->hub_settings = false;
    s->slide_pixels = 0;
    s->input = (engine_input){0};
    controls_consume_inputs(&s->controls);
    memset(&s->game, 0, sizeof s->game);
    games[id]->init(&s->game, seed);
    restore_record(s);
    return true;
}
void engine_home(engine_state *s) {
    (void)engine_select_game(s, s->selected, s->seed);
    s->hub = true;
}
bool engine_set(engine_state *s, const char *key, int32_t value) {
    const engine_game_ops *ops = engine_current_game(s);
    if (!strcmp(key, "control_mode")) {
        if (value < 0 || value >= CONTROLS_MODE_COUNT)
            return false;
        controls_set_mode(&s->controls, (controls_mode)value);
        s->ui.control_mode = (uint8_t)value;
        ops->sync_ui(&s->game, &s->ui);
        return true;
    }
    if (!strcmp(key, "effects")) {
        if (!s->hub || !(ops->capabilities & ENGINE_CAP_EFFECTS) || (value != 0 && value != 1))
            return false;
        if (!ops->set(&s->game, key, value))
            return false;
        s->records[s->selected].neon = value != 0;
        return true;
    }
    if (!ops->set(&s->game, key, value))
        return false;
    save_record(s);
    return true;
}
bool engine_level(engine_state *s, unsigned value) {
    return engine_current_game(s)->level(&s->game, value);
}
static void hub_menu(engine_state *s, game_menu_action action) {
    if (action == GAME_MENU_SETTINGS)
        s->hub_settings = true;
    else if (action == GAME_MENU_BACK)
        s->hub_settings = false;
    else if (action == GAME_MENU_EFFECTS && !s->hub_settings)
        (void)engine_set(s, "effects", !s->records[s->selected].neon);
    else if (s->hub_settings) {
        game_settings values = engine_current_view(s).settings;
        if (engine_settings_menu(&values, action)) {
            (void)engine_set(s, "fire_rate", values.fire_rate);
            (void)engine_set(s, "enemy_speed", values.enemy_speed_percent);
            (void)engine_set(s, "tilt_sensitivity", values.tilt_sensitivity_percent);
        }
    }
}
void engine_step(engine_state *s, const engine_input *input) {
    const engine_input empty = {0};
    const engine_input *in = input ? input : &empty;
    ++s->ticks;
    if (in->home) {
        engine_home(s);
        return;
    }
    if (s->hub) {
        ++s->hub_tick;
        if (s->slide_pixels) {
            int distance = s->slide_pixels;
            int amount = distance / 4;
            if (!amount)
                amount = distance > 0 ? 1 : -1;
            s->slide_pixels = (int16_t)(distance - amount);
        }
        hub_menu(s, in->menu_action);
        if (!s->hub_settings && in->navigate) {
            size_t count;
            (void)engine_catalogue(&count);
            int direction = in->navigate > 0 ? 1 : -1;
            int next = (int)s->selected + direction;
            if (next < 0)
                next = (int)count - 1;
            if ((size_t)next >= count)
                next = 0;
            (void)engine_select_game(s, (engine_game_id)next, s->seed);
            s->hub = true;
            s->slide_pixels = direction > 0 ? 220 : -220;
        } else if (!s->hub_settings && in->start) {
            s->hub = false;
            controls_consume_inputs(&s->controls);
            engine_input launch = {.start = true};
            engine_current_game(s)->step(&s->game, &launch);
        }
    } else {
        engine_current_game(s)->step(&s->game, in);
        save_record(s);
    }
}
static void consume_pulses(engine_input *in) {
    in->angle_delta = 0;
    in->start = in->pause = in->quit = in->special = in->home = false;
    in->navigate = 0;
    in->menu_action = GAME_MENU_NONE;
}
void engine_tick(engine_state *s) {
    engine_input in = s->input;
    engine_step(s, &in);
    consume_pulses(&s->input);
}
static engine_input merge_input(engine_input a, const engine_input *b) {
    if (!b)
        return a;
    if (b->turn)
        a.turn = b->turn;
    int32_t angle = (int32_t)a.angle_delta + b->angle_delta;
    if (angle > INT16_MAX)
        angle = INT16_MAX;
    if (angle < INT16_MIN)
        angle = INT16_MIN;
    a.angle_delta = (int16_t)angle;
    a.fire |= b->fire;
    a.start |= b->start;
    a.pause |= b->pause;
    a.quit |= b->quit;
    a.special |= b->special;
    a.home |= b->home;
    if (b->navigate)
        a.navigate = b->navigate;
    if (b->menu_action)
        a.menu_action = b->menu_action;
    return a;
}
bool engine_update(engine_state *s, controls_sample sample, const engine_input *keyboard) {
    sample.tilt_sensitivity_percent = s->tilt_sensitivity;
    controls_output physical = controls_update(&s->controls, sample, engine_controls_screen(s));
    s->ui = (engine_ui){.control_mode = (uint8_t)s->controls.mode,
                        .reboot_progress = controls_reboot_progress(&s->controls, sample.now_ms),
                        .tilt_ready = controls_tilt_ready(&s->controls),
                        .tilt_progress = controls_tilt_progress(&s->controls, sample.now_ms),
                        .tilt_preview = controls_tilt_preview(&s->controls)};
    engine_current_game(s)->sync_ui(&s->game, &s->ui);
    if (physical.input.quit) {
        physical.input.quit = false;
        physical.input.home = true;
    }
    if (!s->frozen) {
        engine_input in = merge_input(s->input, keyboard);
        in = merge_input(in, &physical.input);
        engine_step(s, &in);
        consume_pulses(&s->input);
    }
    return physical.reboot_requested;
}
const uint16_t *engine_render(const engine_state *s, uint8_t *pixels) {
    const engine_game_ops *ops = engine_current_game(s);
    if (s->hub) {
        size_t count;
        const engine_game_ops *const *games = engine_catalogue(&count);
        engine_game_view view = engine_current_view(s);
        engine_hub_view hub = {.catalogue = games,
                               .count = count,
                               .selected = (unsigned)s->selected,
                               .slide_pixels = s->slide_pixels,
                               .tick = s->hub_tick,
                               .high_score = view.high_score,
                               .neon = s->records[s->selected].neon,
                               .settings_open = s->hub_settings,
                               .settings = view.settings,
                               .ui = s->ui};
        engine_hub_render(&hub, pixels);
    } else {
        ops->render(&s->game, pixels);
        engine_game_view view = engine_current_view(s);
        if ((view.title || view.over) && !view.settings_open)
            engine_ui_home_button(pixels);
    }
    return ops->palette;
}
bool engine_state_json(const engine_state *s, char *out, size_t cap) {
    const engine_game_ops *ops = engine_current_game(s);
    if (!s->hub)
        return ops->state_json(&s->game, s->frozen, out, cap);
    engine_game_view view = engine_current_view(s);
    int n =
        snprintf(out, cap,
                 "{\"hub\":true,\"phase\":\"hub\",\"selected\":%u,\"game\":\"%s\",\"tick\":%" PRIu32
                 ",\"frozen\":%s,\"high_score\":%" PRIu32 ",\"settings\":%s,\"fire_rate\":%u,"
                 "\"enemy_speed\":%u,\"tilt_sensitivity\":%u,\"effects\":%s,\"control_mode\":%u,"
                 "\"tilt_ready\":%s,\"tilt_progress\":%u,\"tilt_preview\":%d}",
                 (unsigned)s->selected, ops->slug, s->hub_tick, s->frozen ? "true" : "false",
                 view.high_score, s->hub_settings ? "true" : "false",
                 (unsigned)view.settings.fire_rate, (unsigned)view.settings.enemy_speed_percent,
                 (unsigned)s->tilt_sensitivity, s->records[s->selected].neon ? "true" : "false",
                 (unsigned)s->ui.control_mode, s->ui.tilt_ready ? "true" : "false",
                 (unsigned)s->ui.tilt_progress, (int)s->ui.tilt_preview);
    return n > 0 && (size_t)n < cap;
}
