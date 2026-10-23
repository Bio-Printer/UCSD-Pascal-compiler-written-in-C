/* pcio_host.c -- the compiler's input and output on the host: files in the
   current directory, as the P-System has them (a text file is its 2-block
   header, then 2-block pages), and the console on stdout.

   A file's name is taken as the P-System would have it: without blanks, a
   volume (VOL: or #5:) or the boot volume's * is dropped, as is a size
   ([*]), and it is looked up in capitals.  Text written to a file follows the OS's FPUT: a
   line end in a page's second block, 385 or more bytes into it, ends the
   page there (the rest is zeros), and a file ends on a page. */
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "pcio.h"

struct PFILE {
    FILE *fp;
    long pos;                           /* text written so far, the header included */
    BOOLEAN isnew, text;
    char name[80];
};

/* the host name of a Pascal string name: as the OS's SCANTITLE has it, no
   blanks and in capitals; without its volume and size */
static void hostname(CHAR *name, char *out)
{
    char t[256];
    int n = 0, i, j = 0, k = 0;
    for (i = 1; i <= name[0]; i++)
        if (name[i] > ' ')
            t[n++] = toupper(name[i]);
    for (i = 0; i < n; i++)
        if (t[i] == ':')
            k = i + 1;
    if (k < n && t[k] == '*')
        k++;
    while (k < n && j < 79 && t[k] != '[')
        out[j++] = t[k++];
    out[j] = 0;
}

static PFILE *popen_(CHAR *name, const char *mode, BOOLEAN isnew, BOOLEAN text)
{
    PFILE *f;
    char n[80];
    FILE *fp;
    hostname(name, n);
    if (n[0] == 0 || !(fp = fopen(n, mode)))
        return NULL;
    f = calloc(1, sizeof *f);
    f->fp = fp;
    f->isnew = isnew;
    f->text = text;
    strcpy(f->name, n);
    if (text) {                         /* (a text file's header: zeros, as the OS writes it) */
        static char zero[1024];
        fwrite(zero, 1, 1024, fp);
        f->pos = 1024;
    }
    return f;
}

PFILE *POPENOLD(CHAR *name)
{
    PFILE *f = popen_(name, "r+b", FALSE, FALSE);
    return f ? f : popen_(name, "rb", FALSE, FALSE);
}

PFILE *POPENNEW(CHAR *name, BOOLEAN text)
{
    return popen_(name, "w+b", TRUE, text);
}

void PCLOSE(PFILE *f, BOOLEAN lock)
{
    static char zero[512];
    if (f == NULL)
        return;
    if (f->isnew && !lock) {            /* a new file closed without LOCK is gone */
        fclose(f->fp);
        remove(f->name);
    } else {
        if (f->text && f->pos > 1024) {        /* (its last page filled) */
            fseek(f->fp, f->pos, SEEK_SET);
            if (f->pos % 512)
                fwrite(zero, 1, 512 - f->pos % 512, f->fp);
            if ((f->pos + 511) / 512 % 2)
                fwrite(zero, 1, 512, f->fp);
        }
        fclose(f->fp);
    }
    free(f);
}

INTEGER PBLOCKREAD(PFILE *f, void *buf, INTEGER nblocks, INTEGER blk)
{
    if (f == NULL || fseek(f->fp, (long)blk * 512, SEEK_SET) != 0)
        return 0;
    return fread(buf, 512, nblocks, f->fp);
}

INTEGER PBLOCKWRITE(PFILE *f, void *buf, INTEGER nblocks, INTEGER blk)
{
    if (f == NULL || fseek(f->fp, (long)blk * 512, SEEK_SET) != 0)
        return 0;
    return fwrite(buf, 512, nblocks, f->fp);
}

void WCH(PFILE *f, CHAR c)
{
    static char zero[512];
    long blk;
    if (f == NULL) {
        putchar(c == EOL ? '\n' : c);
        return;
    }
    fseek(f->fp, f->pos, SEEK_SET);
    fputc(c, f->fp);
    f->pos++;
    if (c == EOL) {
        blk = (f->pos - 1) / 512;
        if (blk % 2 && f->pos - blk * 512 >= 385) {    /* (the page ends here) */
            fwrite(zero, 1, (blk + 1) * 512 - f->pos, f->fp);
            f->pos = (blk + 1) * 512;
        }
    }
}

void WSTR(PFILE *f, const char *s)
{
    while (*s)
        WCH(f, *s++);
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
    char b[8];
    int n = sprintf(b, "%d", v);
    while (width-- > n)
        WCH(f, ' ');
    WSTR(f, b);
}

void WLN(PFILE *f)
{
    WCH(f, EOL);
}

CHAR RKEY(void)
{
    return ' ';                         /* (no one at the keyboard: go on) */
}

void PTIME(INTEGER *HI, INTEGER *LO)
{
    struct timespec t;
    long long ticks;
    clock_gettime(CLOCK_MONOTONIC, &t);
    ticks = (long long)t.tv_sec * 60 + t.tv_nsec / 16666667;
    *HI = (INTEGER)(ticks >> 16);
    *LO = (INTEGER)ticks;
}
