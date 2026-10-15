#ifndef GYRUS_BUTTONS_H
#define GYRUS_BUTTONS_H
#include <stdbool.h>
/* BOOT is flash QSPI CS, not a normal GPIO. Before using this function:
 * - Link pico_flash and pico_multicore.
 * - Core 1 must call flash_safe_execute_core_init() and publish readiness.
 * - No DMA channel may be reading XIP/flash during the sample.
 * Call from core 0's main loop, never an ISR. Output clears on failure.
 * The raw active-high sample is debounced by controls_update(). */
bool buttons_read_fire(bool *pressed);
#endif
