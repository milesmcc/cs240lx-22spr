#include "rpi.h"

// cycle counter routines.
#include "cycle-count.h"

#pragma GCC optimize ("align-functions=32")

// this defines the period: makes it easy to keep track and share
// with the test generator.
#include "../test-gen/test-gen.h"

// trivial logging code to compute the error given a known
// period.
#include "samples.h"

// derive this experimentally: check that your pi is the same!!
#define CYCLE_PER_SEC (700*1000*1000)

// some utility routines: you don't need to use them.
#include "cycle.h"

#define GPIO_PIN 21

#define GPIO_BASE 0x20200000
static const unsigned gpio_lev0  = (GPIO_BASE + 0x34);
static volatile uint32_t *val = (uint32_t *)gpio_lev0;

static inline unsigned gpio_read_pin() {
    return (*val >> GPIO_PIN) & 1;
}

// implement this code and tune it.
unsigned scope(unsigned pin, log_ent_t *l, unsigned n_max, unsigned max_cycles) {
    unsigned v1, v0 = gpio_read_pin();
    unsigned i = 0;
    unsigned end_cyc_count = cycle_cnt_read() + max_cycles;

    // spin until the pin changes.
    while((v1 = gpio_read_pin()) == v0) {}
    v0 = v1;

    
    // when we started sampling 
    unsigned start = cycle_cnt_read(), t = start;

    // sample until record max samples or until exceed <max_cycles>
    unsigned n = 0;
    asm volatile(".align 5");
    while(n < n_max) {
        i++;

        v1 = gpio_read_pin();
        if(v1 != v0) {
            unsigned now = cycle_cnt_read();
            l[n].ncycles = now - start;
            l[n].v = v0;
            
            v0 = v1;
            t = now;
            n++;
        }

        // exit when we have run too long.
        if((i & 0xffff) == 0 && cycle_cnt_read() > end_cyc_count) {
            printk("timeout! start=%d, t=%d, minux=%d\n",
                                start,t,t-start);
            return n;
        }
    }
    return n;
}

void notmain(void) {
    enable_cache();

    // setup input pin.
    int pin = 21;
    gpio_set_input(pin);

    // make sure to init cycle counter hw.
    cycle_cnt_init();

#   define MAXSAMPLES 32
    log_ent_t log[MAXSAMPLES];

    // run 4 times before rebooting: makes things easier.
    // you can get rid of this.
    for(int i = 0; i < 10; i++) {
        unsigned n = scope(pin, log, MAXSAMPLES, sec_to_cycle(1));
        dump_samples(log, n, CYCLE_PER_FLIP);
    }
    clean_reboot();
}
