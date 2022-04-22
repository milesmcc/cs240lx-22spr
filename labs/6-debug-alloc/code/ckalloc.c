// implement a simple ckalloc/free that adds ckhdr_t to the
// allocation.

#include "test-interface.h"
#include "ckalloc.h"

// keep a list of allocated blocks.
static hdr_t *alloc_list;

// returns pointer to the first header block.
hdr_t *ck_first_hdr(void)
{
    return alloc_list;
}

// returns pointer to next hdr or 0 if none.
hdr_t *ck_next_hdr(hdr_t *p)
{
    if (p)
        return p->next;
    return 0;
}

void *ck_hdr_start(hdr_t *h)
{
    return ((char*)(h + 1));
}

// one past the last byte of allocated memory.
void *ck_hdr_end(hdr_t *h)
{
    return (char *)ck_hdr_start(h) + ck_nbytes(h);
}

// is ptr in <h>?
unsigned ck_ptr_in_block(hdr_t *h, void *ptr)
{
    void *min = ck_hdr_start(h);
    void *max = ck_hdr_end(h);
    return (ptr >= min) && (ptr < max);
}

hdr_t *ck_ptr_is_alloced(void *ptr)
{
    // output("checking if %p is alloced...\n", ptr);
    for (hdr_t *h = ck_first_hdr(); h; h = ck_next_hdr(h))
    {
        // output("checking if %p is alloced in %p...\n", ptr, h);
        if (ck_ptr_in_block(h, ptr))
        {
            output("found %p in %p\n", ptr, h);
            return h;
        }
    }
    return 0;
}

static void list_remove(hdr_t **l, hdr_t *h)
{
    assert(l);
    hdr_t *prev = *l;

    if (prev == h)
    {
        *l = h->next;
        return;
    }

    hdr_t *p;
    while ((p = ck_next_hdr(prev)))
    {
        if (p == h)
        {
            prev->next = p->next;
            return;
        }
        prev = p;
    }
    panic("did not find %p in list\n", h);
}

// free a block allocated with <ckalloc>
void(ckfree)(void *addr, src_loc_t l)
{
    hdr_t *h = (void *)addr;
    h -= 1;

    loc_debug(l, "freeing \t%p\n", addr);
    if (h->state != ALLOCED)
        loc_panic(l, "freeing unallocated memory: state=%d\n", h->state);

    for(int i = 0; i < REDZONE_NBYTES; i++) {
        if (h->rz1[i] != REDZONE_VAL) {
            loc_panic(l, "redzone1 was corrupted! found %x, expected %x\n", h->rz1[i], REDZONE_VAL);
        }
        if (*((char *)(h + 1) + h->nbytes_alloc + i) != REDZONE_VAL) {
            loc_panic(l, "redzone2 was corrupted!\n");
        }
    }

    // Write over data
    for(int i = 0; i < h->nbytes_alloc; i++) {
        *((char*)addr + i) = REDZONE_VAL; 
    }

    h->state = FREED;
    // TODO: set src loc
    assert(ck_ptr_is_alloced(addr));

    list_remove(&alloc_list, h);
    // kr_free(h); // We don't want to reuse!
}

// interpose on kr_malloc allocations and
//  1. allocate enough space for a header and fill it in.
//  2. add the allocated block to  the allocated list.
void *(ckalloc)(uint32_t nbytes, src_loc_t l)
{
    hdr_t *h = kr_malloc(nbytes + sizeof *h + REDZONE_NBYTES);

    // Write header zeros
    memset(h, 0, sizeof *h);

    // Write Redzone data
    for(int i = 0; i < REDZONE_NBYTES; i++) {
        h->rz1[i] = REDZONE_VAL; // Don't want to use memset
        *(((char *)(h + 1)) + nbytes + i) = REDZONE_VAL;
    }

    h->nbytes_alloc = nbytes;
    h->state = ALLOCED;
    h->alloc_loc = l;

    loc_debug(l, "allocating %p (%d bytes)\n", h + 1, nbytes);

    h->next = alloc_list;
    alloc_list = h;

    return ((char *)(h + 1));
    // return h + 1;
}
