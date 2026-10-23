/* pc_host.c -- the compiler on the host:

     pc SOURCE CODE

   compiles the UCSD text file SOURCE (its include files beside it) to the
   code file CODE, as the P-System's C(ompile would: what the system gives
   the compiler is USERINFO.  The exit status is 1 if there were errors. */
#include <stdlib.h>
#include "pcio.h"

static CHAR *pstr(const char *s, CHAR *buf)
{
    buf[0] = strlen(s);
    memcpy(buf + 1, s, buf[0]);
    return buf;
}

int main(int argc, char **argv)
{
    CHAR n[256];
    if (argc != 3) {
        fprintf(stderr, "usage: pc SOURCE CODE\n");
        return 2;
    }
    USERINFO.WORKSYM = POPENOLD(pstr(argv[1], n));
    if (USERINFO.WORKSYM == NULL) {
        fprintf(stderr, "pc: no %s\n", argv[1]);
        return 2;
    }
    USERINFO.WORKCODE = POPENNEW(pstr(argv[2], n));
    if (USERINFO.WORKCODE == NULL) {
        fprintf(stderr, "pc: cannot write %s\n", argv[2]);
        return 2;
    }
    USERINFO.ERRSYM = USERINFO.ERRBLK = USERINFO.ERRNUM = 0;
    USERINFO.SLOWTERM = FALSE;
    USERINFO.STUPID = FALSE;
    USERINFO.ALTMODE = 27;
    if (setjmp(EXITCOMPILER) == 0)
        PASCALCOMPILER();
    putchar('\n');
    PCLOSE(USERINFO.WORKSYM, FALSE);
    PCLOSE(USERINFO.WORKCODE, USERINFO.ERRNUM == 0);
    return USERINFO.ERRNUM != 0;
}
