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
        UNITPART(SETADD(&s, FSYS, UNITSY, INTERSY, IMPLESY, ENDSY, SETEND));
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
                BODYPART(SETMINUS(&s, SETADD(&s, FSYS, CASESY, SETEND), SETOF(&u, ENDSY, SETEND)), TOS->DFPROCP);
                BFSYFOUND = SY == TOS->BFSY || (INMODULE && SY == ENDSY);
                if (!BFSYFOUND) {
                    if (TOS->BFSY == SEMICOLON)
                        ERROR(14);              /* ; expected */
                    else
                        ERROR(6);               /* . expected */
                    SKIP(SETADD(&s, FSYS, TOS->BFSY, SETEND));
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
                    UNITPART(SETADD(&s, FSYS, UNITSY, INTERSY, IMPLESY, ENDSY, SETEND));
                    if (SY == PROCSY || SY == FUNCSY || SY == PROGSY)
                        DECLARATIONPART(FSYS);
                }
        }
    } while (TOS != NULL);
    FINISHSEG();
}

void PASCALCOMPILER(void)
{
    SETS s, t;
    COMPINIT();
    PTIME(&LGTH, &LOWTIME);
    BLOCK(SETMINUS(&s, SETPLUS(&s, &BLOCKBEGSYS, &STATBEGSYS), SETOF(&t, CASESY, SETEND)));
    FINISHCOMPILE();
}
