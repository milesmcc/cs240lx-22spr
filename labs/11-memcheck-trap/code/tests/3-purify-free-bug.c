// checks a single memory overflow.
#include "rpi.h"
#include "purify.h"

void foo(char *p) {
    purify_free(p); 
}

void bar(char *p) {
    foo(p);
    p[3] = 1;
}

void notmain(void) {
    trace("should detect use after free bug (store at offset 3)\n");
    purify_init();
    char *p = purify_alloc(4);

    bar(p);
    panic("should have caught use-after-free before\n");
}
