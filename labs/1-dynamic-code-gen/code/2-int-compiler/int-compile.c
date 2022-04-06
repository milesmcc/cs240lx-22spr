#include "rpi.h"
#include <stdarg.h>
#include "../unix-side/armv6-insts.h"

#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
#include "cycle-util.h"

typedef void (*int_fp)(void);
#define THUNK_OFFSET 64

static volatile unsigned cnt = 0;

// fake little "interrupt" handlers: useful just for measurement.
void int_0() { cnt++; }
void int_1() { cnt++; }
void int_2() { cnt++; }
void int_3() { cnt++; }
void int_4() { cnt++; }
void int_5() { cnt++; }
void int_6() { cnt++; }
void int_7() { cnt++; }

void say_hello(unsigned a, unsigned b, unsigned c) {
    printk("a = %x, b = %x, c = %x\n", a, b, c);
}

void generic_call_int(int_fp *intv, unsigned n) { 
    for(unsigned i = 0; i < n; i++)
        intv[i]();
}

void *int_compile(int_fp *intv, unsigned n) {
    uint32_t *code = kmalloc(256*4);

    assert(n < 250);
    code[0] = arm_push(arm_lr);
    for(int i = 0; i < n - 1; i++) {
        code[i+1] = arm_bl((uint32_t) &code[i+1], (uint32_t) intv[i]);
    }
    code[n] = arm_pop(arm_lr);
    code[n+1] = arm_b((uint32_t) &code[n + 1], (uint32_t) intv[n - 1]);

    return code;
}

int_fp thunk(void *func, int n, ...) {
    unsigned *code = kmalloc(1024);
    assert(n < 12);

    va_list args;
    va_start(args, n);
    unsigned loc = 0;

    unsigned arg_vals[12];
    for (int i = 0; i < n; i++) {
        arg_vals[i] = va_arg(args, uint32_t);
    }

    for (int i = 0; i < n; i++) {
        if (i < 4) {
            code[loc++] = arm_ldr(i, arm_pc, (THUNK_OFFSET - 2) * 4);
            code[loc - 1 + THUNK_OFFSET] = arg_vals[i];
        } else {
            int effective_arg = (n - i + 3);
            code[loc++] = arm_ldr(arm_r12, arm_pc, (THUNK_OFFSET - 2) * 4);
            code[loc - 1 + THUNK_OFFSET] = arg_vals[effective_arg];
            code[loc++] = arm_push(arm_r12);
        }
    }

    code[loc] = arm_b((uint32_t) &code[loc], (uint32_t) func);
    return (int_fp) code;
}

// you will generate this dynamically.
void specialized_call_int(void) {
    int_0();
    int_1();
    int_2();
    int_3();
    int_4();
    int_5();
    int_6();
    int_7();
}

void notmain(void) {
    int_fp intv[] = {
        int_0,
        int_1,
        int_2,
        int_3,
        int_4,
        int_5,
        int_6,
        int_7
    };

    cycle_cnt_init();

    unsigned n = NELEM(intv);

    // try with and without cache: but if you modify the routines to do 
    // jump-threadig, must either:
    //  1. generate code when cache is off.
    //  2. invalidate cache before use.
    enable_cache();

    cnt = 0;
    TIME_CYC_PRINT10("cost of generic-int calling",  generic_call_int(intv,n));
    demand(cnt == n*10, "cnt=%d, expected=%d\n", cnt, n*10);

    // rewrite to generate specialized caller dynamically.

    // uint32_t bxlr = arm_bx(arm_lr);
    // for(int i = 0; i < n - 1; i++) { // Don't rewrite last function
    //     uint32_t next = (uint32_t) intv[i+1];
    //     uint32_t *func = (uint32_t *) intv[i];
        
    //     int idx = 0;
    //     for(int idx = 0; func[idx] != bxlr; idx++) {
    //         printk("not %d...\n", idx);
    //         // nop
    //     }

    //     func[idx] = arm_b((uint32_t) &func[idx], next);
    //     printk("Rewriting %x to call %x (%x)", &func[idx], next, func[idx]);
    // }

    cnt = 0;
    TIME_CYC_PRINT10("cost of specialized int calling", specialized_call_int());
    demand(cnt == n*10, "cnt=%d, expected=%d\n", cnt, n*10);

    cnt = 0;
    void *code = int_compile(intv, n);
    void (*fp)(void) = (typeof(fp))code;

    TIME_CYC_PRINT10("cost of int compile calling", fp());
    demand(cnt == n*10, "cnt=%d, expected=%d\n", cnt, n*10);

    int_fp thunked = thunk(say_hello, 3, 5, 500, 3);
    thunked();

    int_fp gooo = thunk(printk, 7, "gooooooo, 1=%d, 2=%d, 3=%d, 4=%d, 5=%d, 6=%d\n", 1, 2, 3, 4, 5, 6);
    gooo();

    printk("done!\n");

    clean_reboot();
}
