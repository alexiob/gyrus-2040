#include "display.h"
#include "board.h"
#include "gyrus/diagnostic.h"
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "hardware/regs/spi.h"
#include <stddef.h>

typedef struct { uint8_t command; uint8_t count; uint8_t data[12]; } lcd_command;
static const lcd_command init_commands[] = {
#include "lcd_init.inc"
};
static int tx_channel = -1;
static uint32_t baud_hz;
static bool active, faulted, needs_full_redraw, full_transfer;
static unsigned error_count;
static uint64_t start_us, dma_us, total_us;

static void clean_rx(void) {
    while (spi_is_readable(BOARD_SPI)) (void)spi_get_hw(BOARD_SPI)->dr;
    spi_get_hw(BOARD_SPI)->icr = SPI_SSPICR_RORIC_BITS;
}

static bool wire_idle(uint64_t deadline) {
    while (spi_is_busy(BOARD_SPI)) {
        if (time_us_64() >= deadline) return false;
        tight_loop_contents();
    }
    clean_rx();
    return true;
}

static void recover(void) {
    bool aborted=true;
    if (tx_channel>=0) {
        const uint32_t mask=1u<<(uint)tx_channel;
        dma_hw->abort=mask;
        const uint64_t deadline=time_us_64()+10000u;
        while ((dma_hw->abort&mask)!=0u) {
            if (time_us_64()>=deadline) {aborted=false;break;}
            tight_loop_contents();
        }
    }
    /* Stop/reset the shifter before changing CS, even if wire idle is unreachable. */
    spi_deinit(BOARD_SPI);
    gpio_put(BOARD_LCD_CS,1);
    faulted=!aborted;
    if (aborted) baud_hz=spi_init(BOARD_SPI,baud_hz);
    /* If abort failed, leave SPI held in reset and reject every subsequent transfer. */
    needs_full_redraw=true;
    active=false;
    ++error_count;
}

static bool write8(const uint8_t *bytes,size_t n) {
    const uint64_t deadline = time_us_64()+100000u;
    spi_set_format(BOARD_SPI,8,SPI_CPOL_0,SPI_CPHA_0,SPI_MSB_FIRST);
    for (size_t i=0;i<n;++i) {
        while (!spi_is_writable(BOARD_SPI)) if (time_us_64()>=deadline) return false;
        spi_get_hw(BOARD_SPI)->dr=bytes[i];
    }
    return wire_idle(deadline);
}

static bool command(uint8_t cmd,const uint8_t *data,size_t n) {
    gpio_put(BOARD_LCD_DC,0);
    if (!write8(&cmd,1)) return false;
    if (n>0u) {
        gpio_put(BOARD_LCD_DC,1);
        if (!write8(data,n)) return false;
    }
    return true;
}

bool display_init(void) {
    const uint pins[]={BOARD_LCD_DC,BOARD_LCD_CS,BOARD_LCD_RESET,BOARD_LCD_BACKLIGHT};
    for (size_t i=0;i<sizeof pins/sizeof pins[0];++i) {
        gpio_init(pins[i]);gpio_set_dir(pins[i],GPIO_OUT);gpio_put(pins[i],0);
    }
    gpio_put(BOARD_LCD_CS,1);
    baud_hz=spi_init(BOARD_SPI,10000000u);
    gpio_set_function(BOARD_LCD_SCK,GPIO_FUNC_SPI);
    gpio_set_function(BOARD_LCD_TX,GPIO_FUNC_SPI);
    gpio_set_function(BOARD_LCD_RX,GPIO_FUNC_SPI);
    gpio_put(BOARD_LCD_RESET,1);sleep_ms(10);
    gpio_put(BOARD_LCD_RESET,0);sleep_ms(20);
    gpio_put(BOARD_LCD_RESET,1);sleep_ms(120);
    gpio_put(BOARD_LCD_CS,0);
    for (size_t i=0;i<sizeof init_commands/sizeof init_commands[0];++i) {
        const lcd_command *c=&init_commands[i];
        if (!command(c->command,c->data,c->count)) {recover();return false;}
    }
    if (!command(0x11,NULL,0)) {recover();return false;}
    sleep_ms(120);
    if (!command(0x29,NULL,0)) {recover();return false;}
    sleep_ms(20);
    gpio_put(BOARD_LCD_CS,1);
    tx_channel=dma_claim_unused_channel(false);
    if (tx_channel<0) return false;
    gpio_set_function(BOARD_LCD_BACKLIGHT,GPIO_FUNC_PWM);
    const uint slice=pwm_gpio_to_slice_num(BOARD_LCD_BACKLIGHT);
    pwm_config cfg=pwm_get_default_config();
    pwm_config_set_wrap(&cfg,999);
    pwm_config_set_clkdiv(&cfg,5.0f);
    pwm_init(slice,&cfg,true);
    pwm_set_gpio_level(BOARD_LCD_BACKLIGHT,600);
    (void)display_set_baud(40000000u);
    return true;
}

uint32_t display_set_baud(uint32_t requested) {
    if (faulted || active || requested<1000000u || requested>62500000u) return 0;
    if (!wire_idle(time_us_64()+100000u)) {recover();return 0;}
    baud_hz=spi_set_baudrate(BOARD_SPI,requested);
    return baud_hz;
}
uint32_t display_baud(void) {return baud_hz;}
bool display_busy(void) {return active;}
bool display_faulted(void) {return faulted;}
unsigned display_errors(void) {return error_count;}
uint64_t display_last_dma_us(void) {return dma_us;}
uint64_t display_last_total_us(void) {return total_us;}

bool display_start(uint16_t x,uint16_t y,uint16_t w,uint16_t h,const uint16_t *pixels,size_t count,bool dma) {
    uint8_t endpoints[8];
    const bool full=(x==0u && y==0u && w==240u && h==240u);
    if (faulted || (needs_full_redraw && !full) || active || tx_channel<0 || pixels==NULL || count!=(size_t)w*h ||
        !diag_window_encode(x,y,w,h,endpoints)) return false;
    full_transfer=full;
    start_us=time_us_64();dma_us=0;total_us=0;
    gpio_put(BOARD_LCD_CS,0);
    if (!command(0x2a,endpoints,4) || !command(0x2b,endpoints+4,4) || !command(0x2c,NULL,0)) {
        recover();return false;
    }
    gpio_put(BOARD_LCD_DC,1);
    spi_set_format(BOARD_SPI,16,SPI_CPOL_0,SPI_CPHA_0,SPI_MSB_FIRST);
    active=true;
    if (dma) {
        dma_channel_config cfg=dma_channel_get_default_config((uint)tx_channel);
        channel_config_set_transfer_data_size(&cfg,DMA_SIZE_16);
        channel_config_set_read_increment(&cfg,true);
        channel_config_set_write_increment(&cfg,false);
        channel_config_set_dreq(&cfg,spi_get_dreq(BOARD_SPI,true));
        dma_channel_configure((uint)tx_channel,&cfg,&spi_get_hw(BOARD_SPI)->dr,pixels,(uint32_t)count,true);
    } else {
        const uint64_t deadline=start_us+500000u;
        for (size_t i=0;i<count;++i) {
            while (!spi_is_writable(BOARD_SPI)) if (time_us_64()>=deadline) {recover();return false;}
            spi_get_hw(BOARD_SPI)->dr=pixels[i];
        }
        dma_us=time_us_64()-start_us; /* CPU enqueue time for the polling baseline. */
    }
    return true;
}

bool display_wait(void (*service)(void)) {
    if (!active) return false;
    const uint64_t deadline=start_us+500000u;
    while (dma_channel_is_busy((uint)tx_channel)) {
        if (time_us_64()>=deadline) {recover();return false;}
        if (service!=NULL) service();
        tight_loop_contents();
    }
    if (dma_us==0u) dma_us=time_us_64()-start_us;
    if (!wire_idle(deadline)) {recover();return false;}
    gpio_put(BOARD_LCD_CS,1);
    total_us=time_us_64()-start_us;
    active=false;
    if (full_transfer) needs_full_redraw=false;
    return true;
}
