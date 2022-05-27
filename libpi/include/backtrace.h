#ifndef BACKTRACE_H
#define BACKTRACE_H

int backtrace();

char *backtrace_buf();

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
const char *name_of(uintptr_t fn_start_addr);


#endif