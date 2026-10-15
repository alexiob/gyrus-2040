#include "waves.h"
/* Authored polar control points, not ROM data. Neptune and Uranus use mirrored
 * five-plane trains. Neptune's trains converge at four different edges;
 * Uranus uses side/top entries and a hook before assembly (JoyStik Dec1983).
 * Later families are deliberate approximations: Saturn's single file comes
 * from contemporary guide descriptions; Jupiter's sweeps and Mars's serial
 * loops follow the documented directional roles. Earth's foldback uses the
 * described lower-left/right then upper-corner convergence roles. All timing
 * and control coordinates are approximate. Q8.8 radii, full-turn65536. */
typedef struct {
    int32_t angle;
    int16_t radius;
} path_point;
static const path_point crossing[] = {{-32768, 4}, {-24576, 22}, {-16384, 54}, {-8192, 84}, {0, 94},
                                      {10000, 78}, {16000, 54},  {8000, 36},   {0, 28}};
static const path_point hook[] = {{0, 114},    {3500, 87},  {9000, 64},  {18000, 67}, {27000, 78},
                                  {35000, 62}, {39000, 42}, {36000, 30}, {32768, 28}};
static const path_point single_file[] = {{0, 112},    {4000, 86},  {10000, 64},
                                         {18000, 62}, {28000, 72}, {38000, 60},
                                         {46000, 44}, {40000, 34}, {32768, 28}};
static const path_point sweep[] = {{0, 4},      {8000, 40},  {17000, 90}, {28000, 104}, {37000, 88},
                                   {45000, 70}, {54000, 50}, {62000, 36}, {65536, 28}};
static const path_point serial_loop[] = {{0, 4},      {6000, 28},  {14000, 62},
                                         {21000, 98}, {28000, 90}, {35000, 72},
                                         {43000, 56}, {56000, 36}, {65536, 28}};
static const path_point foldback[] = {{0, 114},    {4000, 80},  {12000, 38},
                                      {26000, 22}, {38000, 40}, {50000, 74},
                                      {62000, 60}, {70000, 35}, {65536, 28}};
static const path_point *const paths[] = {crossing, hook,        single_file,
                                          sweep,    serial_loop, foldback};
static const uint16_t rotations[6][4] = {{32768, 24576, 40960, 0},     {49152, 16384, 0, 0},
                                         {57344, 8192, 40960, 24576},  {0, 16384, 49152, 32768},
                                         {16384, 49152, 16384, 57344}, {40960, 24576, 57344, 8192}};
static int32_t angle_delta(uint16_t target, uint16_t current) {
    const uint16_t delta = (uint16_t)(target - current);
    return delta <= 32767u ? (int32_t)delta : (int32_t)delta - 65536;
}
void gyrus_wave_home(uint8_t planet, uint8_t formation, uint8_t member, uint16_t clock,
                     uint16_t *angle, int32_t *radius) {
    *angle = (uint16_t)((uint32_t)formation * 16384u + (uint32_t)member * 1250u +
                        (uint32_t)planet * 1700u + (uint32_t)clock * 68u);
    *radius = (int32_t)(20u + (member / 5u) * 11u + (formation & 1u) * 4u) * 256;
}
void gyrus_wave_entry(uint8_t planet, uint8_t formation, uint8_t member, uint8_t path_planet,
                      uint8_t path_formation, uint16_t age, uint16_t *angle, int32_t *radius) {
    const unsigned group = path_formation % 4u;
    const unsigned family = path_planet % 6u;
    const path_point *path = paths[family];
    const uint16_t rotation = rotations[family][group];
    const int32_t mirror = family == 2u || family == 4u ? (group & 1u ? -1 : 1)
                           : member < 5u                ? 1
                                                        : -1;
    const uint16_t t = age > GYRUS_ENTRY_TICKS ? GYRUS_ENTRY_TICKS : age;
    unsigned segment = t / 24u;
    if (segment > 7u)
        segment = 7u;
    const int32_t fraction = (int32_t)t - (int32_t)segment * 24;
    int32_t local =
        path[segment].angle + (path[segment + 1u].angle - path[segment].angle) * fraction / 24;
    int32_t radial =
        (int32_t)path[segment].radius * 256 +
        ((int32_t)path[segment + 1u].radius - path[segment].radius) * 256 * fraction / 24;
    uint16_t a = (uint16_t)((int32_t)rotation + local * mirror);
    if (segment == 7u) {
        uint16_t home;
        int32_t home_radius;
        gyrus_wave_home(planet, formation, member, 0, &home, &home_radius);
        const uint16_t start = (uint16_t)((int32_t)rotation + path[7].angle * mirror);
        a = (uint16_t)((int32_t)start + angle_delta(home, start) * fraction / 24);
        radial = (int32_t)path[7].radius * 256 +
                 (home_radius - (int32_t)path[7].radius * 256) * fraction / 24;
    }
    *angle = a;
    *radius = radial;
}
