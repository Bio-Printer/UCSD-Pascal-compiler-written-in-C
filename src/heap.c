/* heap.c -- NEW, MARK, RELEASE and MEMAVAIL as the Pascal compiler uses them:
   a block's identifiers and types are freed at its end by RELEASE(MARKP).
   On the P-System these are the P-machine's own (CSP 1 NEW, 32 MARK,
   33 RELEASE, 40 MEMAVAIL), as the Pascal compiler calls them -- not malloc,
   whose free list a RELEASE would break.  On the host an arena with marks.

   NEW does not clear what it gives, and the compiler leaves some bytes of
   what it writes unset (the byte GENWORD skips to a word).  On the host new
   memory is filled with the byte $PCFILL (0 if unset), so that two runs with
   different fills show which bytes of the output depend on it (compare.py). */
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
static int fill = -1;
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
    if (fill < 0)
        fill = getenv("PCFILL") ? strtol(getenv("PCFILL"), NULL, 0) & 255 : 0;
    memset(p, fill, bytes);
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
