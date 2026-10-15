#ifndef GYRUS_SENSORS_H
#define GYRUS_SENSORS_H
#include "gyrus/diagnostic.h"
#include <stdbool.h>
#include <stdint.h>
typedef struct { bool touch_ok; uint8_t touch_id; uint8_t touch_fw; bool imu_ok; uint8_t imu_address; uint8_t imu_id; } sensor_info;
sensor_info sensors_init(void);
bool sensors_touch(diag_touch *out);
unsigned sensors_irq_count(void);
unsigned sensors_errors(void);
#endif
