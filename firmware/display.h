#ifndef GYRUS_DISPLAY_H
#define GYRUS_DISPLAY_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* All calls and DMA ownership are on core 0 in this diagnostic baseline.
 * Source must remain immutable through completion. Command changes require wire idle. */
bool display_init(void);
uint32_t display_set_baud(uint32_t requested);
uint32_t display_baud(void);
bool display_start(uint16_t x,uint16_t y,uint16_t w,uint16_t h,const uint16_t *pixels,size_t count,bool dma);
bool display_wait(void (*service)(void));
bool display_busy(void);
uint64_t display_last_dma_us(void);
uint64_t display_last_total_us(void);
unsigned display_errors(void);
#endif
