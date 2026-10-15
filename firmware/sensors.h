#ifndef GYRUS_SENSORS_H
#define GYRUS_SENSORS_H
#include "gyrus/controls.h"
#include "gyrus/diagnostic.h"
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    bool touch_ok;
    uint8_t touch_id;
    uint8_t touch_fw;
    bool imu_ok;
    uint8_t imu_address;
    uint8_t imu_id;
    uint8_t imu_revision;
} sensor_info;
sensor_info sensors_init(void);
bool sensors_touch(diag_touch *out);
/* Fresh 6-axis sample: acceleration mg, angular velocity millidegrees/second.
 * Both ranges configured at startup: +/-4g and +/-512dps, ODR code6 (~125Hz).
 * Hardware SyncSample lock makes each six-axis burst coherent.
 * Core0 only; shares the bounded I2C bus with touch. False also means no new
 * data this tick; output clears on false. No allocation or floating point. */
bool sensors_motion(controls_motion *out);
/* Board mounting to display frame: X=rawY, Y=-rawX, Z=rawZ.
 * Apply to both vectors before controls_update; raw telemetry stays sensor-frame. */
void sensors_motion_to_display(controls_motion *motion);
unsigned sensors_irq_count(void);
unsigned sensors_errors(void);
#endif
