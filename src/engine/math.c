#include "engine/math.h"

static const int16_t sine[256] = {
#include "../sine_table.inc"
};
int16_t engine_sin_q15(uint16_t angle) {
    return sine[angle >> 8u];
}
int16_t engine_cos_q15(uint16_t angle) {
    return sine[(uint8_t)((angle >> 8u) + 64u)];
}
void engine_polar(uint16_t angle, int32_t radius, int16_t *x, int16_t *y) {
    if (radius > 200 * 256)
        radius = 200 * 256;
    if (radius < -200 * 256)
        radius = -200 * 256;
    *x = (int16_t)(120 + ((int32_t)engine_sin_q15(angle) * radius) / 8388608);
    *y = (int16_t)(120 - ((int32_t)engine_cos_q15(angle) * radius) / 8388608);
}
