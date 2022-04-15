/*
 * simple test to use your buffered neopixel interface to push a cursor around
 * a light array.
 */
#include "rpi.h"
#include "WS2812B.h"
#include "neopixel.h"

// the pin used to control the light strip.
enum
{
    pix_pin = 21
};

void notmain(void)
{
    enable_cache();
    gpio_set_output(pix_pin);

    // make sure when you implement the neopixel
    // interface works and pushes a pixel around your light
    // array.
    unsigned npixels = 16; // you'll have to figure this out.
    neo_t h = neopix_init(pix_pin, npixels);

    // does 10 increasingly faster loops.
    int i = 0;
    for (int r = 0; r < 256; r += 5)
    {
        for (int g = 0; g < 256; g += 5)
        {
            for (int b = 0; b < 256; b += 5 )
            {
                    neopix_write(h, i % 16, r, g, b);
                    neopix_flush(h);
                    i++;
                    delay_ms(100);
            }
        }
    }
    output("done!\n");
}
