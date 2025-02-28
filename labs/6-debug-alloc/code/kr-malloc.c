#include "kr-malloc.h"

#define roundup(x, n) (((x) + ((n)-1)) & (~((n)-1)))

union align
{
	double d;
	void *p;
	void (*fp)(void);
};

typedef union header
{ /* block header */
	struct
	{
		union header *ptr; /* next block if on free list */
		unsigned size;	   /* size of this block */
	} s;
	union align x; /* force alignment of blocks */
} Header;

#define NALLOC 1024 /* minimum #units to request */

static Header base;			 /* empty list to get started */
static Header *freep = NULL; /* start of free list */

/* morecore:  ask system for more memory */
static Header *morecore(unsigned nu)
{
	if (nu < NALLOC)
		nu = NALLOC;
	printk("Requersting more memory (%d)...\n", nu);
	char *cp = sbrk(nu * sizeof(Header));
	if (cp == (char *)-1) /* no space at all */
		return NULL;
	Header *up = (Header *)cp;
	up->s.size = nu;
	kr_free((void *)(up + 1));
	return freep;
}

/* malloc:  general-purpose storage allocator */
void *kr_malloc(unsigned nbytes)
{
	Header *p, *prevp;
	unsigned nunits;

	nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;
	if ((prevp = freep) == NULL)
	{ /* no free list yet */
		base.s.ptr = freep = prevp = &base;
		base.s.size = 0;
	}

	for (p = prevp->s.ptr;; prevp = p, p = p->s.ptr)
	{
		if (p->s.size >= nunits)
		{ /* big enough */
			if (p->s.size == nunits)
			{ /* exactly */
				prevp->s.ptr = p->s.ptr;
			}
			else
			{ /* allocate tail end */
				p->s.size -= nunits;
				p += p->s.size;
				p->s.size = nunits;
			}
			freep = prevp;
			return (void *)(p + 1);
		}
		if (p == freep)
		{ /* wrapped around free list */
			if ((p = morecore(1024*32)) == NULL)
			{
				return NULL; /* none left */
			}
		}
	}
}

/* free:  put block ap in free list */
void kr_free(void *ap)
{
	Header *bp, *p;
	bp = (Header *)ap - 1; /* point to  block header */
	for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
		if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
			break; /* freed block at start or end of arena */
	if (bp + bp->s.size == p->s.ptr)
	{ /* join to upper nbr */
		bp->s.size += p->s.ptr->s.size;
		bp->s.ptr = p->s.ptr->s.ptr;
	}
	else
		bp->s.ptr = p->s.ptr;
	if (p + p->s.size == bp)
	{ /* join to lower nbr */
		p->s.size += bp->s.size;
		p->s.ptr = bp->s.ptr;
	}
	else
		p->s.ptr = bp;
	freep = p;
}
