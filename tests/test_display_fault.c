#include "display.h"
#include "fake_pico.h"
#include <stdio.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c);return 1; } } while (0)
int main(void) {
    CHECK(display_init());
    static uint16_t pixel=0xffff;
    CHECK(display_start(120,120,1,1,&pixel,1,true));
    fake_stuck=true;fake_spi_busy=true;
    const uint64_t start=time_us_64();
    CHECK(!display_wait(NULL));
    CHECK(time_us_64()-start<600000u);
    CHECK(!fake_early_cs);
    CHECK(!fake_unbounded_abort);
    CHECK(display_errors()==1u);
    CHECK(!display_start(120,120,1,1,&pixel,1,true));
    puts("PASS: stalled DMA is bounded, SPI stops before CS, faulted source cannot be reused");
    return 0;
}
