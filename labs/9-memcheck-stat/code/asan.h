#include "ckalloc.h"

extern char __heap_start__;

#define CHECK(write) uint32_t lr; asm volatile ("mov %0, lr" : "=r" (lr)); asan_access(addr, lr, write);

static int __in_range(uint32_t addr, void *b, void *e) {
    assert(b<e);
    return addr >= (uint32_t)b && addr < (uint32_t)e;
}

void asan_access(unsigned long addr, unsigned pc, char write) {
    if (ck_mem_checked_pc(pc)) {
        if (__in_range(addr, &__code_start__, &__code_end__) && write) {
            printk("SIMULATED PANIC: Trying to overwrite code at %p (addr = %p)!\n", pc, addr);
            return;
        }

        if (!(addr > (unsigned) &__heap_start__ && addr < (unsigned) (&__heap_start__ + SHADOW_OFFSET))) {
            // printk("not in heap~!\n");
            return;
        }

        hdr_t *h = ck_ptr_is_alloced((void *)addr);
        if (h) {
            if(h->state == FREED) {
                printk("SIMULATED PANIC: Trying to read freed address at %p (addr = %p)!\n", pc, addr);
            }
        } else {
            printk("SIMULATED PANIC: Trying to access non-allocated heap address at %p (addr = %p)!\n", pc, addr);
        }
    } else {
        // printk("[not a checked PC: %p]\n", pc);
    }
}

void __asan_load1_noabort(unsigned long addr) {
    CHECK(0)
}

void __asan_load2_noabort(unsigned long addr) {
    CHECK(0)
}

void __asan_load4_noabort(unsigned long addr) {
    CHECK(0)
}

void __asan_load8_noabort(unsigned long addr) {
    CHECK(0)
}

void __asan_loadN_noabort(unsigned long addr, size_t sz) {
    CHECK(0)
}

void __asan_store1_noabort(unsigned long addr) {
    CHECK(1)
}

void __asan_store2_noabort(unsigned long addr) {
    CHECK(1)
}

void __asan_store4_noabort(unsigned long addr) {
    CHECK(1)
}

void __asan_store8_noabort(unsigned long addr) {
    CHECK(1)
}

void __asan_storeN_noabort(unsigned long addr, size_t sz) {
    CHECK(1)
}

void __asan_handle_no_return() {}
void __asan_before_dynamic_init(const char* module_name) {}
void __asan_after_dynamic_init() {}