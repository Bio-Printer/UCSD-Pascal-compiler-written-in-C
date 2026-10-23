/* pcio_psys.c -- the compiler's input and output on the P-System: the
   operating system's own file procedures, as the Pascal compiler's code
   calls them (FINIT, FOPEN, FCLOSE, FBLOCKIO; FWRITECHAR and FWRITELN for
   the listing, a TEXT file), and the console (UNITWRITE, UNITREAD).

   The files are a fixed set, as the Pascal compiler's are globals: a file
   may be open across a RELEASE of the heap.  A file read or written by
   blocks has no buffer: its FIB is NILFILESIZE words; the listing's is
   FILESIZE, its buffer in it, and its window after it.  Opening and
   closing a file is in the segment RARE: in memory only then. */
#include "pcio.h"

#define NBLOCKFILES 5                   /* the source, the code, an include file, the library, SYSTEM.INFO */

struct PFILE {
    WORD *FIB;
    BOOLEAN INUSE;
};

static WORD BLOCKFIBS[NBLOCKFILES][NILFILESIZE];
static WORD TEXTFIB[FILESIZE + 1];
static struct PFILE OPENFILES[NBLOCKFILES + 1];

INTEGER PBLOCKREAD(PFILE *f, void *buf, INTEGER nblocks, INTEGER blk)
{
    return __cxp0i(28, f->FIB, buf, 0, nblocks, blk, 1, 0, 0);     /* FBLOCKIO */
}

INTEGER PBLOCKWRITE(PFILE *f, void *buf, INTEGER nblocks, INTEGER blk)
{
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

static PFILE *popen_(CHAR *name, BOOLEAN old, BOOLEAN text)
{
    PFILE *f;
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

void PCLOSE(PFILE *f, BOOLEAN lock)
{
    if (f == NULL || !f->INUSE)
        return;
    __cxp0v(6, f->FIB, lock ? 1 : 0);           /* FCLOSE(F, NORMAL or LOCK) */
    f->INUSE = FALSE;
}
