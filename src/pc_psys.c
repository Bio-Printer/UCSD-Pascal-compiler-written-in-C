/* pc_psys.c -- the compiler from the shell:

     PC NAME [CODE]

   compiles NAME.TEXT (NAME if it ends in .TEXT) to CODE (NAME.CODE),
   as the system's C(ompile would. */
#include <string.h>
#include "pcio.h"

/* a Pascal string: S, then SUFFIX unless S ends with it */
static CHAR *title(char *s, char *suffix, CHAR *buf)
{
    int n = strlen(s), k = strlen(suffix);
    memcpy(buf + 1, s, n);
    if (n < k || strcmp(s + n - k, suffix) != 0) {
        memcpy(buf + 1 + n, suffix, k);
        n = n + k;
    }
    buf[0] = n;
    return buf;
}

int main(int argc, char **argv)
{
    CHAR n[64];
    char base[40];
    int k;
    if (argc < 2 || argc > 3) {
        WSTR(NULL, "usage: PC NAME [CODE]\r");
        return 2;
    }
    USERINFO.WORKSYM = POPENOLD(title(argv[1], ".TEXT", n));
    if (USERINFO.WORKSYM == NULL) {
        WSTR(NULL, "PC: no ");
        WSTR(NULL, argv[1]);
        WLN(NULL);
        return 2;
    }
    strncpy(base, argv[1], 39);
    base[39] = 0;
    k = strlen(base);
    if (k > 5 && strcmp(base + k - 5, ".TEXT") == 0)
        base[k - 5] = 0;
    USERINFO.WORKCODE = POPENNEW(title(argc == 3 ? argv[2] : base, ".CODE", n), FALSE);
    if (USERINFO.WORKCODE == NULL) {
        WSTR(NULL, "PC: cannot write the code file\r");
        return 2;
    }
    USERINFO.ERRSYM = USERINFO.ERRBLK = USERINFO.ERRNUM = 0;
    USERINFO.SLOWTERM = FALSE;
    USERINFO.STUPID = FALSE;
    USERINFO.ALTMODE = 27;
    if (setjmp(EXITCOMPILER) == 0)
        PASCALCOMPILER();
    WLN(NULL);
    PCLOSE(USERINFO.WORKSYM, FALSE);
    PCLOSE(USERINFO.WORKCODE, USERINFO.ERRNUM == 0);
    return USERINFO.ERRNUM != 0;
}
