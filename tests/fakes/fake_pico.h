#ifndef TEST_FAKE_PICO_H
#define TEST_FAKE_PICO_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef unsigned uint;
typedef struct { volatile uint32_t dr,icr; } spi_inst_t;
extern spi_inst_t fake_spi;
#define spi1 (&fake_spi)
#define i2c1 ((void *)0)
typedef struct { volatile uint32_t abort; } dma_hw_t;
extern dma_hw_t fake_dma;
#define dma_hw (&fake_dma)
typedef struct { unsigned unused; } dma_channel_config;
typedef struct { unsigned unused; } pwm_config;
enum { GPIO_OUT=1, GPIO_FUNC_SPI=2, GPIO_FUNC_PWM=3, SPI_CPOL_0=0, SPI_CPHA_0=0, SPI_MSB_FIRST=0, DMA_SIZE_16=2, SPI_SSPICR_RORIC_BITS=1 };
void gpio_init(uint p);void gpio_set_dir(uint p,bool v);void gpio_put(uint p,bool v);void gpio_set_function(uint p,uint v);
void sleep_ms(uint ms);void tight_loop_contents(void);uint64_t time_us_64(void);
uint spi_init(spi_inst_t *s,uint rate);void spi_deinit(spi_inst_t *s);uint spi_set_baudrate(spi_inst_t *s,uint rate);
void spi_set_format(spi_inst_t *s,uint bits,uint pol,uint pha,uint order);
spi_inst_t *spi_get_hw(spi_inst_t *s);bool spi_is_readable(spi_inst_t *s);bool spi_is_writable(spi_inst_t *s);bool spi_is_busy(spi_inst_t *s);uint spi_get_dreq(spi_inst_t *s,bool tx);
void dma_channel_abort(uint ch);int dma_claim_unused_channel(bool required);bool dma_channel_is_busy(uint ch);
dma_channel_config dma_channel_get_default_config(uint ch);
void channel_config_set_transfer_data_size(dma_channel_config *c,uint n);void channel_config_set_read_increment(dma_channel_config *c,bool b);void channel_config_set_write_increment(dma_channel_config *c,bool b);void channel_config_set_dreq(dma_channel_config *c,uint n);
void dma_channel_configure(uint ch,const dma_channel_config *c,volatile void *dest,const void *src,uint32_t count,bool start);
uint pwm_gpio_to_slice_num(uint p);pwm_config pwm_get_default_config(void);void pwm_config_set_wrap(pwm_config *c,uint16_t n);void pwm_config_set_clkdiv(pwm_config *c,float n);void pwm_init(uint s,const pwm_config *c,bool start);void pwm_set_gpio_level(uint p,uint16_t n);
extern bool fake_stuck,fake_spi_busy,fake_early_cs,fake_unbounded_abort;
#endif
