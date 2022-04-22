#include "rpi.h"
#include "cycle-count.h"

static unsigned cycle_diff(void) {
    static unsigned last;

    unsigned diff = 0;

    unsigned cur = cycle_cnt_read();
    if(last) 
        diff = cur - last;
    
    last = cur;
    return diff;
}

void _cstart() {
    extern int __bss_start__, __bss_end__;
	void notmain();

    custom_loader();

    int* bss = &__bss_start__;
    int* bss_end = &__bss_end__;
 
    while( bss < bss_end )
        *bss++ = 0;

    uart_init();

    // have to initialize the cycle counter or it returns 0.
    // i don't think any downside to doing here.
    cycle_cnt_init();

    cycle_diff();
    notmain(); 
    unsigned diff = cycle_diff();
    printk("[Running took %d cycles]\n", diff);

	clean_reboot();
}


