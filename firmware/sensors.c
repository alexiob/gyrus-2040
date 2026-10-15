#include "sensors.h"
#include "board.h"
#include "pico/stdlib.h"
#include <stddef.h>
static volatile unsigned irq_count;
static unsigned errors;
static uint8_t motion_address;
static bool motion_needs_unlock;
static void touch_irq(uint gpio, uint32_t events) {
    (void)gpio;
    (void)events;
    ++irq_count;
}
static bool read_regs(uint8_t addr, uint8_t reg, uint8_t *data, size_t size) {
    int a = i2c_write_timeout_us(BOARD_I2C, addr, &reg, 1, true, 2000);
    int b = a == 1 ? i2c_read_timeout_us(BOARD_I2C, addr, data, size, false, 2000) : -1;
    if (b != (int)size) {
        ++errors;
        return false;
    }
    return true;
}
static bool write_at(uint8_t address, uint8_t reg, uint8_t value) {
    const uint8_t data[2] = {reg, value};
    if (i2c_write_timeout_us(BOARD_I2C, address, data, 2, false, 2000) != 2) {
        ++errors;
        return false;
    }
    return true;
}
static bool write_reg(uint8_t reg, uint8_t value) {
    return write_at(0x15, reg, value);
}
static bool wait_command(uint8_t address, bool done) {
    uint64_t deadline = time_us_64() + 50000u;
    do {
        uint8_t status = 0;
        if (!read_regs(address, 0x2d, &status, 1))
            return false;
        if (((status & 0x80u) != 0u) == done)
            return true;
        sleep_ms(1);
    } while (time_us_64() < deadline);
    ++errors;
    return false;
}
static bool disable_ahb_gating(uint8_t address) {
    /* QST CTRL9 handshake: clear a previous command, pass CAL1_L=1,
     * apply command0x12, awaitCmdDone, acknowledge0 and awaitclear. */
    return write_at(address, 0x0a, 0) && wait_command(address, false) &&
           write_at(address, 0x0b, 1) && write_at(address, 0x0a, 0x12) &&
           wait_command(address, true) && write_at(address, 0x0a, 0) &&
           wait_command(address, false);
}
static bool configure_motion(uint8_t address) {
    /* QST QMI8658A/C register tables: CTRL1 auto-increment/little-endian,
     * CTRL2 +/-4g, CTRL3 +/-512dps, both ODR code6 (125Hz nominal C;
     * ~112Hz in the A revision's 6DOF mode), LPF bandwidth13.37% ODR.
     * The downloaded older vendor enum is off by one for gyro full-scale:
     * 512dps is gFS=101, not100. 64LSB/dps therefore requires0x56. */
    static const uint8_t config[][2] = {{8, 0}, {2, 0x40}, {3, 0x16}, {4, 0x56}, {6, 0x77}, {9, 0}};
    for (unsigned i = 0; i < sizeof config / sizeof config[0]; ++i)
        if (!write_at(address, config[i][0], config[i][1]))
            return false;
    /* I2C SyncSample requires AHB clock gating disabled before locking
     * (QST sections13.2.1–13.2.3). Never accept an unacknowledged command. */
    if (!disable_ahb_gating(address) || !write_at(address, 8, 0x83))
        return false;
    const uint8_t registers[] = {2, 3, 4, 6, 8}, expected[] = {0x40, 0x16, 0x56, 0x77, 0x83};
    for (unsigned i = 0; i < sizeof registers; ++i) {
        uint8_t value = 0;
        if (!read_regs(address, registers[i], &value, 1) || value != expected[i]) {
            ++errors;
            return false;
        }
    }
    sleep_ms(180); /* Gyro startup:150ms+3/ODR, per QST datasheet. */
    return true;
}
sensor_info sensors_init(void) {
    sensor_info info = {0};
    motion_address = 0;
    motion_needs_unlock = false;
    i2c_init(BOARD_I2C, 400000);
    gpio_set_function(BOARD_SDA, GPIO_FUNC_I2C);
    gpio_set_function(BOARD_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(BOARD_SDA);
    gpio_pull_up(BOARD_SCL);
    gpio_init(BOARD_TOUCH_RESET);
    gpio_set_dir(BOARD_TOUCH_RESET, GPIO_OUT);
    gpio_put(BOARD_TOUCH_RESET, 0);
    sleep_ms(20);
    gpio_put(BOARD_TOUCH_RESET, 1);
    sleep_ms(100);
    uint8_t id = 0;
    if (read_regs(0x15, 0xa7, &id, 1) && id != 0u && id != 255u) {
        info.touch_id = id;
        info.touch_ok =
            read_regs(0x15, 0xa9, &info.touch_fw, 1) && write_reg(0xfe, 1) && write_reg(0xfa, 0x60);
    }
    const uint8_t addresses[2] = {0x6b, 0x6a};
    for (unsigned i = 0; i < 2u; ++i) {
        if (read_regs(addresses[i], 0, &id, 1) && id == 5u) {
            info.imu_address = addresses[i];
            info.imu_id = id;
            (void)read_regs(addresses[i], 1, &info.imu_revision, 1);
            info.imu_ok = configure_motion(addresses[i]);
            if (info.imu_ok)
                motion_address = addresses[i];
            break;
        }
    }
    gpio_init(BOARD_TOUCH_IRQ);
    gpio_set_dir(BOARD_TOUCH_IRQ, GPIO_IN);
    gpio_pull_up(BOARD_TOUCH_IRQ);
    gpio_set_irq_enabled_with_callback(BOARD_TOUCH_IRQ, GPIO_IRQ_EDGE_FALL, true, touch_irq);
    return info;
}
bool sensors_touch(diag_touch *out) {
    uint8_t bytes[5];
    if (out == NULL)
        return false;
    *out = (diag_touch){0};
    return read_regs(0x15, 2, bytes, sizeof bytes) && diag_touch_decode(bytes, out);
}
static int32_t signed_le16(const uint8_t *bytes) {
    uint32_t raw = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8u);
    return raw >= 32768u ? (int32_t)raw - 65536 : (int32_t)raw;
}
bool sensors_motion(controls_motion *out) {
    if (out == NULL)
        return false;
    *out = (controls_motion){0};
    if (motion_address == 0u)
        return false;
    uint8_t status = 0, bytes[12];
    if (motion_needs_unlock) {
        /* A previous partial transaction may have left a sample locked. Drop
         * that stale sample before waiting for the next fresh acquisition. */
        uint8_t ignored = 0;
        if (read_regs(motion_address, 0x40, &ignored, 1))
            motion_needs_unlock = false;
        return false;
    }
    if (!read_regs(motion_address, 0x2d, &status, 1)) {
        /* The sensor may have observed the status read even if I2C timed out. */
        motion_needs_unlock = true;
        return false;
    }
    if ((status & 1u) == 0u)
        return false;
    /* Reading STATUSINT withAvail=1 starts the hardware shadow-register lock.
     * Wait the specified12us for gyroODR6 even ifLocked already reports1. */
    busy_wait_us_32(12);
    if (!read_regs(motion_address, 0x35, bytes, sizeof bytes)) {
        motion_needs_unlock = true;
        return false;
    }
    /* Burst includesGZ_H (0x40), automatically releasing the locked sample. */
    for (unsigned i = 0; i < 3u; ++i) {
        out->accel_mg[i] = signed_le16(bytes + 2u * i) * 1000 / 8192;
        out->gyro_mdps[i] = signed_le16(bytes + 6u + 2u * i) * 1000 / 64;
    }
    out->sampled_ms = (uint32_t)(time_us_64() / 1000u);
    return true;
}
unsigned sensors_irq_count(void) {
    return irq_count;
}
unsigned sensors_errors(void) {
    return errors;
}

void sensors_motion_to_display(controls_motion *motion) {
    if (motion == NULL)
        return;
    /* The mounted IMU is rotated a quarter-turn relative to the LCD. Keep
     * a proper rotation for both vectors so gyro prediction agrees with gravity. */
    int32_t ax = motion->accel_mg[0], gx = motion->gyro_mdps[0];
    motion->accel_mg[0] = motion->accel_mg[1];
    motion->accel_mg[1] = -ax;
    motion->gyro_mdps[0] = motion->gyro_mdps[1];
    motion->gyro_mdps[1] = -gx;
}
