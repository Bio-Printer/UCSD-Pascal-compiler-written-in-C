/* procs.c -- PROCS.A.TEXT and PROCS.B.TEXT: reading the source, the
   listing, the scanner (INSYMBOL), the symbol table, constants, type
   compatibility and the code buffer.  (ERROR, the options of a comment and
   the long and REAL constants are in rare.c.) */
#include "compglbl.h"
#include "pcio.h"


/* ======================== PROCS.A ======================== */

void GETNEXTPAGE(void)
{
    SYMCURSOR = 0;
    LINESTART = 0;
    if (USING) {
        if (USEFILE == WORKCODE) {
            if (PBLOCKREAD(USERINFO.WORKCODE, SYMBUFP, 2, SYMBLK) != 2)
                USING = FALSE;
        } else if (USEFILE == SYSLIBRARY)
            if (PBLOCKREAD(LIBRARY, SYMBUFP, 2, SYMBLK) != 2)
                USING = FALSE;
        if (!USING) {
            SYMBLK = PREVSYMBLK;
            SYMCURSOR = PREVSYMCURSOR;
            LINESTART = PREVLINESTART;
        }
    }
    if (!USING) {
        if (INCLUDING)
            if (PBLOCKREAD(INCLFILE, SYMBUFP, 2, SYMBLK) != 2) {
                PCLOSE(INCLFILE, FALSE);
                INCLUDING = FALSE;
                SYMBLK = OLDSYMBLK;
                SYMCURSOR = OLDSYMCURSOR;
                LINESTART = OLDLINESTART;
            }
    }
    if (!(INCLUDING || USING))
        if (PBLOCKREAD(USERINFO.WORKSYM, SYMBUFP, 2, SYMBLK) != 2)
            ERROR(401);
    if (SYMCURSOR == 0) {
        if (INMODULE)
            if (ININTERFACE && !USING)
                WRITETEXT();
        if (SYMBUFP[0] == 16)                           /* DLE */
            SYMCURSOR = 2;
    }
    SYMBLK = SYMBLK + 2;
}

void PRINTLINE(void)
{
    CHAR DORLEV, STARORC;
    INTEGER LENG;
    CHAR A[100];
    STARORC = ':';
    if (DP)
        DORLEV = 'D';
    else
        DORLEV = MOD(BEGSTMTLEV, 10) + '0';
    if (BPTONLINE)
        STARORC = '*';
    WINT(LP, SCREENDOTS, 6);
    WINT(LP, SEG, 4);
    WINT(LP, CURPROC, 5);
    WCH(LP, STARORC);
    WCH(LP, DORLEV);
    WINT(LP, LINEINFO, 6);
    WCH(LP, ' ');
    LENG = SYMCURSOR - LINESTART;
    if (LENG > 100)
        LENG = 100;
    MOVELEFT(&SYMBUFP[LINESTART], A, LENG);
    if (A[0] == 16) {                                   /* DLE */
        if (A[1] > ' ')
            while (A[1]-- > ' ')                        /* WRITE(LP, ' ':ORD(A[1])-ORD(' ')) */
                WCH(LP, ' ');
        LENG = LENG - 2;
        MOVELEFT(&A[2], A, LENG);
    }
    A[LENG - 1] = EOL;                                  /* (just to make sure) */
    WPA(LP, A, LENG);
    if (USERINFO.ERRBLK == SYMBLK && USERINFO.ERRSYM > LINESTART) {
        WSTR(LP, ">>>>>> Error # ");
        WINT(LP, USERINFO.ERRNUM, 0);
        WLN(LP);
    }
}

void ENTERID(CTP FCP)
{
    CTP LCP, LCP1;
    INTEGER I;
    LCP = DISPLAY[TOP].FNAME;
    if (LCP == NULL)
        DISPLAY[TOP].FNAME = FCP;
    else {
        I = TREESEARCH(LCP, &LCP1, FCP->NAME);
        while (I == 0) {
            ERROR(101);
            if (LCP1->RLINK == NULL)
                I = 1;
            else
                I = TREESEARCH(LCP1->RLINK, &LCP1, FCP->NAME);
        }
        if (I == 1)
            LCP1->RLINK = FCP;
        else
            LCP1->LLINK = FCP;
    }
    FCP->LLINK = NULL;
    FCP->RLINK = NULL;
}

/* ---- INSYMBOL and its procedures ---- */

static void CHECKEND(void)
{   /* checks for the end of the page */
    SCREENDOTS = SCREENDOTS + 1;
    SYMCURSOR = SYMCURSOR + 1;
    if (NOISY) {
        WCH(NULL, '.');
        if (MOD(SCREENDOTS - STARTDOTS, 50) == 0) {
            WLN(NULL);
            WCH(NULL, '<');
            WINT(NULL, SCREENDOTS, 4);
            WCH(NULL, '>');
        }
    }
    if (LIST)
        PRINTLINE();
    BPTONLINE = FALSE;
    if (SYMBUFP[SYMCURSOR] == 0)
        GETNEXTPAGE();
    else
        LINESTART = SYMCURSOR;
    if (SYMBUFP[SYMCURSOR] == 12)                       /* FF */
        SYMCURSOR = SYMCURSOR + 1;
    if (SYMBUFP[SYMCURSOR] == 16)                       /* DLE */
        SYMCURSOR = SYMCURSOR + 2;
    else {
        SYMCURSOR = SYMCURSOR + SCAN(80, TRUE, 9, &SYMBUFP[SYMCURSOR]);
        SYMCURSOR = SYMCURSOR + SCAN(80, TRUE, ' ', &SYMBUFP[SYMCURSOR]);
    }
    if (DP)
        LINEINFO = LC;
    else
        LINEINFO = IC;
}

void SCANSTRING(CHAR *FSTRG, INTEGER MAXLENG, CHAR STOPPER)
{
    INTEGER LENG;
    SYMCURSOR = SYMCURSOR + 2;
    LENG = SCAN(MAXLENG, FALSE, STOPPER, &SYMBUFP[SYMCURSOR]);
    FSTRG[0] = LENG;
    MOVELEFT(&SYMBUFP[SYMCURSOR], &FSTRG[1], LENG);
    SYMCURSOR = SYMCURSOR + LENG + 1;
}

/* the result: TRUE for EXIT(INSYMBOL) (an include file: INSYMBOL has been called
   for its first symbol) */
static BOOLEAN COMMENTER(CHAR STOPPER)
{
    INTEGER R;
    SYMCURSOR = SYMCURSOR + 1;                          /* the first character past the opener */
    if (SYMBUFP[SYMCURSOR] == '$')
        if (SYMBUFP[SYMCURSOR + 1] != STOPPER) {
            R = OPTIONS(STOPPER);
            if (R != 0)
                return R == 2;
        }
    SYMCURSOR = SYMCURSOR - 1;                          /* adjust */
    do {
        do {
            SYMCURSOR = SYMCURSOR + 1;
            while (SYMBUFP[SYMCURSOR] == EOL)
                CHECKEND();
        } while (SYMBUFP[SYMCURSOR] != STOPPER);
    } while (!(SYMBUFP[SYMCURSOR + 1] == ')' || STOPPER == '}'));
    SYMCURSOR = SYMCURSOR + 1;
    return FALSE;
}

static void STRING(void)
{
    CHAR T[256 + 1];                                    /* PACKED ARRAY [1..80] (the Pascal one stops past 80) */
    INTEGER TP;
    BOOLEAN DUPLE;
    DUPLE = FALSE;                                      /* '' is there */
    TP = 0;                                             /* the index into T */
    do {
        if (DUPLE)
            SYMCURSOR = SYMCURSOR + 1;
        do {
            SYMCURSOR = SYMCURSOR + 1;
            TP = TP + 1;
            if (SYMBUFP[SYMCURSOR] == EOL) {
                ERROR(202);
                CHECKEND();
                goto l1;
            }
            if (TP <= 256)
                T[TP] = SYMBUFP[SYMCURSOR];
        } while (SYMBUFP[SYMCURSOR] != '\'');
        DUPLE = TRUE;
    } while (SYMBUFP[SYMCURSOR + 1] == '\'');
l1:
    TP = TP - 1;                                        /* adjust */
    SY = STRINGCONST;
    OP = NOOP;
    LGTH = TP;
    if (TP == 1)                                        /* a single character constant */
        VAL.IVAL = T[1];
    else {
        SCONST->CCLASS = STRG;
        SCONST->U.S.SLGTH = TP;
        MOVELEFT(&T[1], &SCONST->U.S.SVAL[1], TP);
        VAL.VALP = SCONST;
    }
}

static void NUMBER(void)
{
    INTEGER ENDI, ENDF, ENDE, SIGN, IPART, FPART, EPART, ISUM;
    INTEGER TIPE;                                       /* 0 REALTIPE, 1 INTEGERTIPE, 2 a DOUBLE (1.5D0) */
    BOOLEAN NOTLONG;
    INTEGER K, J;
    /* takes a number and decides whether it's real or integer and converts it
       to the internal form */
    TIPE = 1;
    ENDI = 0;
    ENDF = 0;
    ENDE = 0;
    SIGN = 1;
    NOTLONG = TRUE;
    EPART = 9999;                                       /* out of reach */
    IPART = SYMCURSOR;                                  /* the integer part starts here */
    K = 0;
    FPART = 0;
    do
        SYMCURSOR = SYMCURSOR + 1;
    while (!(SYMBUFP[SYMCURSOR] < '0' || SYMBUFP[SYMCURSOR] > '9'));
    ENDI = SYMCURSOR - 1;                               /* the end of the integer part */
    if (SYMBUFP[SYMCURSOR] == '.')
        if (SYMBUFP[SYMCURSOR + 1] != '.') {            /* watch out for '..' */
            TIPE = 0;
            SYMCURSOR = SYMCURSOR + 1;
            FPART = SYMCURSOR;
            while (SYMBUFP[SYMCURSOR] >= '0' && SYMBUFP[SYMCURSOR] <= '9')
                SYMCURSOR = SYMCURSOR + 1;
            if (SYMCURSOR == FPART)
                ERROR(201);
            ENDF = SYMCURSOR - 1;
        }
    if (SYMBUFP[SYMCURSOR] == 'E' || SYMBUFP[SYMCURSOR] == 'D') {
        TIPE = SYMBUFP[SYMCURSOR] == 'D' ? 2 : 0;
        SYMCURSOR = SYMCURSOR + 1;
        if (SYMBUFP[SYMCURSOR] == '-') {
            SYMCURSOR = SYMCURSOR + 1;
            SIGN = -1;
        } else if (SYMBUFP[SYMCURSOR] == '+')
            SYMCURSOR = SYMCURSOR + 1;
        EPART = SYMCURSOR;
        while (SYMBUFP[SYMCURSOR] >= '0' && SYMBUFP[SYMCURSOR] <= '9')
            SYMCURSOR = SYMCURSOR + 1;
        ENDE = SYMCURSOR - 1;
        if (ENDE < EPART)
            ERROR(201);
    }
    if (TIPE == 1) {
        ISUM = 0;
        for (J = IPART; J <= ENDI; J++) {
            if (ISUM > MAXINT / 10 ||
                (ISUM == MAXINT / 10 && SYMBUFP[J] - '0' > MAXINT % 10)) {
                NOTLONG = FALSE;
                K = J;
                J = ENDI;
            } else
                ISUM = ISUM * 10 + (SYMBUFP[J] - '0');
        }
        if (NOTLONG) {
            SY = INTCONST;
            OP = NOOP;
            VAL.IVAL = ISUM;
        } else
            LONGNUMBER(IPART, ENDI, K, ISUM);
    } else                                              /* a real number */
        REALNUMBER(IPART, ENDI, FPART, ENDF, EPART, ENDE, SIGN, SYMCURSOR - 1, TIPE == 2);
    SYMCURSOR = SYMCURSOR - 1;                          /* adjust for posterity */
}

void INSYMBOL(void)
{   /* compiler version 3.4 06-NOV-76 */
    CHAR c;
    if (GETSTMTLEV) {
        BEGSTMTLEV = STMTLEV;
        GETSTMTLEV = FALSE;
    }
    OP = NOOP;
l1:
    SY = OTHERSY;                                       /* if no case is taken, blow up */
    c = SYMBUFP[SYMCURSOR];
    switch (c) {
    case '\'':
        STRING();
        break;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        NUMBER();
        break;
    case '{':
        if (COMMENTER('}'))
            return;
        goto l1;
    case '(':
        if (SYMBUFP[SYMCURSOR + 1] == '*') {
            SYMCURSOR = SYMCURSOR + 1;
            if (COMMENTER('*'))
                return;
            SYMCURSOR = SYMCURSOR + 1;
            goto l1;                                    /* another token */
        } else
            SY = LPARENT;
        break;
    case ')':
        SY = RPARENT;
        break;
    case ',':
        SY = COMMA;
        break;
    case ' ':
    case 9:
        SYMCURSOR = SYMCURSOR + 1;
        goto l1;
    case '.':
        if (SYMBUFP[SYMCURSOR + 1] == '.') {
            SYMCURSOR = SYMCURSOR + 1;
            SY = COLON;
        } else
            SY = PERIOD;
        break;
    case ':':
        if (SYMBUFP[SYMCURSOR + 1] == '=') {
            SYMCURSOR = SYMCURSOR + 1;
            SY = BECOMES;
        } else
            SY = COLON;
        break;
    case ';':
        SY = SEMICOLON;
        break;
    case '^':
        SY = ARROW;
        break;
    case '[':
        SY = LBRACK;
        break;
    case ']':
        SY = RBRACK;
        break;
    case '*':
        SY = MULOP;
        OP = MUL;
        break;
    case '+':
        SY = ADDOP;
        OP = PLUS;
        break;
    case '-':
        SY = ADDOP;
        OP = MINUS;
        break;
    case '/':
        SY = MULOP;
        OP = RDIV;
        break;
    case '<':
        SY = RELOP;
        OP = LTOP;
        switch (SYMBUFP[SYMCURSOR + 1]) {
        case '>':
            OP = NEOP;
            SYMCURSOR = SYMCURSOR + 1;
            break;
        case '=':
            OP = LEOP;
            SYMCURSOR = SYMCURSOR + 1;
            break;
        }
        break;
    case '=':
        SY = RELOP;
        OP = EQOP;
        break;
    case '>':
        SY = RELOP;
        if (SYMBUFP[SYMCURSOR + 1] == '=') {
            OP = GEOP;
            SYMCURSOR = SYMCURSOR + 1;
        } else
            OP = GTOP;
        break;
    default:
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
            IDSEARCH();                                 /* (magic proc) */
        break;
    }
    if (SY == OTHERSY) {
        if (SYMBUFP[SYMCURSOR] == EOL) {
            CHECKEND();
            GETSTMTLEV = TRUE;
            goto l1;
        } else
            ERROR(400);
    }
    SYMCURSOR = SYMCURSOR + 1;                          /* the next call is about the next token */
}

/* ======================== PROCS.B ======================== */

void SEARCHSECTION(CTP FCP, CTP *FCP1)
{
    if (FCP != NULL) {
        if (TREESEARCH(FCP, FCP1, ID) == 0)
            ;                                           /* (nada) */
        else
            *FCP1 = NULL;
    } else
        *FCP1 = NULL;
}

void SEARCHID(SETOFIDS *FIDCLS, CTP *FCP)
{
    CTP LCP;
    LCP = NULL;
    for (DISX = TOP; DISX >= 0; DISX--) {
        LCP = DISPLAY[DISX].FNAME;
        if (LCP != NULL) {
            if (TREESEARCH(LCP, &LCP, ID) == 0) {
                if (IN(LCP->KLASS, FIDCLS))
                    goto l1;
                else if (PRTERR)
                    ERROR(103);
                else
                    LCP = NULL;
            } else
                LCP = NULL;
        }
    }
    DISX = 0;                                           /* (the FOR's variable, left at its limit) */
    if (PRTERR) {
        ERROR(104);
        if (IN(TYPES, FIDCLS))
            LCP = UTYPPTR;
        else if (IN(ACTUALVARS, FIDCLS))
            LCP = UVARPTR;
        else if (IN(FIELD, FIDCLS))
            LCP = UFLDPTR;
        else if (IN(KONST, FIDCLS))
            LCP = UCSTPTR;
        else if (IN(PROC, FIDCLS))
            LCP = UPRCPTR;
        else
            LCP = UFCTPTR;
    }
l1:
    *FCP = LCP;
}

void GETBOUNDS(STP FSP, INTEGER *FMIN, INTEGER *FMAX)
{
    if (FSP->FORM == SUBRANGE) {
        *FMIN = FSP->U.SR.MIN.IVAL;
        *FMAX = FSP->U.SR.MAX.IVAL;
    } else {
        *FMIN = 0;
        if (FSP == CHARPTR)
            *FMAX = 255;
        else if (FSP->U.SC.FCONST != NULL)
            *FMAX = FSP->U.SC.FCONST->U.VALUES.IVAL;
        else
            *FMAX = 0;
    }
}

void SKIP(SETOFSYS *FSYS)
{
    while (!IN(SY, FSYS))
        INSYMBOL();
}

BOOLEAN PAOFCHAR(STP FSP)
{
    BOOLEAN r = FALSE;
    if (FSP != NULL)
        if (FSP->FORM == ARRAYS)
            r = FSP->U.AR.AISPACKD && FSP->U.AR.AELTYPE == CHARPTR;
    return r;
}

BOOLEAN STRGTYPE(STP FSP)
{
    BOOLEAN r = FALSE;
    if (PAOFCHAR(FSP))
        r = FSP->U.AR.AISSTRNG;
    return r;
}

INTEGER DECSIZE(INTEGER I)
{
    return DIV(I + 3, 4) + 1;                             /* (gross: the most space needed) */
}

void CONSTANT(SETOFSYS *FSYS, STP *FSP, VALU *FVALU)
{
    STP LSP;
    CTP LCP;
    INTEGER SIGN;                                       /* 0 NONE, 1 POS, 2 NEG */
    CSP LVP;
    SETS s;
    LSP = NULL;
    FVALU->IVAL = 0;
    if (!IN(SY, &CONSTBEGSYS)) {
        ERROR(50);
        SKIP(SETPLUS(&s, FSYS, &CONSTBEGSYS));
    }
    if (IN(SY, &CONSTBEGSYS)) {
        if (SY == STRINGCONST) {
            if (LGTH == 1)
                LSP = CHARPTR;
            else {
                LSP = NEWTYPE(AR);   /* NEW(LSP, ARRAYS, TRUE, TRUE) */
                *LSP = *STRGPTR;
                LSP->U.AR.MAXLENG = LGTH;
                LSP->U.AR.INXTYPE = NULL;
                LVP = PNEW(sizeof(struct CONSTREC));
                *LVP = *VAL.VALP;
                VAL.VALP = LVP;
            }
            *FVALU = VAL;
            INSYMBOL();
        } else {
            SIGN = 0;
            if (SY == ADDOP && (OP == PLUS || OP == MINUS)) {
                if (OP == PLUS)
                    SIGN = 1;
                else
                    SIGN = 2;
                INSYMBOL();
            }
            if (SY == IDENT) {
                SEARCHID(SETOF(&s, KONST, SETEND), &LCP);
                LSP = LCP->IDTYPE;
                *FVALU = LCP->U.VALUES;
                if (SIGN != 0) {
                    if (LSP == INTPTR) {
                        if (SIGN == 2)
                            FVALU->IVAL = -FVALU->IVAL;
                    } else if (LSP == REALPTR || LSP == DOUBLEPTR) {
                        if (SIGN == 2) {
                            LVP = NEWCONST(RD);      /* NEW(LVP, REEL), and its DOUBLE */
                            LVP->CCLASS = FVALU->VALP->CCLASS;
                            LVP->U.RD = FVALU->VALP->U.RD;
                            LVP->U.RVAL = RNEG(LVP->U.RVAL);
                            LVP->U.RD.D[3] = LVP->U.RD.D[3] ^ 0x8000;
                            FVALU->VALP = LVP;
                        }
                    } else if (COMPTYPES(LSP, LONGINTPTR)) {
                        if (SIGN == 2) {
                            LVP = NEWCONST(L);    /* NEW(LVP, LONG) */
                            LVP->CCLASS = LONG;
                            LVP->U.L.LONGVAL[1] = -FVALU->VALP->U.L.LONGVAL[1];
                            FVALU->VALP = LVP;
                        }
                    } else
                        ERROR(105);
                }
                INSYMBOL();
            } else if (SY == INTCONST) {
                if (SIGN == 2)
                    VAL.IVAL = -VAL.IVAL;
                LSP = INTPTR;
                *FVALU = VAL;
                INSYMBOL();
            } else if (SY == REALCONST) {
                if (SIGN == 2) {
                    VAL.VALP->U.RVAL = RNEG(VAL.VALP->U.RVAL);
                    VAL.VALP->U.RD.D[3] = VAL.VALP->U.RD.D[3] ^ 0x8000;
                }
                LSP = VAL.VALP->CCLASS == DREEL ? DOUBLEPTR : REALPTR;
                *FVALU = VAL;
                INSYMBOL();
            } else if (SY == LONGCONST) {
                if (SIGN == 2) {                        /* (only a negative one: as the Pascal does) */
                    VAL.VALP->U.L.LONGVAL[1] = -VAL.VALP->U.L.LONGVAL[1];
                    LSP = NEWTYPE0();   /* NEW(LSP, LONGINT) */
                    LSP->SIZE = DECSIZE(LGTH);
                    LSP->FORM = LONGINT;
                    *FVALU = VAL;
                    INSYMBOL();
                }
            } else {
                ERROR(106);
                SKIP(FSYS);
            }
        }
        if (!IN(SY, FSYS)) {
            ERROR(6);
            SKIP(FSYS);
        }
    }
    *FSP = LSP;
}

BOOLEAN COMPTYPES(STP FSP1, STP FSP2)
{
    CTP NXT1, NXT2;
    BOOLEAN COMP;
    TESTP LTESTP1, LTESTP2;
    if (FSP1 == FSP2)
        return TRUE;
    if (FSP1 == NULL || FSP2 == NULL)
        return TRUE;
    if (FSP1->FORM == FSP2->FORM) {
        switch (FSP1->FORM) {
        case SCALAR:
            return FALSE;
        case SUBRANGE:
            return COMPTYPES(FSP1->U.SR.RANGETYPE, FSP2->U.SR.RANGETYPE);
        case POINTER:
            COMP = FALSE;
            LTESTP1 = GLOBTESTP;
            LTESTP2 = GLOBTESTP;
            while (LTESTP1 != NULL) {
                if (LTESTP1->ELT1 == FSP1->U.ELTYPE && LTESTP1->ELT2 == FSP2->U.ELTYPE)
                    COMP = TRUE;
                LTESTP1 = LTESTP1->LASTTESTP;
            }
            if (!COMP) {
                LTESTP1 = PNEW(sizeof(struct TESTPOINTER));
                LTESTP1->ELT1 = FSP1->U.ELTYPE;
                LTESTP1->ELT2 = FSP2->U.ELTYPE;
                LTESTP1->LASTTESTP = GLOBTESTP;
                GLOBTESTP = LTESTP1;
                COMP = COMPTYPES(FSP1->U.ELTYPE, FSP2->U.ELTYPE);
            }
            GLOBTESTP = LTESTP2;
            return COMP;
        case LONGINT:
            return TRUE;
        case POWER:
            return COMPTYPES(FSP1->U.ELSET, FSP2->U.ELSET);
        case ARRAYS:
            COMP = COMPTYPES(FSP1->U.AR.AELTYPE, FSP2->U.AR.AELTYPE)
                   && FSP1->U.AR.AISPACKD == FSP2->U.AR.AISPACKD;
            if (COMP && FSP1->U.AR.AISPACKD)
                COMP = FSP1->U.AR.ELSPERWD == FSP2->U.AR.ELSPERWD
                       && FSP1->U.AR.ELWIDTH == FSP2->U.AR.ELWIDTH
                       && FSP1->U.AR.AISSTRNG == FSP2->U.AR.AISSTRNG;
            if (COMP && !STRGTYPE(FSP1))
                COMP = FSP1->SIZE == FSP2->SIZE;
            return COMP;
        case RECORDS:
            NXT1 = FSP1->U.RE.FSTFLD;
            NXT2 = FSP2->U.RE.FSTFLD;
            COMP = TRUE;
            while (NXT1 != NULL && NXT2 != NULL && COMP) {
                COMP = COMPTYPES(NXT1->IDTYPE, NXT2->IDTYPE);
                NXT1 = NXT1->NEXT;
                NXT2 = NXT2->NEXT;
            }
            return COMP && NXT1 == NULL && NXT2 == NULL
                   && FSP1->U.RE.RECVAR == NULL && FSP2->U.RE.RECVAR == NULL;
        case FILES:
            return COMPTYPES(FSP1->U.FILTYPE, FSP2->U.FILTYPE);
        }
        return FALSE;                                   /* (TAGFLD, VARIANT: no case) */
    }
    if (FSP1->FORM == SUBRANGE)
        return COMPTYPES(FSP1->U.SR.RANGETYPE, FSP2);
    if (FSP2->FORM == SUBRANGE)
        return COMPTYPES(FSP1, FSP2->U.SR.RANGETYPE);
    return FALSE;
}

void GENBYTE(INTEGER FBYTE)
{
    CODEP[IC] = FBYTE;
    IC = IC + 1;
}

void GENWORD(INTEGER FWORD)
{
    CHAR TEMP;
    if (IC & 1)
        IC = IC + 1;
    CODEP[IC] = FWORD & 255;                            /* MOVELEFT(FWORD, CODEP^[IC], 2): low byte first */
    CODEP[IC + 1] = HIBYTE(FWORD);
    if (FLIPBYTES) {
        TEMP = CODEP[IC];
        CODEP[IC] = CODEP[IC + 1];
        CODEP[IC + 1] = TEMP;
    }
    IC = IC + 2;
}

void WRITETEXT(void)
{
    MOVELEFT(&SYMBUFP[SYMCURSOR], &CODEP[0], 1024);
    if (USERINFO.ERRNUM == 0)
        if (PBLOCKWRITE(USERINFO.WORKCODE, &CODEP[0], 2, CURBLK) != 2)
            ERROR(402);
    CURBLK = CURBLK + 2;
}

void WRITECODE(BOOLEAN FORCEBUF)
{
    INTEGER CODEINX, LIC, I;
    CODEINX = 0;
    LIC = IC;
    do {
        I = 512 - CURBYTE;
        if (I > LIC)
            I = LIC;
        MOVELEFT(&CODEP[CODEINX], &DISKBUF[CURBYTE], I);
        CODEINX = CODEINX + I;
        CURBYTE = CURBYTE + I;
        if (CURBYTE == 512 || FORCEBUF) {
            if (USERINFO.ERRNUM == 0)
                if (PBLOCKWRITE(USERINFO.WORKCODE, DISKBUF, 1, CURBLK) != 1)
                    ERROR(402);
            CURBLK = CURBLK + 1;
            CURBYTE = 0;
        }
        LIC = LIC - I;
    } while (LIC != 0);
}

void FINISHSEG(void)
{
    INTEGER I;
    IC = 0;
    for (I = NEXTPROC - 1; I >= 1; I--)
        if (PROCTABLE[I] == 0)
            GENWORD(0);
        else
            GENWORD(SEGINX + IC - PROCTABLE[I]);
    GENBYTE(SEG);
    GENBYTE(NEXTPROC - 1);
    SEGTABLE[SEG].CODELENG = SEGINX + IC;
    WRITECODE(TRUE);
    SEGINX = 0;
    CODEINSEG = FALSE;
}
