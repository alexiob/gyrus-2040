#ifndef GYRUS_DIAGNOSTIC_H
#define GYRUS_DIAGNOSTIC_H
#include <stdbool.h>
#include <stdint.h>
#define DIAG_WIDTH 240u
#define DIAG_HEIGHT 240u
typedef struct { bool pressed; uint16_t x; uint16_t y; } diag_touch;
/* Half-open rectangle; output is big-endian x0,x1 inclusive,y0,y1 inclusive. */
bool diag_window_encode(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t out[8]);
/* Five contiguous bytes from CST816S register 0x02. Invalid reports clear out. */
bool diag_touch_decode(const uint8_t bytes[5], diag_touch *out);
/* 0 RGB bars, 1 checkerboard, 2 coordinate grid. Off-panel returns black. */
uint16_t diag_pattern_pixel(unsigned pattern, uint16_t x, uint16_t y);
typedef enum { DIAG_LINE_NONE, DIAG_LINE_READY, DIAG_LINE_INVALID, DIAG_LINE_LONG } diag_line_result;
typedef struct { char text[64]; uint8_t used; diag_line_result rejected; } diag_line;
/* Returns a complete printable ASCII command or rejects the entire malformed line. */
diag_line_result diag_line_feed(diag_line *line, uint8_t byte);
#endif
