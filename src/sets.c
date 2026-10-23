/* sets.c -- Pascal sets of up to 64 members (SETOFSYS, SETOFIDS): a set is 4
   words; [A, B, ...] and the set operators */
#include <stdarg.h>
#include "compglbl.h"

static void clear(SETS *s)
{
    s->w[0] = s->w[1] = s->w[2] = s->w[3] = 0;
}

/* s := [x, ...]: the members, then -1 */
SETS *SETOF(SETS *s, int x, ...)
{
    va_list ap;
    clear(s);
    va_start(ap, x);
    while (x >= 0) {
        s->w[x >> 4] |= 1 << (x & 15);
        x = va_arg(ap, int);
    }
    va_end(ap);
    return s;
}

/* s := a + [x, ...] */
SETS *SETADD(SETS *s, SETS *a, int x, ...)
{
    va_list ap;
    *s = *a;
    va_start(ap, x);
    while (x >= 0) {
        s->w[x >> 4] |= 1 << (x & 15);
        x = va_arg(ap, int);
    }
    va_end(ap);
    return s;
}

SETS *SETPLUS(SETS *s, SETS *a, SETS *b)
{
    int i;
    for (i = 0; i < 4; i++)
        s->w[i] = a->w[i] | b->w[i];
    return s;
}

SETS *SETMINUS(SETS *s, SETS *a, SETS *b)
{
    int i;
    for (i = 0; i < 4; i++)
        s->w[i] = a->w[i] & ~b->w[i];
    return s;
}
