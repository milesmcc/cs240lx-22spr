// engler, cs140 put your gpio implementations in here.
#include "rpi.h"
#include "gpio.h"

#define GPIO_BASE 0x20200000
#define INT_BASE 0x2000B000

enum { GPREN0 = GPIO_BASE + 0x4C,
       GPREN1 = GPIO_BASE + 0x50,
       GPFEN0 = GPIO_BASE + 0x58,
       GPFEN1 = GPIO_BASE + 0x5C,
       GPHEN0 = GPIO_BASE + 0x64,
       GPHEN1 = GPIO_BASE + 0x48,
       GPEDS0 = GPIO_BASE + 0x40,
       GPEDS1 = GPIO_BASE + 0x44,
       INT_PEND1 = INT_BASE + 0x204,
       INT_PEND2 = INT_BASE + 0x208,
       INT_ENB_2 = INT_BASE + 0x214};

static void enable_gpio_interrupts() {
    // Must be called in a function with dev barriers
    dev_barrier();
    PUT32(INT_ENB_2, 1 << (GPIO_INT0 % 32));
}

int is_gpio_int(unsigned gpio_int) {
    dev_barrier();
    assert(gpio_int >= GPIO_INT0 && gpio_int <= GPIO_INT3);
    return (GET32(INT_PEND2) & (1 << (gpio_int % 32))) != 0;
    dev_barrier();
}


// p97 set to detect rising edge (0->1) on <pin>.
// as the broadcom doc states, it  detects by sampling based on the clock.
// it looks for "011" (low, hi, hi) to suppress noise.  i.e., its triggered only
// *after* a 1 reading has been sampled twice, so there will be delay.
// if you want lower latency, you should us async rising edge (p99)
void gpio_int_rising_edge(unsigned pin) {
    dev_barrier();
    unsigned reg = pin < 32 ? GPREN0 : GPREN1;
    PUT32(reg, GET32(reg) | (1 << (pin % 32)));
    enable_gpio_interrupts();
    dev_barrier();
}

// p98: detect falling edge (1->0).  sampled using the system clock.  
// similarly to rising edge detection, it suppresses noise by looking for
// "100" --- i.e., is triggered after two readings of "0" and so the 
// interrupt is delayed two clock cycles.   if you want  lower latency,
// you should use async falling edge. (p99)
void gpio_int_falling_edge(unsigned pin) {
    dev_barrier();
    unsigned reg = pin < 32 ? GPFEN0 : GPFEN1;
    PUT32(reg, GET32(reg) | (1 << (pin % 32)));
    enable_gpio_interrupts();
    dev_barrier();
}

// p96: a 1<<pin is set in EVENT_DETECT if <pin> triggered an interrupt.
// if you configure multiple events to lead to interrupts, you will have to 
// read the pin to determine which caused it.
int gpio_event_detected(unsigned pin) {
    dev_barrier();
    unsigned reg = pin < 32 ? GPEDS0 : GPEDS1;
    int ret = (GET32(reg) & (1 << (pin % 32))) != 0;
    dev_barrier();
    return ret;
}

// p96: have to write a 1 to the pin to clear the event.
void gpio_event_clear(unsigned pin) {
    dev_barrier();
    unsigned reg = pin < 32 ? GPEDS0 : GPEDS1;
    PUT32(reg, (1 << (pin % 32)));
    dev_barrier();
}
