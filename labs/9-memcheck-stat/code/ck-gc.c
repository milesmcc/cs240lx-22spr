/*************************************************************************
 * engler, cs240lx: Purify/Boehm style leak checker/gc starter code.
 *
 * We've made a bunch of simplifications.  Given lab constraints:
 * people talk about trading space for time or vs. but the real trick
 * is how to trade space and time for less IQ needed to see something
 * is correct. (I haven't really done a good job of that here, but
 * it's a goal.)
 *
 */
#include "rpi.h"
#include "rpi-constants.h"
#include "ckalloc.h"
#include "kr-malloc.h"

static void *heap_start;
static void *heap_end;

int ck_heap_errors(void) {
    // Frankensteined with Igor
    int n_errors = 0, n_blocks = 0;

    for (hdr_t *h = ck_first_hdr(); h; h = ck_next_hdr(h), n_blocks++) {
        for (int i = 0; i < REDZONE_NBYTES; i++) {
            if (h->rz1[i] != REDZONE_VAL) {
                // ck_error(h, "block %d corrupted at offset %d\n", h->block_id, (uint32_t) &h->rz1[i] - (uint32_t) ck_hdr_start(h));
                n_errors++;
            }
        }

        uint8_t* rz2 = ck_hdr_end(h);
        for (int i = 0; i < REDZONE_NBYTES; i++) {
            if (rz2[i] != REDZONE_VAL) {
                // ck_error(h, "block %d corrupted at offset %d\n", h->block_id, (uint32_t) &rz2[i] - (uint32_t) ck_hdr_start(h));
                n_errors++;
            }
        }

        if (h->state == FREED) {
            for (uint8_t* p = ck_hdr_start(h); p < (uint8_t*) ck_hdr_end(h); p++) {
                if (*p != REDZONE_VAL) {
                    // ck_error(h, "block %d corrupted at offset %d\n", h->block_id, (uint32_t) p - (uint32_t) ck_hdr_start(h));
                    // trace("  Wrote block after free!\n");
                    n_errors++;
                }
            }
        }
    }

	// trace("checked %d blocks, detected %d errors\n", n_blocks, n_errors);

    return n_errors;
}

void *sbrk(long increment)
{
    static int init_p;

    assert(increment > 0);
    if (init_p)
        panic("not handling\n");
    else
    {
        unsigned onemb = 0x100000;
        heap_start = (void *)onemb;
        heap_end = (char *)heap_start + onemb;
        kmalloc_init_set_start(onemb, onemb);
        init_p = 1;
    }
    return kmalloc(increment);
}

// quick check that the pointer is between the start of
// the heap and the last allocated heap pointer.  saves us
// walk through all heap blocks.
//
// we could warn if the pointer is within some amount of slop
// so that you can detect some simple overruns?
static int in_heap(void *p)
{
    // should be the last allocated byte(?)
    if (p < heap_start || p >= heap_end)
        return 0;
    // output("ptr %p is in heap!\n", p);
    return 1;
}

// given potential address <addr>, returns:
//  - 0 if <addr> does not correspond to an address range of
//    any allocated block.
//  - the associated header otherwise (even if freed: caller should
//    check and decide what to do in that case).
//
// XXX: you'd want to abstract this some so that you can use it with
// other allocators.  our leak/gc isn't really allocator specific.
static hdr_t *is_ptr(uint32_t addr)
{
    void *p = (void *)addr;

    if (!in_heap(p))
        return 0;
    return ck_ptr_is_alloced(p);
}

// mark phase:
//  - iterate over the words in the range [p,e], marking any block
//    potentially referenced.
//  - if we mark a block for the first time, recurse over its memory
//    as well.
//
// NOTE: if we have lots of words, could be faster with shadow memory / a lookup
// table.  however, given our small sizes, this stupid search may well be faster :)
// If you switch: measure speedup!
//
#include "libc/helper-macros.h"
static void mark(uint32_t *p, uint32_t *e)
{
    assert(p < e);

    // maybe keep this same thing?
    assert(aligned(p, 4));
    assert(aligned(e, 4));

    for (uint32_t *i = p; i <= e; i++)
    {
        hdr_t *b = is_ptr(*i);
        if (b)
        {
            uint32_t exp = (uint32_t)(b + 1);
            uint16_t old_mark = b->mark;
            // output("going to mark %p (i @ %p = %p), exp = %p\n", ck_hdr_start(b), i, *i, exp);
            if (b != 0 && old_mark < 2)
            {
                b->mark = (*i == exp) ? 2 : 1; // 2 -> no error, 1 -> maybe error
                // output("marking as %d\n", (*i == exp) ? 2 : 1);
                // output("going to mark %p\n", b);

                if (old_mark == 0)
                {
                    mark((uint32_t *)b + 1, (uint32_t *)((char *)(b + 1) + b->nbytes_alloc));
                }
            }
            else
            {
                // output("not marking!\n");
            }
        }
    }
}

// write the assembly to dump all registers.
// need this to be in a seperate assembly file since gcc
// seems to be too smart for its own good.
void dump_regs(uint32_t *v, ...);

// a very slow leak checker.
static void mark_all(void)
{

    // slow: should not need this: remove after your code
    // works.
    for (hdr_t *h = ck_first_hdr(); h; h = ck_next_hdr(h))
    {
        h->mark = h->refs_start = h->refs_middle = 0;
    }
    // pointers can be on the stack, in registers, or in the heap itself.

    // get all the registers.
    uint32_t regs[16];
    dump_regs(regs);
    // kill caller-saved registers
    regs[0] = 0;
    regs[1] = 0;
    regs[2] = 0;
    regs[3] = 0;
    // for(int i = 4; i < 16; i++) {
    //     output("r%d = %x\n", i, regs[i]);
    // }

    // unimplemented();
    // mark(regs, &regs[14]);
    mark(regs, &regs[14]);

    // mark the stack: we are assuming only a single
    // stack.  note: stack grows down.
    uint32_t *stack_top = (void *)STACK_ADDR;
    output("checking stack: %p to %p\n", regs[13], stack_top);
    mark((uint32_t *)regs[13], stack_top);

    // these symbols are defined in our memmap
    extern uint32_t __bss_start__, __bss_end__;
    mark(&__bss_start__, &__bss_end__);

    extern uint32_t __data_start__, __data_end__;
    mark(&__data_start__, &__data_end__);
}

// do a sweep, warning about any leaks.
//
//
static unsigned sweep_leak(int warn_no_start_ref_p)
{
    unsigned nblocks = 0, errors = 0, maybe_errors = 0;
    output("---------------------------------------------------------\n");
    output("checking for leaks:\n");

    mark_all();
    for (hdr_t *h = ck_first_hdr(); h; h = ck_next_hdr(h))
    {
        if (h->state != ALLOCED) {
            continue;
        }

        nblocks++;
        if (h->mark == 0)
        {
            errors++;
            output("GC:LOST %d -> %p\n", nblocks, h);
        }
        else if (h->mark == 1)
        {
            maybe_errors++;
            output("GC:MAYBE LOST %d -> %p\n", nblocks, h);
        }
    }

    trace("\tGC:Checked %d blocks.\n", nblocks);
    if (!errors && !maybe_errors)
        trace("\t\tGC:SUCCESS: No leaks found!\n");
    else
        trace("\t\tGC:ERRORS: %d errors, %d maybe_errors\n",
              errors, maybe_errors);
    output("----------------------------------------------------------\n");
    if (warn_no_start_ref_p)
    {
        return errors + maybe_errors;
    }
    else
    {
        return errors;
    }
}

// return number of bytes allocated?  freed?  leaked?
// how do we check people?
unsigned ck_find_leaks(int warn_no_start_ref_p)
{
    // mark_all();
    return sweep_leak(warn_no_start_ref_p);
}

// used for tests.  just keep it here.
void check_no_leak(void)
{
    // when in the original tests, it seemed gcc was messing
    // around with these checks since it didn't see that
    // the pointer could escape.
    gcc_mb();
    if (ck_find_leaks(1))
        panic("GC: should have no leaks!\n");
    else
        trace("GC: SUCCESS: no leaks!\n");
    gcc_mb();
}

// used for tests.  just keep it here.
unsigned check_should_leak(void)
{
    // when in the original tests, it seemed gcc was messing
    // around with these checks since it didn't see that
    // the pointer could escape.
    gcc_mb();
    unsigned nleaks = ck_find_leaks(1);
    if (!nleaks)
        panic("GC: should have leaks!\n");
    else
        trace("GC: SUCCESS: found %d leaks!\n", nleaks);
    gcc_mb();
    return nleaks;
}

// similar to sweep_leak: but mark unreferenced blocks as FREED.
static unsigned sweep_free(void)
{
    unsigned nblocks = 0, nfreed = 0, nbytes_freed = 0;
    output("---------------------------------------------------------\n");
    output("compacting:\n");

    mark_all();
    for (hdr_t *h = ck_first_hdr(); h; h = ck_next_hdr(h))
    {
        nblocks++;
        if (h->mark == 0 && h->state == ALLOCED)
        {
            nfreed++;
            nbytes_freed += h->nbytes_alloc;
            ckfree(h + 1);
            output("GC:FREED %p\n", h + 1);
        }
    }

    trace("\tGC:Checked %d blocks, freed %d, %d bytes\n", nblocks, nfreed, nbytes_freed);

    return nbytes_freed;
}

unsigned ck_gc(void)
{
    mark_all();
    unsigned nbytes = sweep_free();

    // perhaps coalesce these and give back to heap.  will have to modify last.

    return nbytes;
}

void implement_this(void)
{
    panic("did not implement dump_regs!\n");
}
