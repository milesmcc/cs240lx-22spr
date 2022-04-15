/*
 * simple test to use your buffered neopixel interface to push a cursor around
 * a light array.
 */
#include "rpi.h"
#include "WS2812B.h"
#include "neopixel.h"
#include "rpi-armtimer.h"

// the pin used to control the light strip.
enum
{
    pix_pin = 19,
    sonar_trig = 26,
    sonar_echo = 20,
};

int read_distance(void) {
    // 1. Send an echo
    gpio_set_on_raw(sonar_trig);
    delay_us(100);
    gpio_set_off_raw(sonar_trig);

    // 2. Wait
    const unsigned wait_timeout = 10000000;
    unsigned start = cycle_cnt_read();

    unsigned c = start;
    while(!gpio_read(sonar_echo)) {
        if(((c = cycle_cnt_read()) - start) >= (wait_timeout)) {
            return -1;
        }
    }

    start = cycle_cnt_read();
    unsigned d = start;
    while(gpio_read(sonar_echo)) {
        if(((d = cycle_cnt_read()) - start) >= (wait_timeout)) {
            return -1;
        }
    }
    unsigned end = cycle_cnt_read();

    return (end - start) / (10 * 7000);
}

void notmain(void)
{
    enable_cache();
    gpio_set_output(pix_pin);
    gpio_set_output(sonar_trig);
    gpio_set_input(sonar_echo);


    // make sure when you implement the neopixel
    // interface works and pushes a pixel around your light
    // array.
    unsigned npixels = 48; // you'll have to figure this out.
    neo_t h = neopix_init(pix_pin, npixels);

    // does 10 increasingly faster loops.
    int i = 0;
    for (int r = 0; r < 256; r += 5)
    {
        for (int g = 0; g < 256; g += 5)
        {
            for (int b = 0; b < 256; b += 5 )
            {
                int dist = read_distance();
                printk("Distance: %d\n", dist);
                if (dist > 0) {
                    for(int g = 0; g < dist / 2; g++) {
                        neopix_write(h, g, 255 - g, 0, g);
                    }
                    neopix_flush(h);
                    delay_ms(25);
                } else {
                    neopix_clear(h);
                    neopix_flush(h);
                    delay_ms(100);
                }
                i++;
            }
        }
    }
    output("done!\n");
}
