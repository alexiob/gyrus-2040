#ifndef GYRUS_FAKE_SENSOR_H
#define GYRUS_FAKE_SENSOR_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef unsigned uint;
#define spi1 ((void *)0)
#define i2c1 ((void *)0)
enum { GPIO_FUNC_I2C, GPIO_OUT, GPIO_IN, GPIO_IRQ_EDGE_FALL };
void gpio_init(uint);
void gpio_set_dir(uint, bool);
void gpio_set_function(uint, uint);
void gpio_put(uint, bool);
void gpio_pull_up(uint);
void gpio_set_irq_enabled_with_callback(uint, uint32_t, bool, void (*)(uint, uint32_t));
void busy_wait_us_32(uint32_t);
void sleep_ms(uint);
uint64_t time_us_64(void);
uint i2c_init(void *, uint);
int i2c_write_timeout_us(void *, uint8_t, const uint8_t *, size_t, bool, uint);
int i2c_read_timeout_us(void *, uint8_t, uint8_t *, size_t, bool, uint);
#endif
