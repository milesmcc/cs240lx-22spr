#include "rpi.h"
#include "backtrace.h"
#include "rpi-interrupts.h"

/* Type: frame_t
 * -------------
 * This struct stores the information for a function who has a frame on
 * the current stack.
 *
 * The `name` field is the name of the function as found by `name_of`.
 *
 * The `resume_addr` field is taken from saved lr in the callee. The saved lr
 * (sometimes referred to as "return address") is the address of the
 * instruction in the caller's sequence of instructions where control will
 * resume after the callee returns.  The `uintptr_t` type is an unsigned integer
 * of the appropriate size to store an address. This type is used to hold
 * an address that you intend to treat numerically.
 *
 * The `resume_offset` is the number of bytes from the start of the function to
 * the `resume_addr`. This offset represents how far control has advanced into
 * the caller function before it invoked the callee.
 */
typedef struct {
    const char *name;
    unsigned resume_addr;
    int resume_offset;
} frame_t;

int backtrace() {
    unsigned *cur_fp;    
    __asm__("mov %0, fp" : "=r" (cur_fp));

    for (int i = 0; i < 32 && (cur_fp != 0); i++) {
        unsigned pc = *(cur_fp) - 12;
        printk("#%d %x (%s)\n", i, pc, name_of(pc));
        cur_fp = (unsigned *) *(cur_fp - 3);
        if (cur_fp == 0) {
            return i;
        }
    }

    return -1;
}

/*
 * `name_of`
 *
 * The argument to `name_of` is the numeric address in memory of the first
 * instruction of a function and `name_of` returns the name of that
 * function.
 *
 * When compiling with the `-mpoke-function-name` flag, each function's
 * name is written into the text section alongside its instructions.
 * `name_of` finds a function's name by looking in the appropriate
 * location relative to the function's start address. The `uintptr_t` type
 * is an unsigned integer of the appropriate size to store an address.
 * This type is used to hold an address that you intend to treat numerically.
 *
 * If no name is available for the given address, `name_of` returns
 * the constant string "???"
 *
 * @param fn_start_addr     numeric address of instruction in memory
 * @return                  pointer to string of function's name or
 *                          constant string "???" if name not available
 */
const char *name_of(unsigned fn_start_addr) {
    unsigned word = *((unsigned *)fn_start_addr - 1);
    if (word & 0xff000000) {
        unsigned name_len = (word & ~0xff000000);
        return (char *)fn_start_addr - name_len - 4;
    } else {
        return "???";
    }
}
