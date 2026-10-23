/* block.c -- BLOCK.TEXT: BLOCK, which compiles the declarations and bodies
   of a block's procedures in turn (the lex stack, TOS), and the body of the
   segment procedure PASCALCOMPILER: the compile summary and the code file's
   segment dictionary (block 0). */
#include "compglbl.h"
#include "pcio.h"

static void FINDFORW(CTP FCP)
{
    if (FCP != NULL) {
        if (FCP->KLASS == PROC || FCP->KLASS == FUNC)
            if (FCP->U.P.PFDECKIND == DECLARED)
                if (FCP->U.P.PFKIND == ACTUAL)
                    if (FCP->U.P.FORWDECL) {
                        USERINFO.ERRNUM = 117;
                        WLN(NULL);
                        WPA(NULL, FCP->NAME, 8);
                        WSTR(NULL, " undefined");
                    }
        FINDFORW(FCP->RLINK);
        FINDFORW(FCP->LLINK);
    }
}

void BLOCK(SETOFSYS *FSYS)
{
    BOOLEAN BFSYFOUND;
    struct LEXSTKREC *T;
    SETS s, u;
    if (NOSWAP && STARTINGUP) {
        BODYPART(FSYS, NULL);
        return;                                 /* EXIT(BLOCK) */
    }
    if ((SY == UNITSY || SY == SEPARATSY) && !INMODULE) {
        UNITPART(SETADD(&s, FSYS, UNITSY, INTERSY, IMPLESY, ENDSY, -1));
        if (SY == PERIOD)
            return;                             /* EXIT(BLOCK) */
    }
    NEWBLOCK = TRUE;
    do {
        if (!NEWBLOCK) {
            DP = FALSE;
            STMTLEV = 0;
            IC = 0;
            LINEINFO = 0;
            if (!SYSCOMP || LEVEL > 1)
                FINDFORW(DISPLAY[TOP].FNAME);
            if (INMODULE)
                if (TOS->PREVLEXSTACKP->DFPROCP == OUTERBLOCK) {
                    if (SY == ENDSY) {
                        FINISHSEG();
                        return;                 /* EXIT(BLOCK) */
                    } else if (SY == BEGINSY) {
                        ERROR(13);
                        FINISHSEG();
                        return;                 /* EXIT(BLOCK) */
                    }
                }
            if (SY == BEGINSY)
                INSYMBOL();
            else
                ERROR(17);
            do {
                BODYPART(SETMINUS(&s, SETADD(&s, FSYS, CASESY, -1), SETOF(&u, ENDSY, -1)), TOS->DFPROCP);
                BFSYFOUND = SY == TOS->BFSY || (INMODULE && SY == ENDSY);
                if (!BFSYFOUND) {
                    if (TOS->BFSY == SEMICOLON)
                        ERROR(14);              /* ; expected */
                    else
                        ERROR(6);               /* . expected */
                    SKIP(SETADD(&s, FSYS, TOS->BFSY, -1));
                    BFSYFOUND = SY == TOS->BFSY || (INMODULE && SY == ENDSY);
                }
            } while (!(BFSYFOUND || IN(SY, &BLOCKBEGSYS)));
            if (!BFSYFOUND) {
                if (TOS->BFSY == SEMICOLON)
                    ERROR(14);
                else
                    ERROR(6);                   /* . expected */
                DECLARATIONPART(FSYS);
            } else {
                if (SY == SEMICOLON)
                    INSYMBOL();
                if (!(SY == BEGINSY || SY == PROCSY || SY == FUNCSY || SY == PROGSY)
                    && TOS->BFSY == SEMICOLON) {
                    if (!(INMODULE && SY == ENDSY)) {
                        ERROR(6);
                        SKIP(FSYS);
                        DECLARATIONPART(FSYS);
                    } else
                        goto L1;
                } else {
                L1:
                    T = TOS;
                    if (T->DFPROCP != NULL)
                        T->DFPROCP->U.P.INSCOPE = FALSE;
                    if (T->ISSEGMENT) {
                        if (CODEINSEG)
                            FINISHSEG();
                        if (DLINKERINFO && LEVEL == 1) {
                            SEGTABLE[SEG].SEGKIND = 2;
                            WRITELINKERINFO(TRUE);
                        } else if (CLINKERINFO) {
                            SEGTABLE[SEG].SEGKIND = 2;
                            WRITELINKERINFO(FALSE);
                        }
                        NEXTPROC = T->SOLDPROC;
                        SEG = T->DOLDSEG;
                    }
                    LEVEL = T->DOLDLEV;
                    TOP = T->DOLDTOP;
                    LC = T->DLLC;
                    CURPROC = T->POLDPROC;
                    PRELEASE(TOS->DMARKP);
                    TOS = TOS->PREVLEXSTACKP;
                    NEWBLOCK = SY == PROCSY || SY == FUNCSY || SY == PROGSY;
                }
            }
        } else {
            DECLARATIONPART(FSYS);
            if (LEVEL == 0)
                if (SY == UNITSY || SY == SEPARATSY) {
                    UNITPART(SETADD(&s, FSYS, UNITSY, INTERSY, IMPLESY, ENDSY, -1));
                    if (SY == PROCSY || SY == FUNCSY || SY == PROGSY)
                        DECLARATIONPART(FSYS);
                }
        }
    } while (TOS != NULL);
    FINISHSEG();
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

void PASCALCOMPILER(void)
{
    SETS s, t;
    REAL RATE;
    COMPINIT();
    PTIME(&LGTH, &LOWTIME);
    BLOCK(SETMINUS(&s, SETPLUS(&s, &BLOCKBEGSYS, &STATBEGSYS), SETOF(&t, CASESY, -1)));
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
