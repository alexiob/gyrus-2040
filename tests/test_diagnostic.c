#include "gyrus/diagnostic.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c); return 1; } } while (0)
int main(void) {
    uint8_t window[8] = {0};
    const uint8_t edge[8] = {0,239,0,239,0,239,0,239};
    const uint8_t full[8] = {0,0,0,239,0,0,0,239};
    CHECK(diag_window_encode(239,239,1,1,window));
    CHECK(memcmp(window,edge,8)==0);
    CHECK(diag_window_encode(0,0,240,240,window));
    CHECK(memcmp(window,full,8)==0);
    CHECK(!diag_window_encode(0,0,0,240,window));
    CHECK(!diag_window_encode(239,0,2,1,window));
    CHECK(!diag_window_encode(0,240,1,1,window));
    CHECK(!diag_window_encode(65535,0,2,1,window));
    CHECK(!diag_window_encode(0,0,1,1,NULL));
    diag_touch t = {true,9,9};
    const uint8_t down[5] = {1,0,120,0,239};
    const uint8_t up[5] = {0,0xff,0xff,0xff,0xff};
    const uint8_t invalid[5] = {1,0,240,0,0};
    const uint8_t multi[5] = {2,0,120,0,120};
    CHECK(diag_touch_decode(down,&t) && t.pressed && t.x==120 && t.y==239);
    CHECK(diag_touch_decode(up,&t) && !t.pressed && t.x==0 && t.y==0);
    CHECK(!diag_touch_decode(invalid,&t) && !t.pressed);
    CHECK(!diag_touch_decode(multi,&t) && !t.pressed);
    CHECK(!diag_touch_decode(NULL,&t));
    CHECK(!diag_touch_decode(down,NULL));
    CHECK(diag_pattern_pixel(0,39,100)==0xf800);
    CHECK(diag_pattern_pixel(0,40,100)==0x07e0);
    CHECK(diag_pattern_pixel(0,80,100)==0x001f);
    CHECK(diag_pattern_pixel(1,0,0)==0xffff);
    CHECK(diag_pattern_pixel(1,8,0)==0);
    CHECK(diag_pattern_pixel(1,8,8)==0xffff);
    CHECK(diag_pattern_pixel(0,240,0)==0);
    /* Catches embedded-NUL boot commands and proves rejection lasts to newline. */
    diag_line line={0};
    const uint8_t malformed[]={ 'b','o','o','t',0,'x','\n' };
    diag_line_result result=DIAG_LINE_NONE;
    for (size_t i=0;i<sizeof malformed;++i) result=diag_line_feed(&line,malformed[i]);
    CHECK(result==DIAG_LINE_INVALID);
    for (unsigned i=0;i<80u;++i) (void)diag_line_feed(&line,'a');
    CHECK(diag_line_feed(&line,'\n')==DIAG_LINE_LONG);
    const char *good="info\n";
    for (size_t i=0;i<5u;++i) result=diag_line_feed(&line,(uint8_t)good[i]);
    CHECK(result==DIAG_LINE_READY && strcmp(line.text,"info")==0);
    puts("PASS: window boundaries, coherent touch decoding, RGB565 patterns");
    return 0;
}
