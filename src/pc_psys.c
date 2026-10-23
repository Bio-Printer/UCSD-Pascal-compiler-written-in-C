/* pc_psys.c -- the compiler from the shell:

     PC NAME [CODE]

   compiles NAME.TEXT (NAME if it ends in .TEXT) to CODE (NAME.CODE),
   as the system's C(ompile would.

   The command line is read as the library's __callmain reads it (pexec.c),
   but without its redirection (>NAME, <NAME), which would bring stdio in:
   main has no parameters.  Reading it and opening the files is in the
   segment RARE, with no library functions, so that main is all that stays
   in memory. */
#include <psys.h>
#include "pcio.h"

#pragma segment RARE

#define MAXARGS 4

/* the shell's command line (the OS's prompt line, PL), split at blanks:
   argv[0] the program's name; the result is argc */
static int ARGS(CHAR *line, CHAR **argv)
{
    int *e;
    CHAR *pl;
    int sum;
    int n, i, argc;
    e = SYSCOM->expansion;
    pl = (CHAR *)__osvaraddr(PX_PL);
    n = 0;
    sum = pl[0];                                /* (the checksum pexec gives it) */
    for (i = 1; i <= pl[0] && i <= 80; i++)
        sum = sum * 3 + pl[i];
    sum = (sum | 0x5A5A) & ~(sum & 0x5A5A);     /* ^ 0x5A5A (^ is a library function) */
    if (e[0] == PX_CHILD && e[6] == PX_ARGS && pl[0] <= 80 && e[7] == sum)
        n = pl[0];
    e[6] = 0;
    for (i = 0; i < n && pl[i + 1] != PX_PATHSEP; i++)  /* (the shell's path after the arguments) */
        line[i] = pl[i + 1];
    n = i;
    line[n] = 0;
    argc = 0;
    if (n == 0)
        argv[argc++] = line;                    /* (not started by the shell: argv[0] is "") */
    i = 0;
    while (i < n && argc <= MAXARGS) {
        while (i < n && line[i] == ' ')
            i++;
        if (i == n)
            break;
        argv[argc++] = line + i;
        while (i < n && line[i] != ' ')
            i++;
        if (i < n)
            line[i++] = 0;
    }
    return argc;
}

/* S's length if it ends with SUFFIX, else -1 */
static int ENDSWITH(CHAR *s, char *sfx)
{
    CHAR *suffix = (CHAR *)sfx;                 /* (a char would be sign-extended) */
    int n, k, i;
    for (n = 0; s[n]; n++)
        ;
    for (k = 0; suffix[k]; k++)
        ;
    if (n < k)
        return -1;
    for (i = 0; i < k; i++)
        if (s[n - k + i] != suffix[i])
            return -1;
    return n;
}

/* a Pascal string: S (the first N characters), then SUFFIX */
static CHAR *TITLE(CHAR *s, int n, char *sfx, CHAR *buf)
{
    CHAR *suffix = (CHAR *)sfx;
    int i;
    for (i = 0; i < n; i++)
        buf[1 + i] = s[i];
    while (*suffix)
        buf[1 + n++] = *suffix++;
    buf[0] = n;
    return buf;
}

static int LENGTH(CHAR *s)
{
    int n;
    for (n = 0; s[n]; n++)
        ;
    return n;
}

/* the command line and the files; FALSE (said why) if they cannot be had */
static BOOLEAN SETUP(void)
{
    CHAR line[82];
    CHAR *argv[MAXARGS + 1];
    int argc, n;
    CHAR t[64];
    argc = ARGS(line, argv);
    if (argc < 2 || argc > 3) {
        WSTR(NULL, "usage: PC NAME [CODE]\r");
        return FALSE;
    }
    n = ENDSWITH(argv[1], ".TEXT");
    if (n >= 0)
        TITLE(argv[1], n, "", t);
    else
        TITLE(argv[1], LENGTH(argv[1]), ".TEXT", t);
    USERINFO.WORKSYM = POPENOLD(t);
    if (USERINFO.WORKSYM == NULL) {
        WSTR(NULL, "PC: no ");
        WSTR(NULL, (char *)argv[1]);
        WLN(NULL);
        return FALSE;
    }
    if (argc == 3) {
        n = ENDSWITH(argv[2], ".CODE");
        if (n >= 0)
            TITLE(argv[2], n, "", t);
        else
            TITLE(argv[2], LENGTH(argv[2]), ".CODE", t);
    } else {
        n = ENDSWITH(argv[1], ".TEXT");
        TITLE(argv[1], n >= 0 ? n - 5 : LENGTH(argv[1]), ".CODE", t);
    }
    USERINFO.WORKCODE = POPENNEW(t, FALSE);
    if (USERINFO.WORKCODE == NULL) {
        WSTR(NULL, "PC: cannot write the code file\r");
        return FALSE;
    }
    USERINFO.ERRSYM = USERINFO.ERRBLK = USERINFO.ERRNUM = 0;
    USERINFO.SLOWTERM = FALSE;
    USERINFO.STUPID = FALSE;
    USERINFO.ALTMODE = 27;
    return TRUE;
}

#pragma segment MAIN

int main(void)
{
    if (!SETUP())
        return 2;
    PASCALCOMPILER();
    WLN(NULL);
    PCLOSE(USERINFO.WORKSYM, FALSE);
    PCLOSE(USERINFO.WORKCODE, USERINFO.ERRNUM == 0);
    return USERINFO.ERRNUM != 0;
}
