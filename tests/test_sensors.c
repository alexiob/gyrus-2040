#include "fake.h"
#include "sensors.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t regs[128][256], addressed[128];
static bool fail_bus, ignore_ctrl3, command_timeout, locked, update_during_read, fail_data_once,
    fail_status_once;
static uint8_t shadow[12];
static uint32_t lock_wait_us;
static unsigned writes, elapsed;
void gpio_init(uint p) {
    (void)p;
}
void gpio_set_dir(uint p, bool d) {
    (void)p;
    (void)d;
}
void gpio_set_function(uint p, uint f) {
    (void)p;
    (void)f;
}
void gpio_put(uint p, bool d) {
    (void)p;
    (void)d;
}
void gpio_pull_up(uint p) {
    (void)p;
}
void gpio_set_irq_enabled_with_callback(uint p, uint32_t e, bool b, void (*cb)(uint, uint32_t)) {
    (void)p;
    (void)e;
    (void)b;
    (void)cb;
}
void busy_wait_us_32(uint32_t us) {
    lock_wait_us += us;
}
void sleep_ms(uint ms) {
    elapsed += ms;
}
uint64_t time_us_64(void) {
    return (uint64_t)elapsed * 1000u;
}
uint i2c_init(void *p, uint baud) {
    (void)p;
    assert(baud == 400000);
    return baud;
}
int i2c_write_timeout_us(void *p, uint8_t a, const uint8_t *data, size_t n, bool no_stop,
                         uint timeout) {
    (void)p;
    assert(timeout == 2000);
    if (fail_bus)
        return -1;
    if (n == 1) {
        assert(no_stop);
        addressed[a] = data[0];
        return 1;
    }
    assert(n == 2 && !no_stop);
    ++writes;
    if (!(ignore_ctrl3 && a == 0x6b && data[0] == 4))
        regs[a][data[0]] = data[1];
    if (a != 0x15 && data[0] == 0x0a) {
        if (data[1] == 0)
            regs[a][0x2d] &= 0x7fu;
        else if (data[1] == 0x12) {
            assert(regs[a][0x0b] == 1);
            if (!command_timeout)
                regs[a][0x2d] |= 0x80u;
        }
    }
    return 2;
}
int i2c_read_timeout_us(void *p, uint8_t a, uint8_t *data, size_t n, bool no_stop, uint timeout) {
    (void)p;
    assert(timeout == 2000 && !no_stop);
    if (fail_bus)
        return -1;
    if (a != 0x15 && addressed[a] == 0x2d && (regs[a][0x2d] & 1u) != 0u && !locked) {
        memcpy(shadow, &regs[a][0x35], 12);
        locked = true;
        lock_wait_us = 0;
        if (update_during_read) {
            regs[a][0x35] = 0;
            regs[a][0x36] = 0;
        }
    }
    if (a != 0x15 && addressed[a] == 0x2d && fail_status_once) {
        fail_status_once = false;
        return -1;
    }
    if (a != 0x15 && addressed[a] == 0x35 && n == 12u) {
        assert(locked && lock_wait_us >= 12u);
        if (fail_data_once) {
            fail_data_once = false;
            return -1;
        }
        memcpy(data, shadow, n);
        locked = false;
        regs[a][0x2d] &= 0xfcu;
        return (int)n;
    }
    for (size_t i = 0; i < n; ++i)
        data[i] = regs[a][addressed[a] + i];
    if (a != 0x15 && addressed[a] == 0x40) {
        locked = false;
        regs[a][0x2d] &= 0xfcu;
    }
    return (int)n;
}
static void fixture(uint8_t address) {
    fail_bus = false;
    ignore_ctrl3 = false;
    command_timeout = false;
    locked = false;
    fail_data_once = false;
    fail_status_once = false;
    update_during_read = false;
    memset(regs, 0, sizeof regs);
    regs[0x15][0xa7] = 0xb5;
    regs[0x15][0xa9] = 1;
    regs[address][0] = 5;
    writes = 0;
}
static void raw(uint8_t a, unsigned offset, int value) {
    uint16_t v = (uint16_t)value;
    regs[a][0x35 + offset] = (uint8_t)v;
    regs[a][0x36 + offset] = (uint8_t)(v >> 8);
}
static void physical_bank_axes_match_display(void) {
    controls_state input;
    controls_init(&input);
    controls_set_mode(&input, CONTROLS_TILT);
    controls_sample sample = {.motion_valid = true};
    for (uint32_t ms = 0; ms <= 600; ms += 20) {
        sample.now_ms = ms;
        sample.motion = (controls_motion){.accel_mg = {0, 0, -1000}, .sampled_ms = ms};
        sensors_motion_to_display(&sample.motion);
        (void)controls_update(&input, sample, CONTROLS_PLAY);
    }
    assert(controls_tilt_ready(&input));
    int delta = 0;
    /* Real board poses captured with its right, then left edge held lower. */
    for (uint32_t ms = 620; ms <= 1620; ms += 20) {
        sample.now_ms = ms;
        sample.motion = (controls_motion){.accel_mg = {140, 623, -795}, .sampled_ms = ms};
        sensors_motion_to_display(&sample.motion);
        delta = controls_update(&input, sample, CONTROLS_PLAY).input.angle_delta;
    }
    assert(delta > 500);
    for (uint32_t ms = 1640; ms <= 2640; ms += 20) {
        sample.now_ms = ms;
        sample.motion = (controls_motion){.accel_mg = {-23, -510, -806}, .sampled_ms = ms};
        sensors_motion_to_display(&sample.motion);
        delta = controls_update(&input, sample, CONTROLS_PLAY).input.angle_delta;
    }
    assert(delta < -500);
    /* Tipping front/back is the other body axis and must not steer. */
    for (uint32_t ms = 2660; ms <= 4660; ms += 20) {
        sample.now_ms = ms;
        sample.motion = (controls_motion){.accel_mg = {500, 0, -866}, .sampled_ms = ms};
        sensors_motion_to_display(&sample.motion);
        delta = controls_update(&input, sample, CONTROLS_PLAY).input.angle_delta;
    }
    assert(delta == 0);
    controls_motion gyro = {.gyro_mdps = {12300, -45600, 78900}, .sampled_ms = 123};
    sensors_motion_to_display(&gyro);
    assert(gyro.gyro_mdps[0] == -45600 && gyro.gyro_mdps[1] == -12300 &&
           gyro.gyro_mdps[2] == 78900 && gyro.sampled_ms == 123);
    sensors_motion_to_display(NULL);
}
int main(void) {
    physical_bank_axes_match_display();
    fixture(0x6b);
    sensor_info info = sensors_init();
    assert(info.imu_ok && info.touch_ok && info.imu_address == 0x6b);
    assert(regs[0x6b][2] == 0x40 && regs[0x6b][3] == 0x16 && regs[0x6b][4] == 0x56 &&
           regs[0x6b][6] == 0x77 && regs[0x6b][8] == 0x83);
    assert(writes == 13);
    regs[0x6b][0x2d] = 1;
    raw(0x6b, 0, 8192);
    raw(0x6b, 2, -8192);
    raw(0x6b, 4, 4096);
    raw(0x6b, 6, 64);
    raw(0x6b, 8, -128);
    raw(0x6b, 10, 32767);
    controls_motion out;
    assert(sensors_motion(&out));
    assert(out.accel_mg[0] == 1000 && out.accel_mg[1] == -1000 && out.accel_mg[2] == 500);
    assert(out.gyro_mdps[0] == 1000 && out.gyro_mdps[1] == -2000 && out.gyro_mdps[2] == 511984);
    assert(out.sampled_ms == elapsed);
    unsigned previous = sensors_errors();
    regs[0x6b][0x2d] = 0;
    assert(!sensors_motion(&out));
    assert(out.accel_mg[0] == 0 && sensors_errors() == previous);
    regs[0x6b][0x2d] = 1;
    update_during_read = true;
    assert(sensors_motion(&out) && out.accel_mg[0] == 1000);
    assert(regs[0x6b][0x36] == 0);
    update_during_read = false;
    fail_bus = true;
    assert(!sensors_motion(&out));
    assert(out.gyro_mdps[0] == 0 && sensors_errors() > previous);
    fail_bus = false;
    /* A burst timeout leaves the shadow locked; recovery discards it. */
    fixture(0x6b);
    info = sensors_init();
    assert(info.imu_ok);
    regs[0x6b][0x2d] = 1;
    raw(0x6b, 0, 8192);
    fail_data_once = true;
    assert(!sensors_motion(&out) && locked);
    assert(!sensors_motion(&out) && !locked);
    regs[0x6b][0x2d] = 1;
    raw(0x6b, 0, -8192);
    assert(sensors_motion(&out) && out.accel_mg[0] == -1000);
    /* Failed status transactions may already have triggered the device lock. */
    regs[0x6b][0x2d] = 1;
    fail_status_once = true;
    assert(!sensors_motion(&out) && locked);
    assert(!sensors_motion(&out) && !locked);
    regs[0x6b][0x2d] = 1;
    raw(0x6b, 0, 4096);
    assert(sensors_motion(&out) && out.accel_mg[0] == 500);
    fixture(0x6a);
    info = sensors_init();
    assert(info.imu_ok && info.imu_address == 0x6a);
    regs[0x6a][0x2d] = 1;
    raw(0x6a, 4, 8192);
    assert(sensors_motion(&out) && out.accel_mg[2] == 1000);
    fixture(0x6b);
    ignore_ctrl3 = true;
    info = sensors_init();
    assert(!info.imu_ok);
    assert(!sensors_motion(&out));
    fixture(0x6b);
    command_timeout = true;
    unsigned start = elapsed;
    info = sensors_init();
    assert(!info.imu_ok && elapsed - start < 500u);
    assert(!sensors_motion(&out));
    puts("sensor config, signed LE scaling, fresh-data, timeout, fallback-address, readback, "
         "coherent-lock, and lock-recovery tests passed");
    return 0;
}
