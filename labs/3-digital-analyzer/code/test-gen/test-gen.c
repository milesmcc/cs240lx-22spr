// simple example test generator: a lot of error.  you should reduce it.
#include "rpi.h"
#include "test-gen.h"
#include "cycle-count.h"
#include "cycle-util.h"

#define N_ITERS 10

#define GPIO_BASE 0x20200000
// static volatile unsigned *gpio_fsel0 = (volatile unsigned *)(GPIO_BASE + 0x00);

static volatile unsigned *gpio_set0  = (unsigned *)(GPIO_BASE + 0x1C);
static volatile unsigned *gpio_clr0  = (unsigned *)(GPIO_BASE + 0x28);

// send N samples at <ncycle> cycles each in a simple way.
// a bunch of error sources here.
void test_gen(unsigned pin, unsigned ncycle) {
    unsigned ndelay = 0;
    unsigned start = cycle_cnt_read();

    unsigned v = 1;
    unsigned to_write = 1 << pin;
    
    #pragma GCC unroll 5
    for(unsigned i = 0; i < (N_ITERS / 2); i++) {
        *gpio_set0 = to_write;
        ndelay += ncycle;
        // we are not sure: are we delaying too much or too little?
        delay_ncycles(start, ndelay);

        *gpio_clr0 = to_write;
        ndelay += ncycle;
        // we are not sure: are we delaying too much or too little?
        delay_ncycles(start, ndelay);
    }
    unsigned end = cycle_cnt_read();
    printk("expected %d cycles, have %d, v=%d\n", ncycle*N_ITERS, end-start,v);
}

void notmain(void) {
    enable_cache();
    int pin = 21;
    gpio_set_output(pin);
    cycle_cnt_init();

    // keep it seperate so easy to look at assembly.
    while (1) {
        test_gen(pin, CYCLE_PER_FLIP);
        delay_ms(1000);
    }

    clean_reboot();
}
