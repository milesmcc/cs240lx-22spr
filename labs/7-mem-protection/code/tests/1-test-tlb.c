#include "rpi.h"
#include "pinned-vm.h"
#include "assert.h"

void notmain(void) { 
    // map the heap: for lab cksums must be at 0x100000.
    kmalloc_init_set_start(MB, MB);

    // turn on the pinned MMU: identity map.
    procmap_t p = procmap_default_mk(kern_dom);
    pin_mmu_on(&p);

    // if we got here MMU must be working b/c we have accessed GPIO/UART
    output("hello: mmu must be on\n");

    uint32_t result = 0;
    demand(tlb_contains_va(&result, p.map[2].addr + 0xFAA0), "did not contai valid VA");
    demand(result == p.map[2].addr + 0xFAA0, "VA address didn't match!");

    demand(!tlb_contains_va(&result, 0xDEADFAA0), "contained invalid VA");

    output("SUCCESS!\n");
}
