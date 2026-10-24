/* pcio_psys.c -- the compiler's input and output on the P-System: the
   operating system's own file procedures, as the Pascal compiler's code
   calls them (FINIT, FOPEN, FCLOSE, FBLOCKIO; FWRITECHAR and FWRITELN for
   the listing, a TEXT file), and the console (UNITWRITE, UNITREAD).

   The files are a fixed set, as the Pascal compiler's are globals: a file
   may be open across a RELEASE of the heap.  A file read or written by
   blocks has no buffer: its FIB is NILFILESIZE words; the listing's is
   FILESIZE, its buffer in it, and its window after it.  Opening and
   closing a file is in the segment RARE: in memory only then.

   A file read or written by blocks may be in a sub-volume (a path,
   PASCALC:/DEMOS/PI.TEXT, or a name on the shell's path after its CD), as
   in every Tiny-C program: the library finds its place (dirs.c's __svplace,
   __svopenat) and enters a new one in its directory when it is closed
   (__svcloseat); its blocks are read and written by the disk's, within the
   file's.  Its FIB is then the library's record of it (dirs.c): the unit,
   the first block, the room, the blocks, the sub-volume's first block, ...,
   its kind and name. */
#include "pcio.h"
#include <stdio.h>                      /* (FILE: what the library's opens by a place use of it) */
#include <dirs.h>

#define NBLOCKFILES 5                   /* the source, the code, an include file, the library, SYSTEM.INFO */

struct PFILE {
    WORD *FIB;
    INTEGER INUSE;                      /* 0 free, an OS file, or in a sub-volume: */
};
#define SUBOLD 2                        /* ... read */
#define SUBNEW 3                        /* ... written: entered in the directory by PCLOSE */

static WORD BLOCKFIBS[NBLOCKFILES][NILFILESIZE];
static WORD TEXTFIB[FILESIZE + 1];
static struct PFILE OPENFILES[NBLOCKFILES + 1];

/* NBLOCKS blocks of a file in a sub-volume from its block BLK: within its blocks (read) or
   its room (written), as stdio's __blockio */
static INTEGER SUBIO(PFILE *f, void *buf, INTEGER nblocks, INTEGER blk, BOOLEAN doread)
{
    INTEGER *r = (INTEGER *)f->FIB;
    INTEGER n;
    n = (doread ? r[3] : r[2]) - blk;
    if (n > nblocks)
        n = nblocks;
    if (n <= 0)
        return 0;
    if (doread)
        __cspv(5, r[0], buf, 0, n * 512, r[1] + blk, 0);   /* UNITREAD */
    else
        __cspv(6, r[0], buf, 0, n * 512, r[1] + blk, 0);   /* UNITWRITE */
    if (__cspi(34) != 0)
        return 0;
    if (!doread && blk + n > r[3])
        r[3] = r[5] = blk + n;
    return n;
}

INTEGER PBLOCKREAD(PFILE *f, void *buf, INTEGER nblocks, INTEGER blk)
{
    if (f->INUSE >= SUBOLD)
        return SUBIO(f, buf, nblocks, blk, TRUE);
    return __cxp0i(28, f->FIB, buf, 0, nblocks, blk, 1, 0, 0);     /* FBLOCKIO */
}

INTEGER PBLOCKWRITE(PFILE *f, void *buf, INTEGER nblocks, INTEGER blk)
{
    if (f->INUSE >= SUBOLD)
        return SUBIO(f, buf, nblocks, blk, FALSE);
    return __cxp0i(28, f->FIB, buf, 0, nblocks, blk, 0, 0, 0);
}

void WCH(PFILE *f, CHAR c)
{
    if (f == NULL)
        __cspv(6, 1, &c, 0, 1, 0, 0);           /* UNITWRITE(1, C, 1) */
    else if (c == EOL)
        __cxp0v(22, f->FIB);                    /* FWRITELN */
    else
        __cxp0v(17, f->FIB, c, 0);              /* FWRITECHAR */
}

void WSTR(PFILE *f, const char *s)
{
    const CHAR *p = (const CHAR *)s;            /* (a char would be sign-extended: a library function) */
    while (*p)
        WCH(f, *p++);
}

void WPSTR(PFILE *f, CHAR *s)
{
    WPA(f, s + 1, s[0]);
}

void WPA(PFILE *f, CHAR *a, INTEGER width)
{
    while (width-- > 0)
        WCH(f, *a++);
}

void WINT(PFILE *f, INTEGER v, INTEGER width)
{
    CHAR b[8];
    int n = 0, i;
    if (v == -32767 - 1) {                      /* (its magnitude is not an INTEGER) */
        for (i = 6; i < width; i++)
            WCH(f, ' ');
        WSTR(f, "-32768");
        return;
    }
    i = v < 0 ? -v : v;                         /* (DIV and MOD of a magnitude, as C's / and %) */
    do {
        b[n++] = '0' + MOD(i, 10);
        i = DIV(i, 10);
    } while (i != 0);
    if (v < 0)
        b[n++] = '-';
    for (i = n; i < width; i++)
        WCH(f, ' ');
    while (n > 0)
        WCH(f, b[--n]);
}

void WLN(PFILE *f)
{
    WCH(f, EOL);
}

CHAR RKEY(void)
{
    CHAR c;
    __cspv(5, 2, &c, 0, 1, 0, 0);               /* UNITREAD(2, C, 1) */
    return c;
}

void PTIME(INTEGER *HI, INTEGER *LO)
{
    __cspv(9, HI, LO);                          /* TIME */
}

#pragma segment RARE

/* f, by blocks, in a sub-volume: NAME its path; 0 if it is not there */
static INTEGER SUBOPEN(PFILE *f, char *name, BOOLEAN old)
{
    char buf[512];                      /* (the directory, a block at a time) */
    FILE fake;
    struct svat at;
    if (!__svplace(name, &at))
        return 0;
    fake.fib = (char *)f->FIB;
    fake.buf = (unsigned char *)buf;
    if (!__svopenat(&fake, old ? "r" : "w", &at))
        return 0;
    return old ? SUBOLD : SUBNEW;
}

static PFILE *popen_(CHAR *name, BOOLEAN old, BOOLEAN text)
{
    PFILE *f;
    char cname[82];
    int i;
    i = 0;
    if (text)
        i = NBLOCKFILES;
    else
        while (i < NBLOCKFILES && OPENFILES[i].INUSE)
            i++;
    if (OPENFILES[i].INUSE)
        return NULL;
    f = &OPENFILES[i];
    f->FIB = text ? TEXTFIB : BLOCKFIBS[i];
    for (i = 0; i < name[0] && i < 80; i++)
        cname[i] = name[1 + i];
    cname[i] = 0;
    if (!text && __svname(cname) && (f->INUSE = SUBOPEN(f, cname, old)) != 0)
        return f;                               /* (a path or a name on the shell's path: stdio's __svname) */
    if (text)
        __cxp0v(3, f->FIB, &f->FIB[FILESIZE], -2);  /* FINIT(F, F^, TEXT) */
    else
        __cxp0v(3, f->FIB, 0, -1);              /* FINIT(F, NIL, untyped) */
    __cxp0v(5, f->FIB, name, old, 0);           /* FOPEN */
    if (__cspi(34) != 0)                        /* IORESULT */
        return NULL;
    f->INUSE = TRUE;
    return f;
}

PFILE *POPENOLD(CHAR *name)
{
    return popen_(name, TRUE, FALSE);
}

PFILE *POPENNEW(CHAR *name, BOOLEAN text)
{
    return popen_(name, FALSE, text);
}

static void SUBCLOSE(PFILE *f)                  /* a new file in a sub-volume: its entry */
{
    int dir[1024];                              /* (the directory) */
    FILE fake;
    struct svat at;
    fake.fib = (char *)f->FIB;
    fake.flags = __F_SUB | __F_NEW;
    at.dir = dir;
    __svcloseat(&fake, &at);
}

void PCLOSE(PFILE *f, BOOLEAN lock)
{
    if (f == NULL || !f->INUSE)
        return;
    if (f->INUSE == SUBNEW && lock)
        SUBCLOSE(f);
    else if (f->INUSE < SUBOLD)
        __cxp0v(6, f->FIB, lock ? 1 : 0);       /* FCLOSE(F, NORMAL or LOCK) */
    f->INUSE = 0;
}
