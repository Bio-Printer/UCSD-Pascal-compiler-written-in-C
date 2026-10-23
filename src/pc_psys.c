/* pc_psys.c -- the compiler from the shell:

     PC NAME [CODE]

   compiles NAME.TEXT (NAME if it ends in .TEXT) to CODE (NAME.CODE),
   as the system's C(ompile would.

   The command line is read as the library's __callmain reads it (pexec.c),
   but without its redirection (>NAME, <NAME), which would bring stdio in:
   main has no parameters, and less code stays in memory. */
#include <string.h>
#include <psys.h>
#include "pcio.h"

#define MAXARGS 4

/* the shell's command line (the OS's prompt line, PL), split at blanks:
   argv[0] the program's name; the result is argc */
static int ARGS(char *line, char **argv)
{
    int *e;
    unsigned char *pl;
    unsigned sum;
    int n, i, argc;
    e = SYSCOM->expansion;
    pl = (unsigned char *)__osvaraddr(PX_PL);
    n = 0;
    sum = pl[0];                                /* (the checksum pexec gives it) */
    for (i = 1; i <= pl[0] && i <= 80; i++)
        sum = sum * 3 + pl[i];
    if (e[0] == PX_CHILD && e[6] == PX_ARGS && pl[0] <= 80 && e[7] == (int)(sum ^ 0x5A5A))
        n = pl[0];
    e[6] = 0;
    memcpy(line, pl + 1, n);
    line[n] = 0;
    for (i = 0; i < n && line[i] != PX_PATHSEP; i++)   /* (the shell's path after the arguments) */
        ;
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

int main(void)
{
    char line[82];
    char *argv[MAXARGS + 1];
    int argc;
    CHAR n[64];
    char base[40];
    int k;
    argc = ARGS(line, argv);
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
    PASCALCOMPILER();
    WLN(NULL);
    PCLOSE(USERINFO.WORKSYM, FALSE);
    PCLOSE(USERINFO.WORKCODE, USERINFO.ERRNUM == 0);
    return USERINFO.ERRNUM != 0;
}
