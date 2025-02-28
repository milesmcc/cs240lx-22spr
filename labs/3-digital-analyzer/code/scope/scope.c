#include "rpi.h"

// cycle counter routines.
#include "cycle-count.h"

#pragma GCC optimize ("align-functions=32")
#define MAXSAMPLES 32

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
static const unsigned PIN_MASK = 1 << GPIO_PIN;

static inline unsigned gpio_read_pin() {
    return *val;
}

// implement this code and tune it.
unsigned scope(unsigned pin, log_ent_t *l, unsigned n_max, unsigned max_cycles) {
    unsigned n = 0;
    unsigned now = cycle_cnt_read();
    unsigned end_cyc_count = now + max_cycles;
    unsigned v1, v0 = gpio_read_pin();

    // spin until the pin changes.
    while(!(((v1 = gpio_read_pin()) ^ v0) & PIN_MASK)) {}

    // when we started sampling 
    unsigned start = cycle_cnt_read();
    v0 = v1;
    
    // sample until record max samples or until exceed <max_cycles>
    asm volatile(".align 5");
    #pragma GCC unroll 64
    while(n < MAXSAMPLES) {
        asm volatile("mrc 15, 0, %0, cr15, cr12, 1" : "=r" (now));
        v1 = gpio_read_pin();
        
        if((v1 ^ v0) & PIN_MASK) {
            l[n].ncycles = now - start;
            l[n].v = !(v1 & PIN_MASK);
            n++;
            v0 = v1;
        }

        // exit when we have run too long.
        if(now >= end_cyc_count) {
            printk("timeout!\n");
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

    log_ent_t log[MAXSAMPLES];

    // run 4 times before rebooting: makes things easier.
    // you can get rid of this.
    for(int i = 0; i < 4; i++) {
        unsigned n = scope(pin, log, MAXSAMPLES, sec_to_cycle(1));
        dump_samples(log, n, CYCLE_PER_FLIP);
    }
    clean_reboot();
}
