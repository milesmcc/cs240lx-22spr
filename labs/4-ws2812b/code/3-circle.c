/*
 * simple test to use your buffered neopixel interface to push a cursor around
 * a light array.
 */
#include "rpi.h"
#include "WS2812B.h"
#include "neopixel.h"

// the pin used to control the light strip.
enum { pix_pin = 21 };

void notmain(void) {
    enable_cache(); 
    gpio_set_output(pix_pin);

    // make sure when you implement the neopixel 
    // interface works and pushes a pixel around your light
    // array.
    unsigned npixels = 16;  // you'll have to figure this out.
    neo_t h = neopix_init(pix_pin, npixels);

    // does 10 increasingly faster loops.
    for(int j = 0; j < 16; j++) {
        output("loop %d\n", j);
        for(int i = 0; i < npixels; i++) {
            neopix_write(h, i, 255, 0, 255);
            neopix_flush(h);
            delay_ms(100);
        }
    }
    output("done!\n");
}
