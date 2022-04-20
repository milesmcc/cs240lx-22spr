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
    return &h[1];
}

// one past the last byte of allocated memory.
void *ck_hdr_end(hdr_t *h)
{
    return (char *)ck_hdr_start(h) + ck_nbytes(h);
}

// is ptr in <h>?
unsigned ck_ptr_in_block(hdr_t *h, void *ptr)
{
    return ((char *)ptr >= (char *)h) && ((char *)ptr <= (char *)(h + h->nbytes_alloc + sizeof h));
}

int ck_ptr_is_alloced(void *ptr)
{
    // output("checking if %p is alloced...\n", ptr);
    for (hdr_t *h = ck_first_hdr(); h; h = ck_next_hdr(h))
    {
        // output("checking if %p is alloced in %p...\n", ptr, h);
        if (ck_ptr_in_block(h, ptr))
        {
            // output("found %p in %p\n", ptr, h);
            return 1;
        }
        else
        {
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

    h->state = FREED;
    assert(ck_ptr_is_alloced(addr));

    list_remove(&alloc_list, h);
    kr_free(h);
}

// interpose on kr_malloc allocations and
//  1. allocate enough space for a header and fill it in.
//  2. add the allocated block to  the allocated list.
void *(ckalloc)(unsigned nbytes, src_loc_t l)
{
    hdr_t *h = kr_malloc(nbytes + sizeof *h);
    memset(h, 0, sizeof *h);

    h->nbytes_alloc = nbytes;
    h->state = ALLOCED;
    h->alloc_loc = l;

    loc_debug(l, "allocating \t%p\n", h + 1);
    h->next = alloc_list;
    alloc_list = h;

    return h + 1;
}
