// starter code for trivial heap checking using interrupts.
#include "rpi.h"
#include "rpi-internal.h"
#include "timer-interrupt.h"
#include "rpi-armtimer.h"
#include "ckalloc.h"

#include "asan.h"

typedef struct trampoline_t {
    unsigned push, bl, pop, inst, ret;
} trampoline_t;

// you'll need to pull your code from lab 2 here so you
// can fabricate jumps
// #include "armv6-insts.h"

// used to check initialization.
static volatile int init_p, check_on;

// allow them to limit checking to a range.  for simplicity we 
// only check a single contiguous range of code.  initialize to 
// the entire program.
static uint32_t 
    start_check = (uint32_t)&__code_start__, 
    end_check = (uint32_t)&__code_end__,
    // you will have to define these functions.
    start_nocheck = (uint32_t)ckalloc_start,
    end_nocheck = (uint32_t)ckalloc_end;

static int should_overwrite(uint32_t instruction) {
    return 0xe5000000 == (instruction & 0xff000000) && // Is a load or store
        ((0x000f0000 & instruction ) != 0x000f0000); // Does not involve pc
}

static inline uint32_t arm_b(uint32_t from, uint32_t to) {
    int32_t delta = (to - (from + 8)) / 4;
    return 0xea000000 | ((delta) & (0xffffff));
}

static inline uint32_t arm_bl(uint32_t from, uint32_t to) {
    int32_t delta = (to - (from + 8)) / 4;
    return 0xeb000000 | ((delta) & (0xffffff));
}

static int check_heap_wrapper() {
    // printk("[Call to check_heap_wrapper!]\n");
    return ck_heap_errors();
}

static void _trampoline_playground(void) {
    asm volatile("push {r0-r12, lr}");
    ck_heap_errors();
    asm volatile("pop {r0-r12, lr}");
    asm volatile("nop");
}

static void overwrite(uint32_t pc) {
    static trampoline_t *trampoline_vector = 0;
    if (trampoline_vector == 0) {
        trampoline_vector = kmalloc(100000);
    }

    trampoline_t *t = &trampoline_vector[pc];

    // Generate trampoline
    t->push = 0xe92d5fff;
    t->bl = arm_bl((uint32_t) &t->bl, (uint32_t) check_heap_wrapper);
    t->pop = 0xe8bd5fff;
    t->inst = *(uint32_t *)pc;
    t->ret = arm_b((uint32_t) &t->ret, pc + 4);

    // Overwrite original instruction
    *(uint32_t *)pc = arm_b(pc, (uint32_t) t);
}

static int in_range(uint32_t addr, uint32_t b, uint32_t e) {
    assert(b<e);
    return addr >= b && addr < e;
}

// if <pc> is in the range we want to check and not in the 
// range we cannot check, return 1.
int (ck_mem_checked_pc)(uint32_t pc) {
    return in_range(pc, start_check, end_check) && !(in_range(pc, start_nocheck, end_nocheck));
}

// useful variables to track: how many times we did 
// checks, how many times we skipped them b/c <ck_mem_checked_pc>
// returned 0 (skipped)
static volatile unsigned checked = 0, skipped = 0, errors = 0;

unsigned ck_mem_stats(int clear_stats_p) { 
    unsigned s = skipped, c = checked, n = s+c;
    printk("total interrupts = %d, checked instructions = %d, skipped = %d\n",
        n,c,s);
    if(clear_stats_p)
        skipped = checked = 0;
    return c;
}

// note: lr = the pc that we were interrupted at.
// longer term: pass in the entire register bank so we can figure
// out more general questions.
static inline void ck_mem_interrupt(uint32_t pc) {
    // we don't know what the user was doing.
    dev_barrier();

    unsigned pending = GET32(IRQ_basic_pending);
    if((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0)
        return;
    PUT32(arm_timer_IRQClear, 1);

    if (ck_mem_checked_pc(pc)) {
        if (should_overwrite(*(uint32_t *)pc)) {
            overwrite(pc);
            printk("***** Overwrote %p!\n", pc);
        }
        checked++;
    } else {
        skipped++;
    }

    // we don't know what the user was doing.
    dev_barrier();
}

void interrupt_vector(uint32_t pc) {
    ck_mem_interrupt(pc);
}

// do any interrupt init you need, etc.
void ck_mem_init(void) { 
    assert(!init_p);
    init_p = 1;

    assert(in_range((uint32_t)ckalloc, start_nocheck, end_nocheck));
    assert(in_range((uint32_t)ckfree, start_nocheck, end_nocheck));
    assert(!in_range((uint32_t)printk, start_nocheck, end_nocheck));

    int_init();
    timer_interrupt_init(100);
}

// only check pc addresses [start,end)
void ck_mem_set_range(void *start, void *end) {
    assert(start < end);

    start_check = (uint32_t) start;
    end_check = (uint32_t) end;
}

// maybe should always do the heap check at the begining
void ck_mem_on(void) {
    assert(init_p && !check_on);
    check_on = 1;

    system_enable_interrupts();
}

// maybe should always do the heap check at the end.
void ck_mem_off(void) {
    assert(init_p && check_on);

    check_on = 0;
    system_disable_interrupts();
}
