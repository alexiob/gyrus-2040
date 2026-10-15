#ifndef ENGINE_MATH_H
#define ENGINE_MATH_H
#include <stdint.h>
int16_t engine_sin_q15(uint16_t angle);
int16_t engine_cos_q15(uint16_t angle);
void engine_polar(uint16_t angle, int32_t radius_q8, int16_t *x, int16_t *y);
#endif
