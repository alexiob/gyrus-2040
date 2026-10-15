/* macOS Metal/Sokol host. The simulator and G1 socket run on the UI thread. */
#define SOKOL_IMPL
#define SOKOL_METAL
#include "gyrus/assets.h"
#include "gyrus/controls.h"
#include "gyrus/debug_game.h"
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "util/sokol_gl.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static struct {
    game_state game;
    controls_state controls;
    gyrus_debug_game adapter;
    gyrus_debug protocol;
    uint8_t indexed[GAME_WIDTH * GAME_HEIGHT];
    uint32_t rgba[GAME_WIDTH * GAME_HEIGHT];
    sg_image image;
    sg_view view;
    sg_sampler sampler;
    int listener, client;
    uint16_t port;
    uint32_t seed;
    double accumulator, clock, response_deadline;
    float mouse_x, mouse_y;
    int32_t bank_mdeg;
    bool tilt_left, tilt_right;
    bool touch, fire, left, right, start, pause, quit;
} app;

static void logger(const char *tag, uint32_t level, uint32_t item, const char *message,
                   uint32_t line, const char *file, void *user) {
    (void)level;
    (void)item;
    (void)line;
    (void)file;
    (void)user;
    fprintf(stderr, "%s: %s\n", tag, message ? message : "(no message)");
}
static void disconnect_client(void) {
    if (app.client >= 0)
        close(app.client);
    app.client = -1;
}
static double monotonic_seconds(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
        return -1;
    return (double)now.tv_sec + (double)now.tv_nsec / 1000000000.0;
}
static void write_socket(void *ctx, const void *data, size_t size) {
    (void)ctx;
    const uint8_t *p = data;
    while (size && app.client >= 0) {
        double now = monotonic_seconds();
        if (now < 0 || now >= app.response_deadline) {
            disconnect_client();
            break;
        }
        ssize_t n = send(app.client, p, size, 0);
        if (n > 0) {
            p += (size_t)n;
            size -= (size_t)n;
        } else if (n < 0 && errno == EINTR)
            continue;
        else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            struct pollfd ready = {app.client, POLLOUT, 0};
            int wait_ms = (int)((app.response_deadline - now) * 1000.0);
            if (wait_ms > 100)
                wait_ms = 100;
            if (wait_ms < 1)
                wait_ms = 1;
            int status = poll(&ready, 1, wait_ms);
            if (status >= 0 && !(ready.revents & (POLLERR | POLLHUP | POLLNVAL)))
                continue;
            if (status < 0 && errno == EINTR)
                continue;
            disconnect_client();
        } else
            disconnect_client();
    }
}
static void setup_socket(void) {
    app.listener = app.client = -1;
    if (!app.port)
        return;
    app.listener = socket(AF_INET, SOCK_STREAM, 0);
    if (app.listener < 0) {
        perror("debug socket");
        return;
    }
    int yes = 1;
    setsockopt(app.listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    struct sockaddr_in address = {
        .sin_family = AF_INET, .sin_port = htons(app.port), .sin_addr = {htonl(INADDR_LOOPBACK)}};
    if (bind(app.listener, (const struct sockaddr *)&address, sizeof address) ||
        listen(app.listener, 1) || fcntl(app.listener, F_SETFL, O_NONBLOCK) < 0) {
        perror("debug bind/listen");
        close(app.listener);
        app.listener = -1;
        return;
    }
    fprintf(stderr, "G1 debug listening on 127.0.0.1:%u\n", (unsigned)app.port);
}
static void poll_socket(void) {
    if (app.listener < 0)
        return;
    if (app.client < 0) {
        app.client = accept(app.listener, NULL, NULL);
        if (app.client >= 0) {
            int yes = 1;
            setsockopt(app.client, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof yes);
            fcntl(app.client, F_SETFL, O_NONBLOCK);
            gyrus_debug_callbacks callbacks = gyrus_debug_game_callbacks(&app.adapter);
            gyrus_debug_init(&app.protocol, &callbacks);
        }
    }
    if (app.client < 0)
        return;
    /* Cap work per displayed frame so a sender cannot monopolize the UI. */
    uint8_t data[256];
    for (unsigned batch = 0; batch < 4 && app.client >= 0; batch++) {
        ssize_t n = recv(app.client, data, sizeof data, 0);
        if (n > 0) {
            for (size_t i = 0; i < (size_t)n && app.client >= 0; ++i) {
                /* One absolute budget spans every palette/pixel write in a response. */
                if (data[i] == '\n')
                    app.response_deadline = monotonic_seconds() + 5.0;
                gyrus_debug_feed(&app.protocol, data + i, 1);
                if (data[i] == '\n')
                    app.response_deadline = 0;
            }
        } else if (n == 0)
            disconnect_client();
        else if (errno == EINTR)
            continue;
        else {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
                disconnect_client();
            break;
        }
    }
}
static controls_screen current_screen(void) {
    if (app.game.pause_settings && (app.game.phase == GAME_TITLE || app.game.phase == GAME_OVER ||
                                    app.game.phase == GAME_PAUSED))
        return CONTROLS_SETTINGS;
    switch (app.game.phase) {
    case GAME_TITLE:
        return CONTROLS_TITLE;
    case GAME_PAUSED:
        return CONTROLS_PAUSE;
    case GAME_OVER:
        return CONTROLS_GAME_OVER;
    default:
        return CONTROLS_PLAY;
    }
}
static void viewport(int *x, int *y, int *size) {
    int w = sapp_width(), h = sapp_height();
    *size = w < h ? w : h;
    *x = (w - *size) / 2;
    *y = (h - *size) / 2;
}
static void simulate(void) {
    int x, y, size;
    viewport(&x, &y, &size);

    int tx = size ? (int)((app.mouse_x - (float)x) * 240.0f / (float)size) : -1;
    int ty = size ? (int)((app.mouse_y - (float)y) * 240.0f / (float)size) : -1;
    controls_sample sample = {.now_ms = (uint32_t)(app.clock * 1000.0),
                              .touch_pressed =
                                  app.touch && tx >= 0 && tx < 240 && ty >= 0 && ty < 240,
                              .touch_x = (uint16_t)(tx >= 0 ? tx : 0),
                              .touch_y = (uint16_t)(ty >= 0 ? ty : 0),
                              .fire_pressed = app.fire};
    /* Supply a complete, physically consistent virtual IMU sample at 60 Hz. */
    int32_t previous_bank = app.bank_mdeg;
    int32_t target_bank = app.tilt_left == app.tilt_right ? 0 : (app.tilt_right ? 30000 : -30000);
    if (app.bank_mdeg < target_bank) {
        app.bank_mdeg += 1000;
        if (app.bank_mdeg > target_bank)
            app.bank_mdeg = target_bank;
    } else if (app.bank_mdeg > target_bank) {
        app.bank_mdeg -= 1000;
        if (app.bank_mdeg < target_bank)
            app.bank_mdeg = target_bank;
    }
    double radians = (double)app.bank_mdeg * 3.14159265358979323846 / 180000.0;
    sample.motion_valid = true;
    sample.motion.sampled_ms = sample.now_ms;
    sample.motion.accel_mg[0] = (int32_t)lround(sin(radians) * 1000.0);
    sample.motion.accel_mg[2] = (int32_t)lround(cos(radians) * 1000.0);
    sample.motion.gyro_mdps[1] = -(app.bank_mdeg - previous_bank) * 60;
    sample.tilt_sensitivity_percent = app.game.settings.tilt_sensitivity_percent;
    controls_output local = controls_update(&app.controls, sample, current_screen());
    if (local.reboot_requested) {
        game_init(&app.game, app.seed);
        controls_init(&app.controls);
    }
    app.game.control_mode = (uint8_t)app.controls.mode;
    app.game.reboot_progress = controls_reboot_progress(&app.controls, sample.now_ms);
    app.game.tilt_ready = controls_tilt_ready(&app.controls);
    app.game.tilt_progress = controls_tilt_progress(&app.controls, sample.now_ms);
    app.game.tilt_preview = controls_tilt_preview(&app.controls);
    game_input retained = app.adapter.input;
    game_input *input = &app.adapter.input;
    if (app.left != app.right)
        input->turn = app.right ? 1 : -1;
    else if (local.input.turn)
        input->turn = local.input.turn;
    if (local.input.angle_delta)
        input->angle_delta = local.input.angle_delta;
    input->fire = input->fire || local.input.fire;
    input->start = input->start || local.input.start || app.start;
    input->pause = input->pause || local.input.pause || app.pause;
    input->quit = input->quit || local.input.quit || app.quit;
    if (local.input.menu_action != GAME_MENU_NONE)
        input->menu_action = local.input.menu_action;
    if (!app.adapter.frozen)
        gyrus_debug_game_tick(&app.adapter);
    /* Persistent injected controls remain until replaced; edge actions are pulses. */
    app.adapter.input.turn = retained.turn;
    app.adapter.input.fire = retained.fire;
    if (app.adapter.frozen)
        app.adapter.input = retained;
    app.start = app.pause = app.quit = false;
}
static void init(void) {
    sg_setup(&(sg_desc){.environment = sglue_environment(), .logger.func = logger});
    sgl_setup(&(sgl_desc_t){.logger.func = logger});
    app.image = sg_make_image(&(sg_image_desc){.width = 240,
                                               .height = 240,
                                               .pixel_format = SG_PIXELFORMAT_RGBA8,
                                               .usage.write_transient = true,
                                               .label = "indexed-framebuffer"});
    app.view = sg_make_view(&(sg_view_desc){.texture.image = app.image});
    app.sampler = sg_make_sampler(&(sg_sampler_desc){.min_filter = SG_FILTER_NEAREST,
                                                     .mag_filter = SG_FILTER_NEAREST,
                                                     .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                                                     .wrap_v = SG_WRAP_CLAMP_TO_EDGE});
    game_init(&app.game, app.seed);
    controls_init(&app.controls);
    gyrus_debug_game_init(&app.adapter, &app.game, app.indexed, write_socket, NULL);
    app.adapter.controls = &app.controls;
    setup_socket();
}
static void frame(void) {
    poll_socket();
    double elapsed = sapp_frame_duration();
    if (elapsed > 0.25)
        elapsed = 0.25;
    app.clock += elapsed;
    app.accumulator += elapsed;
    while (app.accumulator >= 1.0 / 60.0) {
        simulate();
        app.accumulator -= 1.0 / 60.0;
    }
    game_render(&app.game, app.indexed);
    for (size_t i = 0; i < GAME_WIDTH * GAME_HEIGHT; i++) {
        uint16_t p = gyrus_palette[app.indexed[i]];
        uint32_t r = (p >> 11) & 31u, g = (p >> 5) & 63u, b = p & 31u;
        r = (r << 3) | (r >> 2);
        g = (g << 2) | (g >> 4);
        b = (b << 3) | (b >> 2);
        app.rgba[i] = r | (g << 8) | (b << 16) | 0xff000000u;
    }
    sg_write_image_transient(
        &(sg_write_image_desc){.dst.image = app.image, .src.data = {app.rgba, sizeof app.rgba}});
    sgl_defaults();
    int x, y, size;
    viewport(&x, &y, &size);
    sgl_viewport(x, y, size, size, true);
    sgl_enable_texture();
    sgl_texture(app.view, app.sampler);
    sgl_begin_quads();
    sgl_v2f_t2f(-1, 1, 0, 0);
    sgl_v2f_t2f(1, 1, 1, 0);
    sgl_v2f_t2f(1, -1, 1, 1);
    sgl_v2f_t2f(-1, -1, 0, 1);
    sgl_end();
    sg_begin_pass(&(sg_pass){.action.colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                                                  .clear_value = {0.015f, 0.02f, 0.03f, 1}},
                             .swapchain = sglue_swapchain()});
    sgl_draw();
    sg_end_pass();
    sg_commit();
}
static void event(const sapp_event *e) {
    if (e->type == SAPP_EVENTTYPE_MOUSE_MOVE || e->type == SAPP_EVENTTYPE_MOUSE_DOWN ||
        e->type == SAPP_EVENTTYPE_MOUSE_UP) {
        app.mouse_x = e->mouse_x;
        app.mouse_y = e->mouse_y;
    }
    if (e->type == SAPP_EVENTTYPE_MOUSE_DOWN && e->mouse_button == SAPP_MOUSEBUTTON_LEFT)
        app.touch = true;
    if (e->type == SAPP_EVENTTYPE_MOUSE_UP && e->mouse_button == SAPP_MOUSEBUTTON_LEFT)
        app.touch = false;
    if (e->type == SAPP_EVENTTYPE_UNFOCUSED) {
        app.touch = app.fire = app.left = app.right = app.tilt_left = app.tilt_right = false;
    }
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN || e->type == SAPP_EVENTTYPE_KEY_UP) {
        bool down = e->type == SAPP_EVENTTYPE_KEY_DOWN;
        switch (e->key_code) {
        case SAPP_KEYCODE_LEFT:
        case SAPP_KEYCODE_A:
            app.left = down;
            break;
        case SAPP_KEYCODE_RIGHT:
        case SAPP_KEYCODE_D:
            app.right = down;
            break;
        case SAPP_KEYCODE_SPACE:
            app.fire = down;
            break;
        case SAPP_KEYCODE_ENTER:
            if (down && !e->key_repeat)
                app.start = true;
            break;
        case SAPP_KEYCODE_ESCAPE:
            if (down && !e->key_repeat)
                app.pause = true;
            break;
        case SAPP_KEYCODE_J:
            app.tilt_left = down;
            break;
        case SAPP_KEYCODE_L:
            app.tilt_right = down;
            break;
        case SAPP_KEYCODE_0:
        case SAPP_KEYCODE_K:
            if (down && !e->key_repeat) {
                app.bank_mdeg = 0;
                controls_tilt_reset(&app.controls);
            }
            break;
        case SAPP_KEYCODE_M:
            if (down && !e->key_repeat)
                controls_set_mode(
                    &app.controls,
                    (controls_mode)(((unsigned)app.controls.mode + 1u) % CONTROLS_MODE_COUNT));
            break;
        default:
            break;
        }
    }
}
static void cleanup(void) {
    disconnect_client();
    if (app.listener >= 0)
        close(app.listener);
    sgl_shutdown();
    sg_shutdown();
}
sapp_desc sokol_main(int argc, char **argv) {
    app.seed = 1;
    app.port = 20400;
    for (int i = 1; i < argc; i++) {
        if ((!strcmp(argv[i], "--debug-port") || !strcmp(argv[i], "--seed")) && i + 1 < argc) {
            bool port = !strcmp(argv[i], "--debug-port");
            char *end;
            errno = 0;
            unsigned long n = strtoul(argv[++i], &end, 10);
            if (errno || !*argv[i] || *end || argv[i][0] == '-' ||
                n > (port ? 65535u : UINT32_MAX)) {
                fputs("Invalid argument\n", stderr);
                exit(2);
            }
            if (port)
                app.port = (uint16_t)n;
            else
                app.seed = (uint32_t)n;
        } else {
            fputs("Usage: gyrus_desktop [--seed UINT32] [--debug-port PORT (0 disables)]\n",
                  stderr);
            exit(2);
        }
    }
    return (sapp_desc){.init_cb = init,
                       .frame_cb = frame,
                       .event_cb = event,
                       .cleanup_cb = cleanup,
                       .width = 720,
                       .height = 720,
                       .window_title =
                           "Gyrus 2040 — arrows/touch, J/L tilt, 0 calibrate, M mode, Space fire",
                       .high_dpi = true,
                       .sample_count = 1,
                       .logger.func = logger,
                       .ios = {.keyboard_resizes_canvas = false}};
}
