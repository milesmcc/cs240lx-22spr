/*
   simple driver for a counterfeit texas inst ads1115 adc 
   datasheet is in docs.

   the quickstart guide on p35 describes how the bytes go 
   back and forth:
        - write high byte first.
        - read returns high byte first.

   given the lab setting, we jam everything into one file --- 
   after your code works you should pull the ads1115 code into
   its own file so you can add it easily to other projects.
 */
#include "rpi.h"
#include "ads1115.h"
#include "cycle-count.h"

void notmain(void) {
    unsigned dev_addr = ads1115_config();
    cycle_cnt_init();

    // 5. just loop and get readings.
    //  - vary your potentiometer
    //  - should get negative readings for low
    //  - around 20k for high.
    //
    // 
    // make sure: given we set gain to +/- 4v.
    // does the result make sense?
	for(int i = 0; i < 10000; i++) {
        short v = ads1115_read16(dev_addr, conversion_reg);
        
        unsigned s = cycle_cnt_read();
        if (!ads1115_wait_for_data(100000)) {
            printk("No data received in time!\n");
            break;
        }
        unsigned e = cycle_cnt_read();

        printk("%d\t", e - s);

        for(int i = 0; i < v; i += 200) {
            printk("#");
        }
        
        printk("\n");
	}

	clean_reboot();
}

