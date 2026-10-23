/* pcio.h -- the compiler's input and output, as the Pascal compiler does it:
   whole blocks of the work files (BLOCKREAD / BLOCKWRITE), include files and the
   library opened by name (OPENOLD / OPENNEW, IORESULT), the listing (a TEXT file)
   and the console (OUTPUT, KEYBOARD).  pcio_host.c on the host, pcio_psys.c on
   the P-System (the operating system's own file procedures). */
#ifndef PCIO_H
#define PCIO_H
#include "compglbl.h"

struct PFILE;
typedef struct PFILE PFILE;

/* name: a Pascal string ([0] the length).  The result: the file, or NULL
   (IORESULT <> 0) */
PFILE *POPENOLD(CHAR *name);
PFILE *POPENNEW(CHAR *name);
void PCLOSE(PFILE *f, BOOLEAN lock);    /* CLOSE(F) / CLOSE(F, LOCK) */
INTEGER PBLOCKREAD(PFILE *f, void *buf, INTEGER nblocks, INTEGER blk);
INTEGER PBLOCKWRITE(PFILE *f, void *buf, INTEGER nblocks, INTEGER blk);

/* WRITE to a TEXT file; f NULL: OUTPUT, the console */
void WSTR(PFILE *f, const char *s);             /* a string constant */
void WPSTR(PFILE *f, CHAR *s);                  /* a Pascal string */
void WPA(PFILE *f, CHAR *a, INTEGER width);     /* a packed array of CHAR, A:WIDTH */
void WINT(PFILE *f, INTEGER v, INTEGER width);  /* I:WIDTH (0: as wide as it is) */
void WCH(PFILE *f, CHAR c);
void WLN(PFILE *f);                             /* WRITELN */
CHAR RKEY(void);                                /* READ(KEYBOARD, CH) */

#endif
