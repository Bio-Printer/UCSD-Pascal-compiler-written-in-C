/* rare.c -- what the compiler seldom does, in a segment of its own so that
   it is not in memory the rest of the time: ERROR (PROCS.A), the compiler
   options of a comment ($, COMMENTER's), a long integer or REAL constant
   (NUMBER's), and the end of the compile: the summary and the code file's
   segment dictionary (PASCALCOMPILER's, BLOCK.TEXT). */
#include "compglbl.h"
#include "pcio.h"

#pragma segment RARE

#define MOVELEFT(src, dst, n) memmove(dst, src, n)

void ERROR(INTEGER ERRORNUM)
{
    CHAR CH;
    INTEGER ERRSTART;
    CHAR A[180];
    CH = 0;
    if (USERINFO.ERRSYM != SYMCURSOR || USERINFO.ERRBLK != SYMBLK) {
        USERINFO.ERRBLK = SYMBLK;
        USERINFO.ERRSYM = SYMCURSOR;
        USERINFO.ERRNUM = ERRORNUM;
        if (USERINFO.STUPID)
            CH = 'E';
        else {
            if (NOISY)
                WLN(NULL);
            else if (LIST && ERRORNUM <= 400)
                return;                                 /* EXIT(ERROR) */
            if (LINESTART == 0)
                WPA(NULL, SYMBUFP, SYMCURSOR);
            else {
                ERRSTART = SCAN(-(LINESTART - 1), FALSE, EOL, &SYMBUFP[LINESTART - 2]) + LINESTART - 1;
                MOVELEFT(&SYMBUFP[ERRSTART], &A[0], SYMCURSOR - ERRSTART);
                WPA(NULL, A, SYMCURSOR - ERRSTART);
            }
            WSTR(NULL, " <<<<");
            WLN(NULL);
            WSTR(NULL, "Line ");
            WINT(NULL, SCREENDOTS, 0);
            WSTR(NULL, ", error ");
            WINT(NULL, ERRORNUM, 0);
            WSTR(NULL, ":");
            if (NOISY)
                WSTR(NULL, " <sp>(continue), <esc>(terminate), E(dit");
            WCH(NULL, 7);
            do
                CH = RKEY();
            while (!(CH == ' ' || CH == 'E' || CH == 'e' || CH == USERINFO.ALTMODE));
        }
        if (CH == 'E' || CH == 'e') {
            USERINFO.ERRBLK = SYMBLK - 2;
            longjmp(EXITCOMPILER, 1);                   /* EXIT(PASCALCOMPILER) */
        }
        if (ERRORNUM > 400 || CH == 27) {
            USERINFO.ERRBLK = 0;
            longjmp(EXITCOMPILER, 1);
        }
        WLN(NULL);
        if (NOISY) {
            WCH(NULL, '<');
            WINT(NULL, SCREENDOTS, 4);
            WCH(NULL, '>');
        }
    }
}

/* COMMENTER: the options of a comment that starts with $.  The result: 0 the
   rest of the comment is skipped, 1 EXIT(COMMENTER), 2 EXIT(INSYMBOL) (an
   include file: INSYMBOL has been called for its first symbol) */
INTEGER OPTIONS(CHAR STOPPER)
{
    CHAR CH, SW, DEL;
    CHAR LTITLE[41];                                    /* STRING[40] */
    CHAR T[48];
    do {
        CH = SYMBUFP[SYMCURSOR + 1];
        SW = SYMBUFP[SYMCURSOR + 2];
        DEL = SYMBUFP[SYMCURSOR + 3];
        if (SW == ',' || SW == STOPPER) {
            DEL = SW;
            SW = '+';
            SYMCURSOR = SYMCURSOR - 1;
        }
        switch (CH) {
        case 'C':
            if (LEVEL > 1)
                ERROR(194);
            COMMENT = PNEW(STRGLGTH + 1);       /* NEW(COMMENT): a STRING */
            SCANSTRING(COMMENT, 80, STOPPER);
            return 1;                           /* EXIT(COMMENTER) */
        case 'D':
            DEBUGGING = SW == '+';
            break;
        case 'F':
            FLIPBYTES = SW == '+';
            break;
        case 'G':
            GOTOOK = SW == '+';
            break;
        case 'I':
            if (SW == '+' || SW == '-')
                IOCHECK = SW == '+';
            else {
                SCANSTRING(LTITLE, 40, STOPPER);
                if (STOPPER == '*')
                    SYMCURSOR = SYMCURSOR + 1;
                if (LIST) {
                    SYMCURSOR = SYMCURSOR + 1;
                    PRINTLINE();
                    SYMCURSOR = SYMCURSOR - 1;
                }
                if (INCLUDING || (INMODULE && ININTERFACE)) {
                    ERROR(406);
                    return 1;
                }
                INCLFILE = POPENOLD(LTITLE);
                if (!INCLFILE) {
                    memcpy(T, LTITLE, LTITLE[0] + 1);   /* CONCAT(LTITLE, '.TEXT') */
                    memcpy(&T[T[0] + 1], ".TEXT", 5);
                    T[0] = T[0] + 5;
                    INCLFILE = POPENOLD(T);
                    if (!INCLFILE)
                        ERROR(403);
                }
                INCLUDING = TRUE;
                OLDSYMCURSOR = SYMCURSOR;
                OLDLINESTART = LINESTART;
                OLDSYMBLK = SYMBLK - 2;
                SYMBLK = 2;
                GETNEXTPAGE();
                INSYMBOL();
                return 2;                       /* EXIT(INSYMBOL) */
            }
            break;
        case 'L':
            if (SW == '+' || SW == '-') {
                LIST = SW == '+';
                if (LIST) {
                    static CHAR LST[] = "\020*SYSTEM.LST.TEXT";
                    LP = POPENNEW(LST, TRUE);
                }
            } else {
                SCANSTRING(LTITLE, 40, STOPPER);
                LP = POPENNEW(LTITLE, TRUE);
                LIST = LP != NULL;
                return 1;
            }
            break;
        case 'Q':
            NOISY = SW == '-';
            break;
        case 'P':
            WCH(LP, 12);                        /* FF */
            break;
        case 'R':
            RANGECHECK = SW == '+';
            break;
        case 'S':
            NOSWAP = SW == '-';
            break;
        case 'T':
            TINY = SW == '+';
            break;
        case 'U':
            if (SW == '+' || SW == '-') {
                SYSCOMP = SW == '-';
                RANGECHECK = !SYSCOMP;
                IOCHECK = RANGECHECK;
                GOTOOK = SYSCOMP;
            } else if (!USING) {
                SCANSTRING(SYSTEMLIB, 40, STOPPER);
                if (LIBRARY)
                    PCLOSE(LIBRARY, FALSE);
                LIBRARY = NULL;
                LIBNOTOPEN = TRUE;
                return 1;
            }
            break;
        }
        SYMCURSOR = SYMCURSOR + 3;
    } while (DEL == ',');
    return 0;
}

/* NUMBER: an integer too big for INTEGER, from K on (ISUM the digits before) */
void LONGNUMBER(INTEGER IPART, INTEGER ENDI, INTEGER K, INTEGER ISUM)
{
    INTEGER J;
    CSP LVP;
    if (ENDI - IPART >= MAXDEC) {
        ERROR(203);
        IPART = ENDI;
        K = ENDI;
    }
    LVP = NEWCONST(L);        /* NEW(LVP, LONG) */
    LVP->CCLASS = LONG;
    J = 4;
    LVP->U.L.LLENG = 0;
    while (K <= ENDI) {
        if (J == 4) {
            LVP->U.L.LLENG = LVP->U.L.LLENG + 1;
            LVP->U.L.LONGVAL[LVP->U.L.LLENG] = ISUM;
            ISUM = 0;
            J = 0;
        }
        ISUM = ISUM * 10 + SYMBUFP[K] - '0';
        K = K + 1;
        J = J + 1;
    }
    LVP->U.L.LLAST = J;
    if (J > 0) {
        LVP->U.L.LLENG = LVP->U.L.LLENG + 1;
        LVP->U.L.LONGVAL[LVP->U.L.LLENG] = ISUM;
    }
    SY = LONGCONST;
    OP = NOOP;
    LGTH = ENDI - IPART + 1;
    VAL.VALP = LVP;
}

/* NUMBER: a REAL: its integer part, fraction and exponent */
void REALNUMBER(INTEGER IPART, INTEGER ENDI, INTEGER FPART, INTEGER ENDF,
                INTEGER EPART, INTEGER ENDE, INTEGER SIGN)
{
    INTEGER EXPONENT, J;
    REAL RSUM;
    CSP LVP;
    RSUM = RFLOAT(0);
    for (J = IPART; J <= ENDI; J++)
        RSUM = RADD(RMUL(RSUM, RFLOAT(10)), RFLOAT(SYMBUFP[J] - '0'));
    for (J = ENDF; J >= FPART; J--)
        RSUM = RADD(RSUM, RDIVR(RFLOAT(SYMBUFP[J] - '0'), RPOT(J - FPART + 1)));
    EXPONENT = 0;
    for (J = EPART; J <= ENDE; J++)
        EXPONENT = EXPONENT * 10 + SYMBUFP[J] - '0';
    if (SIGN == -1)
        RSUM = RDIVR(RSUM, RPOT(EXPONENT));
    else
        RSUM = RMUL(RSUM, RPOT(EXPONENT));
    SY = REALCONST;
    OP = NOOP;
    LVP = NEWCONST(RVAL);            /* NEW(LVP, REEL) */
    LVP->CCLASS = REEL;
    LVP->U.RVAL = RSUM;
    VAL.VALP = LVP;
}

/* the lines a minute, when it is more than 32767: two parts of at most 4 digits */
static void WRITERATE(REAL RATE)
{
    LGTH = RTRUNC(RDIVR(RATE, RFLOAT(10000)));
    STARTDOTS = RROUND(RSUB(RATE, RMUL(RFLOAT(LGTH), RFLOAT(10000))));
    if (STARTDOTS < 0) {
        LGTH = LGTH - 1;
        STARTDOTS = STARTDOTS + 10000;
    }
    if (STARTDOTS >= 10000) {
        LGTH = LGTH + 1;
        STARTDOTS = STARTDOTS - 10000;
    }
    WINT(NULL, LGTH, 0);
    if (STARTDOTS < 1000)
        WCH(NULL, '0');
    if (STARTDOTS < 100)
        WCH(NULL, '0');
    if (STARTDOTS < 10)
        WCH(NULL, '0');
    WINT(NULL, STARTDOTS, 0);
}

/* the rest of PASCALCOMPILER, after BLOCK */
void FINISHCOMPILE(void)
{
    REAL RATE;
    if (SY != PERIOD)
        ERROR(21);
    if (LIST) {
        SCREENDOTS = SCREENDOTS + 1;
        SYMBUFP[SYMCURSOR] = EOL;
        SYMCURSOR = SYMCURSOR + 1;
        PRINTLINE();
    }
    USERINFO.ERRBLK = 0;
    PTIME(&LGTH, &STARTDOTS);
    LOWTIME = STARTDOTS - LOWTIME;
    /* (UNITWRITE(3, IC, 7): to a graphics screen, unit 3 -- not here) */
    if (DLINKERINFO || CLINKERINFO) {
        SEGTABLE[SEG].SEGKIND = 1;
        WRITELINKERINFO(TRUE);
    }
    PCLOSE(LP, TRUE);
    if (NOISY)
        WLN(NULL);
    WINT(NULL, SCREENDOTS, 0);
    WSTR(NULL, " lines");
    if (LOWTIME > 0) {
        /* the elapsed time and the lines a minute (REAL arithmetic, written as
           integers: nothing that can exceed 32767 is ROUNDed or TRUNCed) */
        STARTDOTS = RROUND(RDIVR(RFLOAT(LOWTIME), RFLOAT(6)));      /* tenths of a second */
        WSTR(NULL, ", ");
        WINT(NULL, STARTDOTS / 10, 0);
        WSTR(NULL, ".");
        WINT(NULL, STARTDOTS % 10, 0);
        WSTR(NULL, " secs, ");
        RATE = RMUL(RDIVR(RFLOAT(3600), RFLOAT(LOWTIME)), RFLOAT(SCREENDOTS));
        if (RLT(RATE, RFLOAT(32767)))
            WINT(NULL, RROUND(RATE), 0);
        else
            WRITERATE(RATE);
        WSTR(NULL, " lines/min");
    }
    if (NOISY) {
        WLN(NULL);
        WSTR(NULL, "Smallest available space = ");
        WINT(NULL, SMALLESTSPACE, 0);
        WSTR(NULL, " words");
    }
    /* the segment dictionary */
    IC = 0;
    for (SEG = 0; SEG <= MAXSEG; SEG++) {
        GENWORD(SEGTABLE[SEG].DISKADDR);
        GENWORD(SEGTABLE[SEG].CODELENG);
    }
    for (SEG = 0; SEG <= MAXSEG; SEG++)
        for (LGTH = 1; LGTH <= 8; LGTH++)
            GENBYTE(SEGTABLE[SEG].SEGNAME[LGTH - 1]);
    for (SEG = 0; SEG <= MAXSEG; SEG++)
        GENWORD(SEGTABLE[SEG].SEGKIND);
    for (SEG = 0; SEG <= MAXSEG; SEG++)
        GENWORD(SEGTABLE[SEG].TEXTADDR);
    for (LGTH = 1; LGTH <= 80; LGTH++)
        if (COMMENT != NULL)
            GENBYTE(COMMENT[LGTH]);
        else
            GENBYTE(0);
    for (LGTH = 1; LGTH <= 256 - 8 * (MAXSEG + 1) - 40; LGTH++)
        GENWORD(0);
    CURBLK = 0;
    CURBYTE = 0;
    WRITECODE(TRUE);
}
