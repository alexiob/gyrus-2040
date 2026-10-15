#include "fake_pico.h"
spi_inst_t fake_spi;
dma_hw_t fake_dma;
bool fake_stuck,fake_spi_busy,fake_early_cs,fake_unbounded_abort;
static uint64_t now;
void gpio_init(uint p){(void)p;}void gpio_set_dir(uint p,bool v){(void)p;(void)v;}
void gpio_put(uint p,bool v){if(p==9u && v && fake_spi_busy)fake_early_cs=true;}
void gpio_set_function(uint p,uint v){(void)p;(void)v;}
void sleep_ms(uint ms){now+=(uint64_t)ms*1000u;}void tight_loop_contents(void){}
uint64_t time_us_64(void){now+=1000u;return now;}
uint spi_init(spi_inst_t *s,uint r){(void)s;return r;}void spi_deinit(spi_inst_t *s){(void)s;fake_spi_busy=false;}
uint spi_set_baudrate(spi_inst_t *s,uint r){(void)s;return r;}
void spi_set_format(spi_inst_t *s,uint b,uint p,uint h,uint o){(void)s;(void)b;(void)p;(void)h;(void)o;}
spi_inst_t *spi_get_hw(spi_inst_t *s){return s;}bool spi_is_readable(spi_inst_t *s){(void)s;return false;}
bool spi_is_writable(spi_inst_t *s){(void)s;return true;}bool spi_is_busy(spi_inst_t *s){(void)s;return fake_spi_busy;}
uint spi_get_dreq(spi_inst_t *s,bool tx){(void)s;(void)tx;return 0;}
void dma_channel_abort(uint ch){(void)ch;fake_unbounded_abort=true;}
int dma_claim_unused_channel(bool r){(void)r;return 0;}bool dma_channel_is_busy(uint ch){(void)ch;return fake_stuck;}
dma_channel_config dma_channel_get_default_config(uint ch){(void)ch;return (dma_channel_config){0};}
void channel_config_set_transfer_data_size(dma_channel_config *c,uint n){(void)c;(void)n;}
void channel_config_set_read_increment(dma_channel_config *c,bool b){(void)c;(void)b;}
void channel_config_set_write_increment(dma_channel_config *c,bool b){(void)c;(void)b;}
void channel_config_set_dreq(dma_channel_config *c,uint n){(void)c;(void)n;}
void dma_channel_configure(uint ch,const dma_channel_config *c,volatile void *d,const void *s,uint32_t n,bool start){(void)ch;(void)c;(void)d;(void)s;(void)n;(void)start;}
uint pwm_gpio_to_slice_num(uint p){(void)p;return 0;}pwm_config pwm_get_default_config(void){return (pwm_config){0};}
void pwm_config_set_wrap(pwm_config *c,uint16_t n){(void)c;(void)n;}void pwm_config_set_clkdiv(pwm_config *c,float n){(void)c;(void)n;}
void pwm_init(uint s,const pwm_config *c,bool b){(void)s;(void)c;(void)b;}void pwm_set_gpio_level(uint p,uint16_t n){(void)p;(void)n;}
