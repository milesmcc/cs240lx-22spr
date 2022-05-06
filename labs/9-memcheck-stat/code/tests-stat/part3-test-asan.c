#include "rpi.h"
#include "ckalloc.h"

static volatile int g;
static volatile int *h;

void __start_pos() {}

void use_after_free() {
    printk("should panic in heap\n");

    int j;
    int* p = (int*) ckalloc(sizeof(int));
    *p = 42;
    h = p;
    ckfree(p);
    printk("%x\n", p[1]);
    // h = p;

    printk("---end---\n");
}

void illegal_access() {
    printk("should panic in heap\n");

    int* p = (int*) ckalloc(sizeof(int) * 4);
    memset(p, 0, 4);
    g = p[4]; // should panic
    ckfree(p);

    printk("---end---\n");
}

void legal_access() {
    printk("should not panic\n");

    int* p = (int*) ckalloc(sizeof(int) * 4);
    memset(p, 0, 4);
    g = p[3]; // should not panic
    ckfree(p);

    printk("---end---\n");
}

void invalid_code() {
    printk("should panic on code segment\n");

    *(unsigned *)invalid_code = 0; // should panic

    printk("---end---\n");
}

void __end_pos() {}

void notmain() {
    ck_mem_set_range(__start_pos, __end_pos);

    use_after_free();
    illegal_access();
    legal_access();
    invalid_code();
}