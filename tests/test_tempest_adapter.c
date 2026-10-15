#include "tempest/game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    tempest_state s;
    const engine_game_ops *g = &engine_tempest_game;
    g->init(&s, 44);
    assert(g->state_size == sizeof s);
    assert(g->set(&s, "high_score", 99));
    assert(g->view(&s).high_score == 99);
    assert(!g->set(&s, "high_score", -1));
    assert(!g->set(&s, "score", 100000000));
    assert(g->set(&s, "score", 20000));
    assert(g->set(&s, "lives", 4));
    assert(!g->set(&s, "lives", 100));
    assert(g->level(&s, 20));
    assert(g->view(&s).screen == CONTROLS_TEMPEST_PLAY);
    s.fire_cooldown = 10;
    s.fire_remainder = 2;
    assert(g->set(&s, "fire_rate", 5));
    assert(s.fire_cooldown == 10 && s.fire_remainder == 2);
    assert(g->set(&s, "fire_rate", 6));
    assert(s.fire_cooldown == 0);
    uint32_t h = g->hash(&s);
    assert(g->set(&s, "effects", 1));
    assert(h == g->hash(&s));
    assert(!g->set(&s, "effects", 2));
    engine_ui ui = {.control_mode = 2, .tilt_ready = true, .tilt_progress = 1000};
    g->sync_ui(&s, &ui);
    assert(h == g->hash(&s));
    char state[2048];
    assert(g->state_json(&s, true, state, sizeof state));
    assert(strstr(state, "\"game\":\"tempest\""));
    assert(strstr(state, "\"frozen\":true"));
    assert(strstr(state, "\"phase_ticks\":"));
    assert(strlen(state) < 600);
    char tiny[8];
    assert(!g->state_json(&s, false, tiny, sizeof tiny));
    engine_input in = {.pause = true};
    g->step(&s, &in);
    assert(g->view(&s).screen == CONTROLS_PAUSE);
    in = (engine_input){.menu_action = GAME_MENU_SETTINGS};
    g->step(&s, &in);
    assert(g->view(&s).screen == CONTROLS_SETTINGS);
    puts("Tempest engine adapter contracts passed");
    return 0;
}
