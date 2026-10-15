#include "sensors.h"
#include "board.h"
#include "pico/stdlib.h"
#include <stddef.h>
static volatile unsigned irq_count;
static unsigned errors;
static void touch_irq(uint gpio,uint32_t events) {(void)gpio;(void)events;++irq_count;}
static bool read_regs(uint8_t addr,uint8_t reg,uint8_t *data,size_t size) {
    int a=i2c_write_timeout_us(BOARD_I2C,addr,&reg,1,true,2000);
    int b=a==1 ? i2c_read_timeout_us(BOARD_I2C,addr,data,size,false,2000) : -1;
    if (b!=(int)size) {++errors;return false;}return true;
}
static bool write_reg(uint8_t reg,uint8_t value) {
    const uint8_t data[2]={reg,value};
    if (i2c_write_timeout_us(BOARD_I2C,0x15,data,2,false,2000)!=2) {++errors;return false;}
    return true;
}
sensor_info sensors_init(void) {
    sensor_info info={0};
    i2c_init(BOARD_I2C,400000);
    gpio_set_function(BOARD_SDA,GPIO_FUNC_I2C);gpio_set_function(BOARD_SCL,GPIO_FUNC_I2C);
    gpio_pull_up(BOARD_SDA);gpio_pull_up(BOARD_SCL);
    gpio_init(BOARD_TOUCH_RESET);gpio_set_dir(BOARD_TOUCH_RESET,GPIO_OUT);
    gpio_put(BOARD_TOUCH_RESET,0);sleep_ms(20);gpio_put(BOARD_TOUCH_RESET,1);sleep_ms(100);
    uint8_t id=0;
    if (read_regs(0x15,0xa7,&id,1) && id!=0u && id!=255u) {
        info.touch_id=id;
        info.touch_ok=read_regs(0x15,0xa9,&info.touch_fw,1) && write_reg(0xfe,1) && write_reg(0xfa,0x60);
    }
    const uint8_t addresses[2]={0x6b,0x6a};
    for (unsigned i=0;i<2u;++i) {
        if (read_regs(addresses[i],0,&id,1) && id==5u) {
            info.imu_ok=true;info.imu_address=addresses[i];info.imu_id=id;break;
        }
    }
    gpio_init(BOARD_TOUCH_IRQ);gpio_set_dir(BOARD_TOUCH_IRQ,GPIO_IN);gpio_pull_up(BOARD_TOUCH_IRQ);
    gpio_set_irq_enabled_with_callback(BOARD_TOUCH_IRQ,GPIO_IRQ_EDGE_FALL,true,touch_irq);
    return info;
}
bool sensors_touch(diag_touch *out) {
    uint8_t bytes[5];
    if (out==NULL) return false;
    *out=(diag_touch){0};
    return read_regs(0x15,2,bytes,sizeof bytes) && diag_touch_decode(bytes,out);
}
unsigned sensors_irq_count(void) {return irq_count;}
unsigned sensors_errors(void) {return errors;}
