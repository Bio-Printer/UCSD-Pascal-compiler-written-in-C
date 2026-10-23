/* compinit.c -- COMPINIT.TEXT: the standard types, names and procedures, the
   compiler's switches and sets, the banner, and the program heading. */
#include "compglbl.h"
#include "pcio.h"

#pragma segment COMPINIT

static STP newtype(void)
{
    return PNEW(sizeof(struct STRUCTURE));
}

static CTP newid(void)
{
    return PNEW(sizeof(struct IDENTIFIER));
}

static void setname(CTP cp, const char *name)
{
    memcpy(cp->NAME, name, 8);
}

static void ENTSTDTYPES(void)
{
    INTPTR = newtype();                         /* NEW(INTPTR, SCALAR, STANDARD) */
    INTPTR->SIZE = INTSIZE; INTPTR->FORM = SCALAR; INTPTR->U.SC.SCALKIND = STANDARD;
    REALPTR = newtype();
    REALPTR->SIZE = REALSIZE; REALPTR->FORM = SCALAR; REALPTR->U.SC.SCALKIND = STANDARD;
    LONGINTPTR = newtype();
    LONGINTPTR->SIZE = DECSIZE((BITSPERWD - 1) * 100 / 332 + 1); LONGINTPTR->FORM = LONGINT;
    CHARPTR = newtype();
    CHARPTR->SIZE = CHARSIZE; CHARPTR->FORM = SCALAR; CHARPTR->U.SC.SCALKIND = STANDARD;
    BOOLPTR = newtype();
    BOOLPTR->SIZE = BOOLSIZE; BOOLPTR->FORM = SCALAR; BOOLPTR->U.SC.SCALKIND = DECLARED;
    NILPTR = newtype();
    NILPTR->SIZE = PTRSIZE; NILPTR->FORM = POINTER; NILPTR->U.ELTYPE = NULL;
    TEXTPTR = newtype();
    TEXTPTR->SIZE = FILESIZE + CHARSIZE; TEXTPTR->FORM = FILES; TEXTPTR->U.FILTYPE = CHARPTR;
    INTRACTVPTR = newtype();
    INTRACTVPTR->SIZE = FILESIZE + CHARSIZE; INTRACTVPTR->FORM = FILES; INTRACTVPTR->U.FILTYPE = CHARPTR;
    STRGPTR = newtype();                        /* NEW(STRGPTR, ARRAYS, TRUE, TRUE) */
    STRGPTR->FORM = ARRAYS; STRGPTR->SIZE = (DEFSTRGLGTH + CHRSPERWD) / CHRSPERWD;
    STRGPTR->U.AR.AISPACKD = TRUE; STRGPTR->U.AR.AISSTRNG = TRUE; STRGPTR->U.AR.INXTYPE = INTPTR;
    STRGPTR->U.AR.ELWIDTH = BITSPERCHR; STRGPTR->U.AR.ELSPERWD = CHRSPERWD;
    STRGPTR->U.AR.AELTYPE = CHARPTR; STRGPTR->U.AR.MAXLENG = DEFSTRGLGTH;
}

static void ENTTYPE(const char *name, STP t)
{
    CTP cp = newid();
    setname(cp, name);
    cp->IDTYPE = t;
    cp->KLASS = TYPES;
    ENTERID(cp);
}

static CTP ENTFILE(const char *name, INTEGER addr)
{
    CTP cp = newid();                           /* NEW(CP, FORMALVARS, FALSE) */
    setname(cp, name);
    cp->IDTYPE = TEXTPTR;
    cp->KLASS = FORMALVARS;
    cp->U.V.VLEV = 0;
    cp->U.V.VADDR = addr;
    ENTERID(cp);
    return cp;
}

static void ENTSTDNAMES(void)
{
    CTP CP, CP1;
    INTEGER I;
    ENTTYPE("INTEGER ", INTPTR);
    ENTTYPE("REAL    ", REALPTR);
    ENTTYPE("CHAR    ", CHARPTR);
    ENTTYPE("BOOLEAN ", BOOLPTR);
    ENTTYPE("STRING  ", STRGPTR);
    ENTTYPE("TEXT    ", TEXTPTR);
    ENTTYPE("INTERACT", INTRACTVPTR);
    INPUTPTR = ENTFILE("INPUT   ", 2);
    OUTPUTPTR = ENTFILE("OUTPUT  ", 3);
    ENTFILE("KEYBOARD", 4);
    CP1 = NULL;
    for (I = 0; I <= 1; I++) {
        CP = newid();                           /* NEW(CP, KONST) */
        CP->IDTYPE = BOOLPTR;
        if (I == 0)
            setname(CP, "FALSE   ");
        else
            setname(CP, "TRUE    ");
        CP->NEXT = CP1;
        CP->U.VALUES.IVAL = I;
        CP->KLASS = KONST;
        ENTERID(CP);
        CP1 = CP;
    }
    BOOLPTR->U.SC.FCONST = CP;
    CP = newid();
    setname(CP, "NIL     ");
    CP->IDTYPE = NILPTR;
    CP->NEXT = NULL;
    CP->U.VALUES.IVAL = 0;
    CP->KLASS = KONST;
    ENTERID(CP);
    CP = newid();
    setname(CP, "MAXINT  ");
    CP->IDTYPE = INTPTR;
    CP->KLASS = KONST;
    CP->U.VALUES.IVAL = MAXINT;
    ENTERID(CP);
}

static CTP undecl(IDCLASS klass)
{
    CTP cp = newid();
    setname(cp, "        ");
    cp->IDTYPE = NULL;
    cp->NEXT = NULL;
    cp->KLASS = klass;
    return cp;
}

static void ENTUNDECL(void)
{
    UTYPPTR = undecl(TYPES);
    UCSTPTR = undecl(KONST);
    UCSTPTR->U.VALUES.IVAL = 0;
    UVARPTR = undecl(ACTUALVARS);
    UVARPTR->U.V.VLEV = 0;
    UVARPTR->U.V.VADDR = 0;
    UFLDPTR = undecl(FIELD);
    UFLDPTR->U.F.FLDADDR = 0;
    UPRCPTR = undecl(PROC);
    UFCTPTR = undecl(FUNC);
    UPRCPTR->U.P.FORWDECL = UFCTPTR->U.P.FORWDECL = FALSE;
    UPRCPTR->U.P.INSCOPE = UFCTPTR->U.P.INSCOPE = FALSE;
    UPRCPTR->U.P.LOCALLC = UFCTPTR->U.P.LOCALLC = 0;
    UPRCPTR->U.P.EXTURNAL = UFCTPTR->U.P.EXTURNAL = FALSE;
    UPRCPTR->U.P.PFLEV = UFCTPTR->U.P.PFLEV = 0;
    UPRCPTR->U.P.PFNAME = UFCTPTR->U.P.PFNAME = 0;
    UPRCPTR->U.P.PFSEG = UFCTPTR->U.P.PFSEG = 0;
    UPRCPTR->U.P.PFDECKIND = UFCTPTR->U.P.PFDECKIND = DECLARED;
    UPRCPTR->U.P.PFKIND = UFCTPTR->U.P.PFKIND = ACTUAL;
}

static void ENTSPCPROCS(void)
{
    static const char NA[43][9] = {
        "READ    ", "READLN  ", "WRITE   ", "WRITELN ", "EOF     ", "EOLN    ",
        "PRED    ", "SUCC    ", "ORD     ", "SQR     ", "ABS     ", "NEW     ",
        "UNITREAD", "UNITWRIT", "CONCAT  ", "LENGTH  ", "INSERT  ", "DELETE  ",
        "COPY    ", "POS     ", "MOVELEFT", "MOVERIGH", "EXIT    ", "IDSEARCH",
        "TREESEAR", "TIME    ", "FILLCHAR", "OPENNEW ", "OPENOLD ", "REWRITE ",
        "CLOSE   ", "SEEK    ", "RESET   ", "GET     ", "PUT     ", "SCAN    ",
        "BLOCKREA", "BLOCKWRI", "TRUNC   ", "PAGE    ", "SIZEOF  ", "STR     ",
        "GOTOXY  " };
    SETS tinyset, funcs;
    CTP LCP;
    INTEGER I;
    BOOLEAN ISFUNC;
    SETOF(&tinyset, 2, 7, 8, 10, 13, 17, 18, 19, 20, 32, 34, 35, 40, 42, 43, -1);
    SETOF(&funcs, 5, 6, 7, 8, 9, 10, 11, 15, 16, 19, 20, 25, 36, 37, 38, 39, 41, -1);
    for (I = 1; I <= 43; I++) {
        if (TINY)
            if (IN(I, &tinyset))
                continue;                       /* GOTO 1 */
        ISFUNC = IN(I, &funcs);
        LCP = newid();                          /* NEW(LCP, FUNC/PROC, SPECIAL) */
        memcpy(LCP->NAME, NA[I - 1], 8);
        LCP->NEXT = NULL;
        LCP->IDTYPE = NULL;
        LCP->KLASS = ISFUNC ? FUNC : PROC;
        LCP->U.P.PFDECKIND = SPECIAL;
        LCP->U.P.KEY = I;
        ENTERID(LCP);
    }
}

static void ENTSTDPROCS(void)
{
    static const char NA[19][9] = {
        "ODD     ", "CHR     ", "MEMAVAIL", "ROUND   ", "SIN     ", "COS     ",
        "LOG     ", "ATAN    ", "LN      ", "EXP     ", "SQRT    ", "MARK    ",
        "RELEASE ", "IORESULT", "UNITBUSY", "PWROFTEN", "UNITWAIT", "UNITCLEA",
        "HALT    " };
    SETS procs;
    CTP LCP, PARAM;
    STP LSP, FTYPE;
    INTEGER I;
    BOOLEAN ISPROC;
    SETOF(&procs, 12, 13, 17, 18, 19, -1);
    PARAM = NULL;
    FTYPE = NULL;
    for (I = 1; I <= 19; I++) {
        ISPROC = IN(I, &procs);
        switch (I) {                            /* the parameter and the type (cases not listed: as before) */
        case 1:
            FTYPE = BOOLPTR;
            PARAM = newid();                    /* NEW(PARAM, ACTUALVARS, FALSE) */
            PARAM->IDTYPE = INTPTR;
            PARAM->KLASS = ACTUALVARS;
            break;
        case 2:
            FTYPE = CHARPTR;
            break;
        case 3:
            FTYPE = INTPTR;
            PARAM = NULL;
            break;
        case 4:
            FTYPE = INTPTR;
            PARAM = newid();
            PARAM->IDTYPE = REALPTR;
            PARAM->KLASS = ACTUALVARS;
            break;
        case 5:
            FTYPE = REALPTR;
            break;
        case 12:
            FTYPE = NULL;
            PARAM = newid();                    /* NEW(PARAM, FORMALVARS, FALSE) */
            LSP = newtype();                    /* NEW(LSP, POINTER) */
            LSP->SIZE = PTRSIZE;
            LSP->FORM = POINTER;
            LSP->U.ELTYPE = NULL;
            PARAM->IDTYPE = LSP;
            PARAM->KLASS = FORMALVARS;
            break;
        case 14:
            FTYPE = INTPTR;
            PARAM = NULL;
            break;
        case 15:
            FTYPE = BOOLPTR;
            PARAM = newid();
            PARAM->IDTYPE = INTPTR;
            PARAM->KLASS = ACTUALVARS;
            break;
        case 16:
            FTYPE = REALPTR;
            break;
        case 17:
            FTYPE = NULL;
            break;
        case 19:
            FTYPE = NULL;
            PARAM = NULL;
            break;
        }
        LCP = newid();                          /* NEW(LCP, PROC/FUNC, STANDARD) */
        memcpy(LCP->NAME, NA[I - 1], 8);
        LCP->U.P.PFDECKIND = STANDARD;
        LCP->U.P.CSPNUM = I + 20;
        LCP->KLASS = ISPROC ? PROC : FUNC;
        if (PARAM != NULL)
            PARAM->NEXT = NULL;
        LCP->IDTYPE = FTYPE;
        LCP->NEXT = PARAM;
        ENTERID(LCP);
    }
}

static void INITSCALARS(void)
{
    INTEGER I;
    if (MEMAVAIL() > 9950)                      /* (an empirical value for a 50K byte machine) */
        NOSWAP = TRUE;
    else
        NOSWAP = FALSE;
    FWPTR = NULL;
    MODPTR = NULL;
    GLOBTESTP = NULL;
    LINESTART = 0;
    LINEINFO = LCAFTERMARKSTACK;
    LIST = FALSE;
    SYMBLK = 2;
    SCREENDOTS = 0;
    STARTDOTS = 0;
    for (SEG = 0; SEG <= MAXSEG; SEG++) {
        SEGTABLE[SEG].DISKADDR = 0;
        SEGTABLE[SEG].CODELENG = 0;
        memcpy(SEGTABLE[SEG].SEGNAME, "        ", 8);
        SEGTABLE[SEG].SEGKIND = 0;
        SEGTABLE[SEG].TEXTADDR = 0;
    }
    USINGLIST = NULL;
    if (USERINFO.STUPID)
        memcpy(SYSTEMLIB, "\016*SYSTEM.PASCAL", 15);
    else
        memcpy(SYSTEMLIB, "\017*SYSTEM.LIBRARY", 16);
    LC = LCAFTERMARKSTACK;
    IOCHECK = TRUE;
    DP = TRUE;
    SEGINX = 0;
    NEXTJTAB = 1;
    NEXTPROC = 2;
    CURPROC = 1;
    SCONST = PNEW(sizeof(struct CONSTREC));
    SYMBUFP = PNEW(sizeof(SYMBUFARRAY));
    CODEP = PNEW(sizeof(CODEARRAY));
    CLINKERINFO = FALSE;
    DLINKERINFO = FALSE;
    SEG = 1;
    NEXTSEG = 10;
    CURBLK = 1;
    CURBYTE = 0;
    LSEPPROC = FALSE;
    STARTINGUP = TRUE;
    NOISY = !USERINFO.SLOWTERM;
    SEPPROC = FALSE;
    DEBUGGING = FALSE;
    BPTONLINE = FALSE;
    INMODULE = FALSE;
    GOTOOK = FALSE;
    RANGECHECK = TRUE;
    SYSCOMP = FALSE;
    TINY = FALSE;
    CODEINSEG = FALSE;
    PRTERR = TRUE;
    INCLUDING = FALSE;
    USING = FALSE;
    for (I = SEEK; I <= DECOPS; I++)
        PFNUMOF[I] = 0;
    COMMENT = NULL;
    LIBNOTOPEN = TRUE;
    GETSTMTLEV = TRUE;
    BEGSTMTLEV = 0;
    FLIPBYTES = FALSE;
}

static void INITSETS(void)
{
    SETOF(&CONSTBEGSYS, ADDOP, INTCONST, REALCONST, STRINGCONST, IDENT, -1);
    SETADD(&SIMPTYPEBEGSYS, &CONSTBEGSYS, LPARENT, -1);
    SETADD(&TYPEBEGSYS, &SIMPTYPEBEGSYS, ARROW, PACKEDSY, ARRAYSY, RECORDSY, SETSY, FILESY, -1);
    SETOF(&TYPEDELS, ARRAYSY, RECORDSY, SETSY, FILESY, -1);
    SETOF(&BLOCKBEGSYS, USESSY, LABELSY, CONSTSY, TYPESY, VARSY, PROCSY, FUNCSY, PROGSY, BEGINSY, -1);
    SETOF(&SELECTSYS, ARROW, PERIOD, LBRACK, -1);
    SETOF(&FACBEGSYS, INTCONST, REALCONST, LONGCONST, STRINGCONST, IDENT, LPARENT, LBRACK, NOTSY, -1);
    SETOF(&STATBEGSYS, BEGINSY, GOTOSY, IFSY, WHILESY, REPEATSY, FORSY, WITHSY, CASESY, -1);
    SETOF(&VARS, FORMALVARS, ACTUALVARS, -1);
}

void COMPINIT(void)
{
    SETS s, t;
    INITSCALARS();
    INITSETS();
    LEVEL = 0;
    TOP = 0;
    if (NOISY) {
        for (IC = 1; IC <= 7; IC++)
            WLN(NULL);
        WSTR(NULL, "PASCAL Compiler [II.0.A.1]   EDN Built! ");
        WLN(NULL);
        WSTR(NULL, "<edn0>");
    }
    DISPLAY[0].FNAME = NULL;
    DISPLAY[0].FFILE = NULL;
    DISPLAY[0].FLABEL = NULL;
    DISPLAY[0].OCCUR = BLCK;
    SMALLESTSPACE = MEMAVAIL();
    GETNEXTPAGE();
    INSYMBOL();
    ENTSTDTYPES();
    ENTSTDNAMES();
    ENTUNDECL();
    ENTSPCPROCS();
    ENTSTDPROCS();
    if (SYSCOMP) {
        OUTERBLOCK = NULL;
        SEG = 0;
        NEXTSEG = 1;
        GLEV = 1;
        SETADD(&BLOCKBEGSYS, &BLOCKBEGSYS, UNITSY, SEPARATSY, -1);
    } else {
        TOP = 1;
        LEVEL = 1;
        DISPLAY[1].FNAME = NULL;
        DISPLAY[1].FFILE = NULL;
        DISPLAY[1].FLABEL = NULL;
        DISPLAY[1].OCCUR = BLCK;
        LC = LC + 2;
        GLEV = 3;                               /* (keep the stack straight for now) */
        OUTERBLOCK = newid();                   /* NEW(OUTERBLOCK, PROC, DECLARED, ACTUAL, FALSE) */
        OUTERBLOCK->NEXT = NULL;
        OUTERBLOCK->U.P.LOCALLC = LC;
        memcpy(OUTERBLOCK->NAME, "PROGRAM ", 8);
        OUTERBLOCK->IDTYPE = NULL;
        OUTERBLOCK->KLASS = PROC;
        OUTERBLOCK->U.P.PFDECKIND = DECLARED;
        OUTERBLOCK->U.P.PFLEV = 0;
        OUTERBLOCK->U.P.PFNAME = 1;
        OUTERBLOCK->U.P.PFSEG = SEG;
        OUTERBLOCK->U.P.PFKIND = ACTUAL;
        OUTERBLOCK->U.P.FORWDECL = FALSE;
        OUTERBLOCK->U.P.EXTURNAL = FALSE;
        OUTERBLOCK->U.P.INSCOPE = TRUE;
    }
    if (SY == PROGSY) {
        INSYMBOL();
        if (SY == IDENT) {
            memcpy(SEGTABLE[SEG].SEGNAME, ID, 8);
            if (OUTERBLOCK != NULL) {
                memcpy(OUTERBLOCK->NAME, ID, 8);
                ENTERID(OUTERBLOCK);            /* (allows EXIT on the program's name) */
            }
        } else
            ERROR(2);
        INSYMBOL();
        if (SY == LPARENT) {
            do
                INSYMBOL();
            while (!IN(SY, SETPLUS(&t, SETOF(&s, RPARENT, SEMICOLON, -1), &BLOCKBEGSYS)));
            if (SY == RPARENT)
                INSYMBOL();
            else
                ERROR(4);
        }
        if (SY == SEMICOLON)
            INSYMBOL();
        else
            ERROR(14);
    }
    PMARK(&MARKP);
    TOS = PNEW(sizeof(struct LEXSTKREC));       /* the LEXSTKREC for the outer block */
    TOS->PREVLEXSTACKP = NULL;
    TOS->BFSY = PERIOD;
    TOS->DFPROCP = OUTERBLOCK;
    TOS->DLLC = LC;
    TOS->DOLDLEV = LEVEL;
    TOS->DOLDTOP = TOP;
    TOS->POLDPROC = CURPROC;
    TOS->ISSEGMENT = FALSE;
    TOS->DMARKP = MARKP;
}
