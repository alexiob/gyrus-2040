#include "buttons.h"
#include "hardware/regs/io_qspi.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include "pico/flash.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include <stddef.h>

/* Both cores/interrupts are quiescent before QSPI CS is made high impedance.
 * This entire callback, including delay, must execute from SRAM. */
static void __no_inline_not_in_flash_func(sample_bootsel)(void *parameter) {
    bool *pressed = (bool *)parameter;
    uint32_t saved = ioqspi_hw->io[1].ctrl;
    ioqspi_hw->io[1].ctrl = (saved & ~IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS) |
                            ((uint32_t)IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_VALUE_DISABLE
                             << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB);
    /* Deliberately inline, no SDK sleep call or flash-resident literal table. */
    for (volatile unsigned i = 0; i < 1000u; ++i)
        __asm volatile("nop");
    *pressed = (sio_hw->gpio_hi_in & (1u << 1u)) == 0u;
    ioqspi_hw->io[1].ctrl = saved;
}
bool buttons_read_fire(bool *pressed) {
    if (pressed == NULL)
        return false;
    *pressed = false;
    /* Avoid the SDK's unsafe-zone assertion when startup has not finished.
     * A caller must still wait for core1 readiness before normal sampling. */
    if (get_core_num() != 0u || !multicore_lockout_victim_is_initialized(1u))
        return false;
    bool ok = flash_safe_execute(sample_bootsel, pressed, 2u) == PICO_OK;
    if (!ok)
        *pressed = false;
    return ok;
}
