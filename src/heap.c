/* heap.c -- NEW, MARK, RELEASE and MEMAVAIL as the Pascal compiler uses them:
   a block's identifiers and types are freed at its end by RELEASE(MARKP).
   On the P-System these are the P-machine's own (CSP 1 NEW, 32 MARK,
   33 RELEASE, 40 MEMAVAIL), as the Pascal compiler calls them -- not malloc,
   whose free list a RELEASE would break.  On the host an arena with marks. */
#include "compglbl.h"

#ifdef __TINYC__
void *PNEW(int bytes)
{
    void *p;
    __cspv(1, &p, (bytes + 1) / 2);     /* NEW: words */
    return p;
}

void PMARK(void **p)
{
    __cspv(32, p);
}

void PRELEASE(void *p)
{
    __cspv(33, &p);
}

INTEGER MEMAVAIL(void)
{
    return __cspi(40);
}
#else
#include <stdlib.h>
#define ARENA 200000
static unsigned char arena[ARENA];
static long used;
#define HOSTMEM 20000                   /* the words a program has on BIGGY, about */

void *PNEW(int bytes)
{
    void *p;
    bytes = (bytes + 7) & ~7;
    if (used + bytes > ARENA) {
        fprintf(stderr, "pc: out of host heap\n");
        exit(2);
    }
    p = arena + used;
    memset(p, 0, bytes);
    used += bytes;
    return p;
}

void PMARK(void **p)
{
    *p = arena + used;
}

void PRELEASE(void *p)
{
    used = (unsigned char *)p - arena;
}

INTEGER MEMAVAIL(void)
{
    return HOSTMEM - (INTEGER)(used / 4);   /* (a host pointer is twice a P-machine word or more) */
}
#endif
