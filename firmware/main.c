#include "display.h"
#include "sensors.h"
#include "board.h"
#include "gyrus/diagnostic.h"
#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "pico/bootrom.h"
#include "pico/unique_id.h"
#include "hardware/clocks.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t frame[DIAG_WIDTH*DIAG_HEIGHT];
static sensor_info info;
static diag_touch touch;
static bool touch_valid;
static uint64_t next_touch_us;
static uint32_t touch_reads,touch_failures,touch_pressed_reads,touch_max_read_us;
static uint16_t min_x=239,min_y=239,max_x,max_y;
static unsigned current_pattern;
static bool streaming_touch;

static void service_touch(void) {
    const uint64_t now=time_us_64();
    if (now<next_touch_us) return;
    next_touch_us=now+10000u;
    if (!info.touch_ok) return;
    touch_valid=sensors_touch(&touch);
    const uint32_t duration=(uint32_t)(time_us_64()-now);
    if (duration>touch_max_read_us) touch_max_read_us=duration;
    ++touch_reads;
    if (!touch_valid) ++touch_failures;
    if (touch_valid && touch.pressed) {
        ++touch_pressed_reads;
        if (touch.x<min_x) min_x=touch.x;
        if (touch.x>max_x) max_x=touch.x;
        if (touch.y<min_y) min_y=touch.y;
        if (touch.y>max_y) max_y=touch.y;
    }
}

static void print_info(void) {
    char id[2*PICO_UNIQUE_BOARD_ID_SIZE_BYTES+1];
    pico_get_unique_board_id_string(id,sizeof id);
    printf("{\"event\":\"info\",\"firmware\":\"gyrus_board_diag\",\"version\":\"0.1.0\",\"id\":\"%s\",\"sys_hz\":%" PRIu32 ",\"peri_hz\":%" PRIu32 ",\"usb_hz\":%" PRIu32 ",\"spi_hz\":%" PRIu32 ",\"lcd_reset\":13,\"frame_bytes\":%u,\"touch_ok\":%s,\"touch_id\":%u,\"touch_fw\":%u,\"imu_ok\":%s,\"imu_address\":%u,\"imu_id\":%u}\n",
        id,clock_get_hz(clk_sys),clock_get_hz(clk_peri),clock_get_hz(clk_usb),display_baud(),(unsigned)sizeof frame,
        info.touch_ok?"true":"false",info.touch_id,info.touch_fw,info.imu_ok?"true":"false",info.imu_address,info.imu_id);
}

static void print_touch(void) {
    printf("{\"event\":\"touch\",\"valid\":%s,\"pressed\":%s,\"x\":%u,\"y\":%u,\"reads\":%" PRIu32 ",\"failures\":%" PRIu32 ",\"pressed_reads\":%" PRIu32 ",\"irq_count\":%u,\"max_read_us\":%" PRIu32 ",\"min_x\":%u,\"max_x\":%u,\"min_y\":%u,\"max_y\":%u,\"i2c_errors\":%u}\n",
        touch_valid?"true":"false",touch.pressed?"true":"false",touch.x,touch.y,touch_reads,touch_failures,touch_pressed_reads,
        sensors_irq_count(),touch_max_read_us,min_x,max_x,min_y,max_y,sensors_errors());
}

static void prepare_pattern(unsigned pattern) {
    for (uint16_t y=0;y<DIAG_HEIGHT;++y)
        for (uint16_t x=0;x<DIAG_WIDTH;++x) frame[(size_t)y*DIAG_WIDTH+x]=diag_pattern_pixel(pattern,x,y);
}

static bool send_frame(void) {
    return display_start(0,0,240,240,frame,DIAG_WIDTH*DIAG_HEIGHT,true) && display_wait(service_touch);
}

static void show_pattern(unsigned pattern) {
    current_pattern=pattern;
    prepare_pattern(pattern);
    const bool ok=send_frame();
    printf("{\"event\":\"pattern\",\"pattern\":%u,\"ok\":%s,\"spi_hz\":%" PRIu32 "}\n",pattern,ok?"true":"false",display_baud());
}

static void benchmark(void) {
    const uint32_t requests[]={20000000u,40000000u,62500000u};
    prepare_pattern(1);
    for (unsigned r=0;r<3u;++r) {
        const uint32_t actual=display_set_baud(requests[r]);
        for (unsigned mode=0;mode<2u;++mode) {
            uint64_t sum=0,min=UINT64_MAX,max=0,dma_sum=0;
            unsigned passed=0;
            const uint32_t reads_before=touch_reads;
            for (unsigned n=0;n<16u;++n) {
                service_touch();
                if (!display_start(0,0,240,240,frame,DIAG_WIDTH*DIAG_HEIGHT,mode==1u) || !display_wait(service_touch)) break;
                const uint64_t us=display_last_total_us();
                sum+=us;dma_sum+=display_last_dma_us();
                if (us<min) min=us;
                if (us>max) max=us;
                ++passed;
            }
            printf("{\"event\":\"bench\",\"mode\":\"%s\",\"request_hz\":%" PRIu32 ",\"actual_hz\":%" PRIu32 ",\"pixels\":57600,\"payload_bytes\":115200,\"samples\":%u,\"min_us\":%" PRIu64 ",\"mean_us\":%" PRIu64 ",\"max_us\":%" PRIu64 ",\"enqueue_mean_us\":%" PRIu64 ",\"touch_reads\":%" PRIu32 ",\"display_errors\":%u}\n",
                mode==1u?"dma16":"poll16",requests[r],actual,passed,passed?min:0u,passed?sum/passed:0u,max,passed?dma_sum/passed:0u,touch_reads-reads_before,display_errors());
            stdio_flush();
        }
    }
    (void)display_set_baud(40000000u);
    show_pattern(current_pattern);
    puts("{\"event\":\"bench_done\"}");
}

static void window_test(void) {
    static const uint16_t white=0xffff;
    const uint16_t points[][2]={{0,0},{239,0},{0,239},{239,239},{120,120}};
    bool ok=true;
    for (unsigned i=0;i<5u;++i)
        if (!display_start(points[i][0],points[i][1],1,1,&white,1,true) || !display_wait(service_touch)) ok=false;
    printf("{\"event\":\"windows\",\"ok\":%s,\"display_errors\":%u}\n",ok?"true":"false",display_errors());
}

static void dispatch(const char *line) {
    if (strcmp(line,"info")==0) print_info();
    else if (strcmp(line,"bench")==0) benchmark();
    else if (strcmp(line,"touch")==0) print_touch();
    else if (strcmp(line,"windows")==0) window_test();
    else if (strcmp(line,"stream on")==0) {streaming_touch=true;puts("{\"event\":\"stream\",\"enabled\":true}");}
    else if (strcmp(line,"stream off")==0) {streaming_touch=false;puts("{\"event\":\"stream\",\"enabled\":false}");}
    else if (strcmp(line,"pattern bars")==0) show_pattern(0);
    else if (strcmp(line,"pattern checker")==0) show_pattern(1);
    else if (strcmp(line,"pattern grid")==0) show_pattern(2);
    else if (strcmp(line,"speed slow")==0) { (void)display_set_baud(20000000u);show_pattern(current_pattern); }
    else if (strcmp(line,"speed medium")==0) { (void)display_set_baud(40000000u);show_pattern(current_pattern); }
    else if (strcmp(line,"speed fast")==0) { (void)display_set_baud(62500000u);show_pattern(current_pattern); }
    else if (strcmp(line,"boot")==0) {puts("{\"event\":\"boot\"}");stdio_flush();sleep_ms(100);reset_usb_boot(0,0);}
    else puts("{\"event\":\"error\",\"message\":\"unknown command\"}");
}

int main(void) {
    (void)set_sys_clock_khz(125000,true);
    /* set_sys_clock_khz intentionally parks clk_peri on PLL_USB (48 MHz). */
    const uint32_t sys_hz=clock_get_hz(clk_sys);
    (void)clock_configure(clk_peri,0,CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,sys_hz,sys_hz);
    stdio_init_all();
    const bool lcd_ok=display_init();
    info=sensors_init();
    if (lcd_ok) {prepare_pattern(0);(void)send_frame();}
    char line[64];size_t used=0;bool discard=false,was_connected=false;
    uint64_t next_report=0,next_cursor=0;
    for (;;) {
        const bool connected=stdio_usb_connected();
        if (connected && !was_connected) {
            print_info();printf("{\"event\":\"ready\",\"lcd_ok\":%s}\n",lcd_ok?"true":"false");
            used=0;discard=false;
        }
        was_connected=connected;
        int ch=getchar_timeout_us(0);
        while (ch!=PICO_ERROR_TIMEOUT) {
            if (ch=='\n' || ch=='\r') {
                if (discard) puts("{\"event\":\"error\",\"message\":\"command too long\"}");
                else if (used>0u) {line[used]='\0';dispatch(line);}
                used=0;discard=false;
            } else if (!discard) {
                if (used+1u<sizeof line) line[used++]=(char)ch;
                else discard=true;
            }
            ch=getchar_timeout_us(0);
        }
        service_touch();
        const uint64_t now=time_us_64();
        if (streaming_touch && connected && now>=next_report) {print_touch();next_report=now+100000u;}
        if (lcd_ok && touch_valid && touch.pressed && now>=next_cursor) {
            prepare_pattern(2);
            const int tx=touch.x,ty=touch.y;
            for (int dy=-5;dy<=5;++dy) for (int dx=-5;dx<=5;++dx) {
                const int x=tx+dx,y=ty+dy;
                if (x>=0 && y>=0 && x<240 && y<240) frame[(size_t)y*DIAG_WIDTH+(size_t)x]=0xf800;
            }
            (void)send_frame();next_cursor=now+33333u;
        }
        sleep_us(100);
    }
}
