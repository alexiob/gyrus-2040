#ifndef GYRUS_WAVES_H
#define GYRUS_WAVES_H
#include <stdint.h>
#define GYRUS_ENTRY_TICKS 192u
#define GYRUS_GROUP_TICKS 216u
#define GYRUS_MEMBER_TICKS 9u
void gyrus_wave_entry(uint8_t planet, uint8_t formation, uint8_t member, uint8_t path_planet,
                      uint8_t path_formation, uint16_t age, uint16_t *angle, int32_t *radius);
void gyrus_wave_home(uint8_t planet, uint8_t formation, uint8_t member, uint16_t clock,
                     uint16_t *angle, int32_t *radius);
#endif
