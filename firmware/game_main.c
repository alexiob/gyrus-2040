#include "buttons.h"
#include "display.h"
#include "gyrus/assets.h"
#include "gyrus/controls.h"
#include "gyrus/debug_game.h"
#include "gyrus/video.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "pico/flash.h"
#include "pico/multicore.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "sensors.h"
#include "tusb.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define FRAME_PIXELS (GAME_WIDTH * GAME_HEIGHT)
#define STRIP_ROWS 16u
#define STRIP_PIXELS (GAME_WIDTH * STRIP_ROWS)
#define FRAME_COUNT 2u
#ifndef GYRUS_PRESENT_HZ
#define GYRUS_PRESENT_HZ 60u
#endif
_Static_assert(GYRUS_PRESENT_HZ == 30u || GYRUS_PRESENT_HZ == 60u, "presentation must be30/60Hz");

/* Every frame index has exactly one owner: free queue, core0 rendering/debug,
 * ready queue, or core1 presentation. Only SRAM strip buffers are DMA sources. */
static uint8_t indexed[FRAME_COUNT][FRAME_PIXELS];
static uint16_t strips[2][STRIP_PIXELS];
static queue_t free_frames, ready_frames;
static game_state game;
static controls_state controls;
static controls_motion last_motion;
static bool have_motion;
static gyrus_debug_game adapter;
static gyrus_debug protocol;
static gyrus_debug_callbacks game_callbacks;
static sensor_info sensors;
static volatile bool core1_ready, lcd_ok;
static volatile uint32_t presented_frames, present_last_us, present_max_us, lcd_error_count,
    lcd_baud;
static uint32_t rendered_frames, dropped_frames, render_last_us, render_max_us, sim_max_us,
    touch_failures, button_failures, sim_overruns;
static int debug_frame_index = -1;
static uint64_t usb_reply_deadline;
static bool usb_reply_aborted;
static bool usb_transport_failed;

static bool present(uint8_t frame_index, bool full) {
    video_span span = video_strip_span(0, full);
    size_t count = video_expand_strip(indexed[frame_index], 0, full, strips[0], STRIP_PIXELS);
    if (!display_start(span.x, 0, span.width, STRIP_ROWS, strips[0], count, true))
        return false;
    for (unsigned strip = 1; strip < VIDEO_STRIPS; ++strip) {
        unsigned target = strip & 1u;
        span = video_strip_span(strip, full);
        /* Convert while DMA reads the other immutable SRAM strip. */
        count = video_expand_strip(indexed[frame_index], strip, full, strips[target], STRIP_PIXELS);
        if (!display_wait(NULL))
            return false;
        if (!display_start(span.x, (uint16_t)(strip * STRIP_ROWS), span.width, STRIP_ROWS,
                           strips[target], count, true))
            return false;
    }
    return display_wait(NULL);
}

static void display_core(void) {
    bool flash_ready = flash_safe_execute_core_init();
    bool initialized = display_init();
    if (initialized)
        initialized = display_set_baud(62500000u) == 62500000u;
    lcd_baud = display_baud();
    lcd_ok = initialized;
    /* Publishing readiness happens after both lockout registration and display
     * initialization. A failed registration prevents all BOOT samples. */
    __dmb();
    core1_ready = flash_ready;
    __sev();
    bool seeded = false;
    for (;;) {
        uint8_t index;
        if (!queue_try_remove(&ready_frames, &index)) {
            sleep_us(50);
            continue;
        }
        if (lcd_ok) {
            uint64_t began = time_us_64();
            if (present(index, !seeded)) {
                seeded = true;
                uint32_t elapsed = (uint32_t)(time_us_64() - began);
                present_last_us = elapsed;
                if (elapsed > present_max_us)
                    present_max_us = elapsed;
                ++presented_frames;
            } else {
                /* display.c requests a full redraw after recovery, whereas this
                 * backend streams strips. Latch disabled; never reuse faulted
                 * DMA strip storage, and keep controls/USB available. */
                lcd_ok = false;
            }
            lcd_error_count = display_errors();
        }
        queue_add_blocking(&free_frames, &index);
    }
}

static controls_screen screen(void) {
    switch (game.phase) {
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

static void sync_ui(uint32_t now_ms) {
    game.control_mode = (uint8_t)controls.mode;
    game.reboot_progress = controls_reboot_progress(&controls, now_ms);
    game.tilt_ready = controls_tilt_ready(&controls);
}

static void simulate(void) {
    uint64_t began = time_us_64();
    diag_touch touch = {0};
    bool touch_invalid = false;
    if (!sensors.touch_ok || !sensors_touch(&touch)) {
        touch_invalid = true;
        controls_cancel_touch(&controls);
        ++touch_failures;
    }
    bool fire = false;
    if (core1_ready && !buttons_read_fire(&fire))
        ++button_failures;
    uint32_t now_ms = (uint32_t)(time_us_64() / 1000u);
    controls_sample sample = {.now_ms = now_ms,
                              .touch_pressed = touch.pressed,
                              .touch_x = touch.x,
                              .touch_y = touch.y,
                              .fire_pressed = fire,
                              .touch_invalid = touch_invalid};
    sample.motion_valid = sensors_motion(&sample.motion);
    /* IMU timestamps are taken after I2C completion. Sample the frame clock
     * afterward so a fresh reading cannot appear to come from the future. */
    now_ms = (uint32_t)(time_us_64() / 1000u);
    sample.now_ms = now_ms;
    if (sample.motion_valid) {
        last_motion = sample.motion;
        have_motion = true;
    }
    controls_output local = controls_update(&controls, sample, screen());
    if (local.reboot_requested) {
        watchdog_reboot(0, 0, 10);
        for (;;)
            tight_loop_contents();
    }
    game_input retained = adapter.input;
    if (!adapter.frozen) {
        if (local.input.turn != 0)
            adapter.input.turn = local.input.turn;
        /* A finger movement takes precedence over an injected relative pulse;
         * persistent injected turn/fire otherwise remain available. */
        if (local.input.angle_delta != 0)
            adapter.input.angle_delta = local.input.angle_delta;
        adapter.input.fire = adapter.input.fire || local.input.fire;
        adapter.input.start = adapter.input.start || local.input.start;
        adapter.input.pause = adapter.input.pause || local.input.pause;
        adapter.input.quit = adapter.input.quit || local.input.quit;
        gyrus_debug_game_tick(&adapter);
        adapter.input.turn = retained.turn;
        adapter.input.fire = retained.fire;
    }
    sync_ui(now_ms);
    uint32_t elapsed = (uint32_t)(time_us_64() - began);
    if (elapsed > sim_max_us)
        sim_max_us = elapsed;
}

static void render_next(void) {
    uint8_t index;
    if (!queue_try_remove(&free_frames, &index)) {
        ++dropped_frames;
        return;
    }
    uint64_t began = time_us_64();
    game_render(&game, indexed[index]);
    render_last_us = (uint32_t)(time_us_64() - began);
    if (render_last_us > render_max_us)
        render_max_us = render_last_us;
    ++rendered_frames;
    queue_add_blocking(&ready_frames, &index);
}

static void write_usb(void *context, const void *bytes, size_t count) {
    (void)context;
    const char *cursor = bytes;
    if (usb_reply_aborted)
        return;
    while (count > 0u) {
        /* TinyUSB has one foreground owner in this firmware. Its USB IRQ only
         * queues events; stdio's background task is disabled for this target. */
        tud_task();
        if (!stdio_usb_connected() || time_us_64() >= usb_reply_deadline) {
            usb_reply_aborted = true;
            usb_transport_failed = true;
            (void)tud_cdc_write_clear();
            return;
        }
        uint32_t available = tud_cdc_write_available();
        uint32_t chunk = count > available ? available : (uint32_t)count;
        if (chunk > 0u) {
            uint32_t accepted = tud_cdc_write(cursor, chunk);
            cursor += accepted;
            count -= accepted;
        }
        (void)tud_cdc_write_flush();
        if (chunk == 0u)
            sleep_us(50);
    }
}

static bool debug_frame(void *context, gyrus_debug_frame *out) {
    gyrus_debug_game *debug = context;
    /* On core0 no new frame can be queued during this callback. When both
     * buffers are free, every earlier queued presentation is complete. */
    uint64_t deadline = time_us_64() + 1000000u;
    while (queue_get_level(&free_frames) != FRAME_COUNT) {
        if (time_us_64() >= deadline)
            return false;
        sleep_us(50);
    }
    uint8_t index;
    if (!queue_try_remove(&free_frames, &index))
        return false;
    debug_frame_index = index;
    debug->framebuffer = indexed[index];
    sync_ui((uint32_t)(time_us_64() / 1000u));
    return game_callbacks.frame(context, out);
}

static bool debug_state(void *context, char *out, size_t capacity) {
    if (!game_callbacks.state(context, out, capacity))
        return false;
    size_t used = strlen(out);
    if (used < 1u)
        return false;
    /* Core1 owns these aligned uint32_t stores. Each metric is a complete
     * single-word snapshot; fields may describe adjacent frames, not one atomic
     * multi-field sample. No shared 64-bit accesses or extra lockout locks. */
    int added = snprintf(
        out + used - 1u, capacity - used + 1u,
        ",\"hardware\":{\"firmware\":\"gyrus_game\",\"cpu_hz\":%" PRIu32 ",\"peri_hz\":%" PRIu32
        ",\"spi_hz\":%" PRIu32 ",\"display_disabled\":%s,\"display_errors\":%" PRIu32
        ",\"rendered_frames\":%" PRIu32 ",\"presented_frames\":%" PRIu32
        ",\"dropped_frames\":%" PRIu32 ",\"render_us\":%" PRIu32 ",\"render_max_us\":%" PRIu32
        ",\"last_present_us\":%" PRIu32 ",\"max_present_us\":%" PRIu32 ",\"sim_max_us\":%" PRIu32
        ",\"touch_errors\":%" PRIu32 ",\"touch_irq\":%u,\"bootsel_read_failures\":%" PRIu32
        ",\"sim_overruns\":%" PRIu32 ",\"control_mode\":%u,\"imu_ok\":%s,"
        "\"imu_revision\":%u,\"motion_age_ms\":%" PRIu32 ",\"accel_mg\":[%" PRId32 ",%" PRId32 ",%" PRId32 "],"
        "\"gyro_mdps\":[%" PRId32 ",%" PRId32 ",%" PRId32 "]}}",
        clock_get_hz(clk_sys), clock_get_hz(clk_peri), lcd_baud, lcd_ok ? "false" : "true",
        lcd_error_count, rendered_frames, presented_frames, dropped_frames, render_last_us,
        render_max_us, present_last_us, present_max_us, sim_max_us, touch_failures,
        sensors_irq_count(), button_failures, sim_overruns, (unsigned)controls.mode,
        sensors.imu_ok ? "true" : "false", (unsigned)sensors.imu_revision,
        have_motion ? (uint32_t)(time_us_64() / 1000u) - last_motion.sampled_ms : UINT32_MAX,
        last_motion.accel_mg[0], last_motion.accel_mg[1], last_motion.accel_mg[2],
        last_motion.gyro_mdps[0], last_motion.gyro_mdps[1], last_motion.gyro_mdps[2]);
    return added > 0 && (size_t)added < capacity - used + 1u;
}

static void service_usb(void) {
    /* Bound each pass to preserve the simulation cadence under a noisy host.
     * Feed one byte at a time so each FRAME reservation is released before a
     * following command in the same USB packet can request another FRAME. */
    for (unsigned i = 0; i < 256u; ++i) {
        int ch = getchar_timeout_us(0);
        if (ch == PICO_ERROR_TIMEOUT)
            break;
        uint8_t byte = (uint8_t)ch;
        usb_reply_deadline = time_us_64() + 5000000u;
        usb_reply_aborted = false;
        gyrus_debug_feed(&protocol, &byte, 1u);
        if (debug_frame_index >= 0) {
            uint8_t index = (uint8_t)debug_frame_index;
            adapter.framebuffer = NULL;
            queue_add_blocking(&free_frames, &index);
            debug_frame_index = -1;
        }
        if (usb_reply_aborted)
            break;
    }
}

int main(void) {
    (void)set_sys_clock_khz(125000, true);
    /* The SDK parks clk_peri on the 48MHz USB PLL after a system-clock change.
     * Restore 125MHz explicitly so the SPI divider can produce 62.5MHz. */
    uint32_t sys_hz = clock_get_hz(clk_sys);
    (void)clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, sys_hz, sys_hz);
    stdio_init_all();
    if (!queue_init(&free_frames, sizeof(uint8_t), FRAME_COUNT) ||
        !queue_init(&ready_frames, sizeof(uint8_t), FRAME_COUNT))
        return 1;
    for (uint8_t i = 0; i < FRAME_COUNT; ++i)
        queue_add_blocking(&free_frames, &i);
    game_init(&game, 2040u);
    controls_init(&controls);
    gyrus_debug_game_init(&adapter, &game, NULL, write_usb, NULL);
    adapter.controls = &controls;
    game_callbacks = gyrus_debug_game_callbacks(&adapter);
    gyrus_debug_callbacks callbacks = game_callbacks;
    callbacks.frame = debug_frame;
    callbacks.state = debug_state;
    gyrus_debug_init(&protocol, &callbacks);
    multicore_launch_core1(display_core);
    sensors = sensors_init();
    /* BOOT remains unavailable until core1 publishes lockout readiness. The
     * simulation itself does not wait indefinitely for display initialization. */
    uint64_t next_tick = time_us_64();
    unsigned remainder = 0, render_divider = 0;
    bool was_connected = false;
    for (;;) {
        tud_task();
        bool connected = stdio_usb_connected();
        if (!connected) {
            /* A DTR-only reconnect does not reset TinyUSB endpoint FIFOs. */
            tud_cdc_read_flush();
            if (was_connected || usb_transport_failed) {
                gyrus_debug_init(&protocol, &callbacks);
                (void)tud_cdc_write_clear();
            }
            usb_transport_failed = false;
        }
        was_connected = connected;
        if (connected && !usb_transport_failed)
            service_usb();
        unsigned catchup = 0;
        while (time_us_64() >= next_tick && catchup < 4u) {
            simulate();
            next_tick += 1000000u / GAME_TICK_HZ;
            remainder += 1000000u % GAME_TICK_HZ;
            if (remainder >= GAME_TICK_HZ) {
                ++next_tick;
                remainder -= GAME_TICK_HZ;
            }
            ++catchup;
            if (++render_divider == GAME_TICK_HZ / GYRUS_PRESENT_HZ) {
                render_divider = 0;
                render_next();
            }
        }
        /* Only discard backlog after exhausting the catch-up budget. A tick
         * becoming due after the while condition must run on the next pass. */
        if (catchup == 4u && time_us_64() >= next_tick) {
            ++sim_overruns;
            next_tick = time_us_64() + 1000000u / GAME_TICK_HZ;
            remainder = 0;
        }
        sleep_us(50);
    }
}
