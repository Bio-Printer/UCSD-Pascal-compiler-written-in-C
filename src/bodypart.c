/* bodypart.c -- BODYPART.A.TEXT .. BODYPART.E.TEXT: the segment procedure
   BODYPART -- the code generator (GEN0 .. PUTLABEL, LOAD, STORE, SELECTOR),
   the standard procedures (CALL), expressions and statements, and BODY,
   which compiles a procedure's body and writes its code.

   CALL's nested procedures use CALL's FSYS, FCP, LKEY and WASLPARENT, and
   CALL recurses (through EXPRESSION), so those are a struct (CALLCTX) whose
   address its nested procedures get.  STATEMENT's are passed as parameters. */
#include "compglbl.h"
#include "pcio.h"

#pragma segment BODYPART

#define ABS(x) ((x) < 0 ? -(x) : (x))
#define ODD(x) ((x) & 1)

/* ======================== BODYPART.A: the code generator ======================== */

static void LINKERREF(IDCLASS KLASS, INTEGER FID, INTEGER ADDR)
{
    if (NREFS > REFSPERBLK) {                   /* write the buffer */
        if (PBLOCKWRITE(REFFILE, &(*REFLIST)[1], 1, REFBLK) != 1)
            ERROR(402);
        REFBLK = REFBLK + 1;
        NREFS = 1;
    }
    if (IN(KLASS, &VARS))
        (*REFLIST)[NREFS].KEY = FID + 32;
    else                                        /* PROC */
        (*REFLIST)[NREFS].KEY = FID;
    (*REFLIST)[NREFS].OFFSET = SEGINX + ADDR;
    NREFS = NREFS + 1;
}

static void GEN0(OPRANGE FOP)
{
    INTEGER I;
    BOOLEAN ODDIC;
    CSP P;
    if (FOP != 38)                              /* LCA */
        GENBYTE(FOP + 128);
    else {
        ODDIC = ODD(IC);
        STRGCSTIC = IC;
        if (!ODDIC)
            GENBYTE(215);                       /* NOP */
        GENBYTE(166);                           /* LCA */
        P = GATTR.CVAL.VALP;
        GENBYTE(P->U.S.SLGTH);
        for (I = 1; I <= P->U.S.SLGTH; I++)
            GENBYTE(P->U.S.SVAL[I]);
        if (ODDIC)
            GENBYTE(215);                       /* NOP */
    }
}

static void GENLDC(INTEGER IVAL)
{
    if (IVAL >= 0 && IVAL <= 127)
        GENBYTE(IVAL);
    else {
        GENBYTE(51 + 148);                      /* LDCI */
        GENBYTE(MOD(ABS(IVAL), 256));
        GENBYTE(DIV(ABS(IVAL), 256));
        if (IVAL < 0)
            GEN0(17);                           /* NGI */
    }
}

static void GENBIG(INTEGER IVAL)
{
    if (IVAL <= 127)
        GENBYTE(IVAL);
    else {
        GENBYTE(128 + DIV(IVAL, 256));
        GENBYTE(MOD(IVAL, 256));
    }
}

static void GEN1(OPRANGE FOP, INTEGER FP2)
{
    INTEGER I, J;
    GENBYTE(FOP + 128);
    if (FOP == 51) {                            /* LDC */
        if (FP2 == 2)
            I = REALSIZE;
        else {
            I = 8;
            while (I > 0)
                if (CSTVAL(GATTR.CVAL.VALP, I) != 0)
                    break;                      /* GOTO 1 */
                else
                    I = I - 1;
        }
        GATTR.TYPTR->SIZE = I;
        if (I > 1) {
            GENBYTE(I);
            for (J = I; J >= 1; J--)
                GENWORD(CSTVAL(GATTR.CVAL.VALP, J));
        } else {
            IC = IC - 1;
            if (I == 1)
                GENLDC(CSTVAL(GATTR.CVAL.VALP, 1));
        }
    } else if (FOP == 30 || FOP == 32 || FOP == 45     /* CSP ADJ RNP */
               || FOP == 46 || FOP == 60 || FOP == 61  /* CIP LDM STM */
               || FOP == 65 || FOP == 66 || FOP == 78  /* RBP CBP CLP */
               || FOP == 42 || FOP == 79)              /* SAS CGP */
        GENBYTE(FP2);
    else if (INMODULE && (FOP == 37 || FOP == 41 || FOP == 43)) {  /* LAO LDO SRO */
        LINKERREF(ACTUALVARS, FP2, IC);
        GENBYTE(128);
        GENBYTE(0);
    } else if ((FOP == 74 || FOP == 41) && FP2 <= 16) {  /* LDL LDO: SLDL, SLDO */
        IC = IC - 1;
        if (FOP == 41)
            GENBYTE(231 + FP2);
        else
            GENBYTE(215 + FP2);
    } else if (FOP == 35 && FP2 <= 7) {         /* IND: SIND */
        IC = IC - 1;
        GENBYTE(248 + FP2);
    } else
        GENBIG(FP2);
}

static void GEN2(OPRANGE FOP, INTEGER FP1, INTEGER FP2)
{
    if (FOP == 64 || FOP == 77) {               /* IXP CXP */
        GENBYTE(FOP + 128);
        GENBYTE(FP1);
        GENBYTE(FP2);
    } else if (FOP == 47 || FOP == 48 || FOP == 49     /* EQU GEQ GRT */
               || FOP == 52 || FOP == 53 || FOP == 55) {  /* LEQ LES NEQ */
        if (FP1 == 0)
            GEN0(FOP + 20);
        else {
            GEN1(FOP, FP1 + FP1);
            if (FP1 > 4)
                GENBIG(FP2);
        }
    } else {                                    /* LDA, LOD, STR */
        if (FP1 == 0)
            GEN1(FOP + 20, FP2);
        else {
            GENBYTE(FOP + 128);
            GENBYTE(FP1);
            GENBIG(FP2);
        }
    }
}

static void ASSIGN(NONRESIDENT EXTPROC)
{
    PROCTABLE[NEXTPROC] = 0;
    PFNUMOF[EXTPROC] = NEXTPROC;
    NEXTPROC = NEXTPROC + 1;
    if (NEXTPROC > MAXPROCNUM)
        ERROR(193);                             /* not enough room for this operation */
    CLINKERINFO = TRUE;
}

static void GENNR(NONRESIDENT EXTPROC)
{
    if (PFNUMOF[EXTPROC] == 0)
        ASSIGN(EXTPROC);
    if (SEPPROC) {
        GEN1(79, 0);                            /* CGP */
        LINKERREF(PROC, -PFNUMOF[EXTPROC], IC - 1);
    } else
        GEN1(79, PFNUMOF[EXTPROC]);             /* CGP */
}

static void GENJMP(OPRANGE FOP, LBP FLBP)
{
    INTEGER DISP;
    if (FLBP->DEFINED) {
        GENBYTE(FOP + 128);
        DISP = FLBP->OCCURIC - IC - 1;
        if (DISP >= 0 && DISP <= 127)
            GENBYTE(DISP);
        else {
            if (FLBP->JTABINX == 0) {
                FLBP->JTABINX = NEXTJTAB;
                if (NEXTJTAB == MAXJTAB)
                    ERROR(253);
                else
                    NEXTJTAB = NEXTJTAB + 1;
                JTAB[FLBP->JTABINX] = FLBP->OCCURIC;
            }
            GENBYTE(248 - FLBP->JTABINX - FLBP->JTABINX);
        }
    } else {
        /* MOVELEFT(REFLIST, CODEP^[IC], 2): the chain of references, in the code */
        CODEP[IC] = FLBP->REFLIST & 255;
        CODEP[IC + 1] = HIBYTE(FLBP->REFLIST);
        if (FOP == 57)                          /* UJP */
            DISP = IC + 4096;
        else
            DISP = IC;
        FLBP->REFLIST = DISP;
        IC = IC + 2;
    }
}

static void LOAD(void);

static void GENFJP(LBP FLBP)
{
    LOAD();
    if (GATTR.TYPTR != BOOLPTR)
        ERROR(135);
    GENJMP(33, FLBP);                           /* FJP */
}

static void GENLABEL(LBP *FLBP)
{
    *FLBP = PNEW(sizeof(struct CODELABEL));
    (*FLBP)->DEFINED = FALSE;
    (*FLBP)->REFLIST = MAXADDR;
}

static void PUTLABEL(LBP FLBP)
{
    INTEGER LREF;
    OPRANGE LOP;
    LREF = FLBP->REFLIST;
    FLBP->DEFINED = TRUE;
    FLBP->OCCURIC = IC;
    FLBP->JTABINX = 0;
    while (LREF < MAXADDR) {
        if (LREF >= 4096) {
            LREF = LREF - 4096;
            LOP = 57;                           /* UJP */
        } else
            LOP = 33;                           /* FJP */
        IC = LREF;
        LREF = CODEP[IC] + 256 * CODEP[IC + 1];     /* MOVELEFT(CODEP^[IC], LREF, 2) */
        GENJMP(LOP, FLBP);
    }
    IC = FLBP->OCCURIC;
}

static void LOAD(void)
{
    INTEGER J, M;
    CSP P;
    STP T;
    if (GATTR.TYPTR != NULL) {
        switch (GATTR.KIND) {
        case CST:
            if (GATTR.TYPTR->FORM == LONGINT) {
                P = GATTR.CVAL.VALP;
                M = 10000;
                GENLDC(P->U.L.LONGVAL[1]);
                GENLDC(18);                     /* DCVT */
                GENNR(DECOPS);
                for (J = 2; J <= P->U.L.LLENG; J++) {
                    if (J == P->U.L.LLENG)
                        M = RTRUNC(RPOT(P->U.L.LLAST));
                    GENLDC(M);
                    GENLDC(18);                 /* DCVT */
                    GENNR(DECOPS);
                    GENLDC(8);                  /* DMP */
                    GENNR(DECOPS);
                    GENLDC(P->U.L.LONGVAL[J]);
                    GENLDC(18);                 /* DCVT */
                    GENNR(DECOPS);
                    GENLDC(2);                  /* DAD */
                    GENNR(DECOPS);
                }
            } else if (GATTR.TYPTR->FORM == SCALAR && GATTR.TYPTR != REALPTR)
                GENLDC(GATTR.CVAL.IVAL);
            else if (GATTR.TYPTR == NILPTR)
                GEN0(31);                       /* LDCN */
            else if (GATTR.TYPTR == REALPTR)
                GEN1(51, 2);                    /* LDC */
            else
                GEN1(51, 5);                    /* LDC */
            break;
        case VARBL:
            switch (GATTR.ACCESS) {
            case DRCT:
                if (GATTR.VLEVEL == 1)
                    GEN1(41, GATTR.DPLMT);      /* LDO */
                else
                    GEN2(54, LEVEL - GATTR.VLEVEL, GATTR.DPLMT);  /* LOD */
                break;
            case INDRCT:
                GEN1(35, GATTR.IDPLMT);         /* IND */
                break;
            case PACKD:
                GEN0(58);                       /* LDP */
                break;
            case MULTI:
                GEN1(60, GATTR.TYPTR->SIZE);    /* LDM */
                break;
            case BYTE:
                GEN0(62);                       /* LDB */
                break;
            }
            break;
        case EXPR:
            break;
        }
        T = GATTR.TYPTR;
        if ((T->FORM == POWER || (T->FORM == LONGINT && GATTR.KIND != CST))
            && GATTR.KIND != EXPR)
            GENLDC(GATTR.TYPTR->SIZE);
        GATTR.KIND = EXPR;
    }
}

static void STORE(ATTR *FATTR)
{
    if (FATTR->TYPTR != NULL)
        switch (FATTR->ACCESS) {
        case DRCT:
            if (FATTR->VLEVEL == 1)
                GEN1(43, FATTR->DPLMT);         /* SRO */
            else
                GEN2(56, LEVEL - FATTR->VLEVEL, FATTR->DPLMT);  /* STR */
            break;
        case INDRCT:
            if (FATTR->IDPLMT != 0)
                ERROR(400);
            else
                GEN0(26);                       /* STO */
            break;
        case PACKD:
            GEN0(59);                           /* STP */
            break;
        case MULTI:
            GEN1(61, FATTR->TYPTR->SIZE);       /* STM */
            break;
        case BYTE:
            GEN0(63);                           /* STB */
            break;
        }
}

/* LOADADDRESS and BYTEADDRESS: the address of GATTR */
static void ADDRESSOF(void)
{
    switch (GATTR.KIND) {
    case CST:
        if (STRGTYPE(GATTR.TYPTR))
            GEN0(38);                           /* LCA */
        else
            ERROR(400);
        break;
    case VARBL:
        switch (GATTR.ACCESS) {
        case DRCT:
            if (GATTR.VLEVEL == 1)
                GEN1(37, GATTR.DPLMT);          /* LAO */
            else
                GEN2(50, LEVEL - GATTR.VLEVEL, GATTR.DPLMT);  /* LDA */
            break;
        case INDRCT:
            if (GATTR.IDPLMT != 0)
                GEN1(34, GATTR.IDPLMT);         /* INC */
            break;
        case PACKD:
            ERROR(103);
            break;
        }
        break;
    }
}

static void LOADADDRESS(void)
{
    if (GATTR.TYPTR != NULL) {
        ADDRESSOF();
        GATTR.KIND = VARBL;
        GATTR.ACCESS = INDRCT;
        GATTR.IDPLMT = 0;
    }
}

static void BYTEADDRESS(void)
{
    if (GATTR.TYPTR != NULL) {
        ADDRESSOF();
        if (GATTR.KIND != VARBL) {
            GATTR.KIND = VARBL;
            GENLDC(0);
        } else if (GATTR.ACCESS != BYTE)
            GENLDC(0);
        GATTR.ACCESS = BYTE;
    }
}

static void STRGTOPA(ADDRRANGE FIC)
{
    if (ODD(FIC)) {
        MOVERIGHT(&CODEP[FIC + 1], &CODEP[FIC + 2], CODEP[FIC + 1] + 1);
        CODEP[FIC] = 215;                       /* NOP */
        CODEP[FIC + 1] = 208;                   /* LPA */
    } else {
        MOVELEFT(&CODEP[FIC + 2], &CODEP[FIC + 1], CODEP[FIC + 2] + 1);
        CODEP[FIC] = 208;
        CODEP[FIC + CODEP[FIC + 1] + 2] = 215;
    }
}

static void EXPRESSION(SETOFSYS *FSYS);

/* a selected type that is more than a word is reached through its address */
static void MULTIWORD(void)
{
    if (GATTR.TYPTR != NULL)
        if (GATTR.TYPTR->FORM <= POWER && GATTR.TYPTR->SIZE > PTRSIZE) {
            LOADADDRESS();
            GATTR.ACCESS = MULTI;
        }
}

static void SELECTOR(SETOFSYS *FSYS, CTP FCP)
{
    ATTR LATTR;
    CTP LCP;
    INTEGER LMIN, LMAX;
    struct DISPREC *D;
    STP A, T;
    SETS s, u;
    GATTR.TYPTR = FCP->IDTYPE;
    GATTR.KIND = VARBL;
    switch (FCP->KLASS) {
    case ACTUALVARS:
        GATTR.VLEVEL = FCP->U.V.VLEV;
        GATTR.DPLMT = FCP->U.V.VADDR;
        GATTR.ACCESS = DRCT;
        if (INMODULE)
            if (GATTR.TYPTR != NULL)
                if (FCP->U.V.VLEV == 1 && GATTR.TYPTR->FORM == RECORDS)
                    LOADADDRESS();
        break;
    case FORMALVARS:
        if (FCP->U.V.VLEV == 1)
            GEN1(41, FCP->U.V.VADDR);           /* LDO */
        else
            GEN2(54, LEVEL - FCP->U.V.VLEV, FCP->U.V.VADDR);  /* LOD */
        GATTR.ACCESS = INDRCT;
        GATTR.IDPLMT = 0;
        break;
    case FIELD:
        D = &DISPLAY[DISX];
        if (D->OCCUR == CREC) {
            GATTR.ACCESS = DRCT;
            GATTR.VLEVEL = D->CLEV;
            GATTR.DPLMT = D->CDSPL + FCP->U.F.FLDADDR;
        } else {
            if (LEVEL == 1)
                GEN1(41, D->VDSPL);             /* LDO */
            else
                GEN2(54, 0, D->VDSPL);          /* LOD */
            GATTR.ACCESS = INDRCT;
            GATTR.IDPLMT = FCP->U.F.FLDADDR;
        }
        if (FCP->U.F.FISPACKD) {
            LOADADDRESS();
            GATTR.ACCESS = PACKD;
            GENLDC(FCP->U.F.FLDWIDTH);
            GENLDC(FCP->U.F.FLDRBIT);
        }
        break;
    case FUNC:
        if (FCP->U.P.PFDECKIND != DECLARED)
            ERROR(150);
        else if (!FCP->U.P.INSCOPE)
            ERROR(103);
        else {
            GATTR.ACCESS = DRCT;
            GATTR.VLEVEL = FCP->U.P.PFLEV + 1;
            GATTR.DPLMT = LCAFTERMARKSTACK;
        }
        break;
    }
    MULTIWORD();
    if (!IN(SY, SETPLUS(&s, &SELECTSYS, FSYS))) {
        ERROR(59);
        SKIP(&s);
    }
    while (IN(SY, &SELECTSYS)) {
        if (SY == LBRACK) {                     /* [ */
            do {
                LATTR = GATTR;
                if (LATTR.TYPTR != NULL)
                    if (LATTR.TYPTR->FORM != ARRAYS) {
                        ERROR(138);
                        LATTR.TYPTR = NULL;
                    }
                LOADADDRESS();
                INSYMBOL();
                EXPRESSION(SETADD(&u, FSYS, COMMA, RBRACK, -1));
                LOAD();
                if (GATTR.TYPTR != NULL)
                    if (GATTR.TYPTR->FORM != SCALAR)
                        ERROR(113);
                if (LATTR.TYPTR != NULL) {
                    A = LATTR.TYPTR;            /* WITH LATTR.TYPTR^ */
                    if (COMPTYPES(A->U.AR.INXTYPE, GATTR.TYPTR)) {
                        if (A->U.AR.INXTYPE != NULL && !STRGTYPE(LATTR.TYPTR)) {
                            GETBOUNDS(A->U.AR.INXTYPE, &LMIN, &LMAX);
                            if (RANGECHECK) {
                                GENLDC(LMIN);
                                GENLDC(LMAX);
                                GEN0(8);        /* CHK */
                            }
                            if (LMIN != 0) {
                                GENLDC(ABS(LMIN));
                                if (LMIN > 0)
                                    GEN0(21);   /* SBI */
                                else
                                    GEN0(2);    /* ADI */
                            }
                        }
                    } else
                        ERROR(139);
                    GATTR.TYPTR = A->U.AR.AELTYPE;
                    GATTR.KIND = VARBL;
                    GATTR.ACCESS = INDRCT;
                    GATTR.IDPLMT = 0;
                    if (GATTR.TYPTR != NULL) {
                        if (A->U.AR.AISPACKD) {
                            if (A->U.AR.ELWIDTH == 8) {
                                GATTR.ACCESS = BYTE;
                                if (STRGTYPE(LATTR.TYPTR) && RANGECHECK)
                                    GEN0(27);   /* IXS */
                                /* (else leave the base-index pair) */
                            } else {
                                GATTR.ACCESS = PACKD;
                                GEN2(64, A->U.AR.ELSPERWD, A->U.AR.ELWIDTH);  /* IXP */
                            }
                        } else {
                            GEN1(36, GATTR.TYPTR->SIZE);    /* IXA */
                            if (GATTR.TYPTR->FORM <= POWER && GATTR.TYPTR->SIZE > PTRSIZE)
                                GATTR.ACCESS = MULTI;
                        }
                    }
                }
            } while (SY == COMMA);
            if (SY == RBRACK)
                INSYMBOL();
            else
                ERROR(12);
        } else if (SY == PERIOD) {              /* . */
            if (GATTR.TYPTR != NULL)
                if (GATTR.TYPTR->FORM != RECORDS) {
                    ERROR(140);
                    GATTR.TYPTR = NULL;
                }
            INSYMBOL();
            if (SY == IDENT) {
                if (GATTR.TYPTR != NULL) {
                    SEARCHSECTION(GATTR.TYPTR->U.RE.FSTFLD, &LCP);
                    if (LCP == NULL) {
                        ERROR(152);
                        GATTR.TYPTR = NULL;
                    } else {
                        GATTR.TYPTR = LCP->IDTYPE;
                        switch (GATTR.ACCESS) {
                        case DRCT:
                            GATTR.DPLMT = GATTR.DPLMT + LCP->U.F.FLDADDR;
                            break;
                        case INDRCT:
                            GATTR.IDPLMT = GATTR.IDPLMT + LCP->U.F.FLDADDR;
                            break;
                        case MULTI:
                        case BYTE:
                        case PACKD:
                            ERROR(400);
                            break;
                        }
                        if (LCP->U.F.FISPACKD) {
                            LOADADDRESS();
                            GATTR.ACCESS = PACKD;
                            GENLDC(LCP->U.F.FLDWIDTH);
                            GENLDC(LCP->U.F.FLDRBIT);
                        }
                        MULTIWORD();
                    }
                }
                INSYMBOL();
            } else
                ERROR(2);
        } else {                                /* ^ */
            if (GATTR.TYPTR != NULL) {
                T = GATTR.TYPTR;                /* WITH GATTR, TYPTR^ */
                if (T->FORM == POINTER || T->FORM == FILES) {
                    LOAD();
                    GATTR.KIND = VARBL;
                    GATTR.ACCESS = INDRCT;
                    GATTR.IDPLMT = 0;
                    if (T->FORM == POINTER)
                        GATTR.TYPTR = T->U.ELTYPE;
                    else {
                        GATTR.TYPTR = T->U.FILTYPE;
                        if (GATTR.TYPTR == NULL)
                            ERROR(399);
                    }
                    if (GATTR.TYPTR != NULL)
                        if (GATTR.TYPTR->FORM <= POWER && GATTR.TYPTR->SIZE > PTRSIZE)
                            GATTR.ACCESS = MULTI;
                } else
                    ERROR(141);
            }
            INSYMBOL();
        }
        if (!IN(SY, SETPLUS(&s, FSYS, &SELECTSYS))) {
            ERROR(6);
            SKIP(&s);
        }
    }
}

/* ======================== BODYPART.B: the standard procedures ======================== */

struct CALLCTX {                        /* CALL's */
    SETOFSYS *FSYS;
    CTP FCP;
    INTEGER LKEY;
    BOOLEAN WASLPARENT;
};

static void VARIABLE(SETOFSYS *FSYS)
{
    CTP LCP;
    SETS s;
    if (SY == IDENT) {
        SEARCHID(SETADD(&s, &VARS, FIELD, -1), &LCP);
        INSYMBOL();
    } else {
        ERROR(2);
        LCP = UVARPTR;
    }
    SELECTOR(FSYS, LCP);
}

static void STRGVAR(SETOFSYS *FSYS, BOOLEAN MUSTBEVAR)
{
    EXPRESSION(FSYS);
    if ((GATTR.KIND == CST && GATTR.TYPTR == CHARPTR) || STRGTYPE(GATTR.TYPTR)) {
        if (GATTR.KIND == VARBL)
            LOADADDRESS();
        else {
            if (MUSTBEVAR)
                ERROR(154);
            if (GATTR.KIND == CST) {
                if (GATTR.TYPTR == CHARPTR) {
                    SCONST->CCLASS = STRG;
                    SCONST->U.S.SLGTH = 1;
                    SCONST->U.S.SVAL[1] = GATTR.CVAL.IVAL;
                    GATTR.CVAL.VALP = SCONST;
                    GATTR.TYPTR = NEWTYPE(AR);   /* NEW(TYPTR, ARRAYS, TRUE, TRUE) */
                    *GATTR.TYPTR = *STRGPTR;
                    GATTR.TYPTR->U.AR.MAXLENG = 1;
                }
                LOADADDRESS();
            }
        }
    } else {
        if (GATTR.TYPTR != NULL)
            ERROR(125);
        GATTR.TYPTR = STRGPTR;
    }
}

static void NEWSTMT(struct CALLCTX *c)
{
    STP LSP, LSP1;
    INTEGER VARTS;
    ADDRRANGE LSIZE;
    VALU LVAL;
    SETS s;
    VARIABLE(SETADD(&s, c->FSYS, COMMA, RPARENT, -1));
    LOADADDRESS();
    LSP = NULL;
    VARTS = 0;
    LSIZE = 0;
    if (GATTR.TYPTR != NULL) {
        if (GATTR.TYPTR->FORM == POINTER) {
            if (GATTR.TYPTR->U.ELTYPE != NULL) {
                LSIZE = GATTR.TYPTR->U.ELTYPE->SIZE;
                if (GATTR.TYPTR->U.ELTYPE->FORM == RECORDS)
                    LSP = GATTR.TYPTR->U.ELTYPE->U.RE.RECVAR;
            }
        } else
            ERROR(116);
    }
    while (SY == COMMA) {
        INSYMBOL();
        CONSTANT(&s, &LSP1, &LVAL);
        VARTS = VARTS + 1;
        if (LSP == NULL)
            ERROR(158);
        else if (LSP->FORM != TAGFLD)
            ERROR(162);
        else if (LSP->U.TF.TAGFIELDP != NULL) {
            if (STRGTYPE(LSP1) || LSP1 == REALPTR)
                ERROR(159);
            else if (COMPTYPES(LSP->U.TF.TAGFIELDP->IDTYPE, LSP1)) {
                LSP1 = LSP->U.TF.FSTVAR;
                while (LSP1 != NULL)
                    if (LSP1->U.VA.VARVAL.IVAL == LVAL.IVAL) {
                        LSIZE = LSP1->SIZE;
                        LSP = LSP1->U.VA.SUBVAR;
                        goto L1;
                    } else
                        LSP1 = LSP1->U.VA.NXTVAR;
                LSIZE = LSP->SIZE;
                LSP = NULL;
            } else
                ERROR(116);
        }
    L1:;
    }
    GENLDC(LSIZE);
    GEN1(30, 1);                                /* CSP NEW */
}

static void MOVE(struct CALLCTX *c)
{
    SETS s;
    VARIABLE(SETADD(&s, c->FSYS, COMMA, -1));
    BYTEADDRESS();
    if (SY == COMMA)
        INSYMBOL();
    else
        ERROR(20);
    if (c->LKEY == 27) {
        EXPRESSION(&s);
        LOAD();
    } else {
        VARIABLE(&s);
        BYTEADDRESS();
    }
    if (SY == COMMA)
        INSYMBOL();
    else
        ERROR(20);
    EXPRESSION(SETADD(&s, c->FSYS, RPARENT, -1));
    LOAD();
    if (c->LKEY == 27)
        GEN1(30, 10);                           /* CSP FLC */
    else if (c->LKEY == 21)
        GEN1(30, 2);                            /* CSP MVL */
    else
        GEN1(30, 3);                            /* CSP MVR */
}

static void EXITSTMT(void)                      /* EXIT */
{
    CTP LCP;
    SETS s;
    if (SY == IDENT) {
        SEARCHID(SETOF(&s, PROC, FUNC, -1), &LCP);
        INSYMBOL();
    } else if (SY == PROGSY) {
        LCP = OUTERBLOCK;
        INSYMBOL();
    } else
        LCP = NULL;
    if (LCP != NULL) {
        if (LCP->U.P.PFDECKIND == DECLARED) {
            GENLDC(LCP->U.P.PFSEG);
            GENLDC(LCP->U.P.PFNAME);
            if (INMODULE) {
                LINKERREF(PROC, LCP->U.P.PFSEG, IC - 2);
                if (SEPPROC)
                    LINKERREF(PROC, -LCP->U.P.PFNAME, IC - 1);
            }
        } else
            ERROR(125);
    } else
        ERROR(125);
    GEN1(30, 4);                                /* CSP XIT */
}

static void UNITIO(struct CALLCTX *c)
{
    SETS s;
    if (GATTR.TYPTR != INTPTR)
        ERROR(125);
    if (SY == COMMA)
        INSYMBOL();
    else
        ERROR(20);
    VARIABLE(SETADD(&s, c->FSYS, COMMA, -1));
    BYTEADDRESS();
    if (SY == COMMA)
        INSYMBOL();
    else
        ERROR(20);
    EXPRESSION(SETADD(&s, c->FSYS, COMMA, RPARENT, -1));
    LOAD();
    if (GATTR.TYPTR != INTPTR)
        ERROR(125);
    if (SY == COMMA) {
        INSYMBOL();
        if (SY == COMMA)
            GENLDC(0);
        else {
            EXPRESSION(&s);
            LOAD();
            if (GATTR.TYPTR != INTPTR)
                ERROR(125);
        }
    } else
        GENLDC(0);
    if (SY == COMMA) {
        INSYMBOL();
        EXPRESSION(SETADD(&s, c->FSYS, RPARENT, -1));
        LOAD();
        if (GATTR.TYPTR != INTPTR)
            ERROR(125);
    } else
        GENLDC(0);
    if (c->LKEY == 13)
        GEN1(30, 5);                            /* CSP URD */
    else
        GEN1(30, 6);                            /* CSP UWT */
}

static void CONCAT(struct CALLCTX *c)
{
    ADDRRANGE LLC;
    INTEGER TEMPLGTH;
    SETS s;
    TEMPLGTH = 0;
    LLC = LC;
    LC = LC + STRGLGTH / CHRSPERWD + 1;
    GENLDC(0);
    GEN2(56, 0, LLC);                           /* STR */
    GEN2(50, 0, LLC);                           /* LDA */
    do {
        STRGVAR(SETADD(&s, c->FSYS, COMMA, RPARENT, -1), FALSE);
        TEMPLGTH = TEMPLGTH + GATTR.TYPTR->U.AR.MAXLENG;
        if (TEMPLGTH < STRGLGTH)
            GENLDC(TEMPLGTH);
        else
            GENLDC(STRGLGTH);
        GEN2(77, 0, 23);                        /* CXP SYS SCONCAT */
        GEN2(50, 0, LLC);                       /* LDA */
        TEST = SY != COMMA;
        if (!TEST)
            INSYMBOL();
    } while (!TEST);
    if (TEMPLGTH < STRGLGTH)
        LC = LLC + DIV(TEMPLGTH, CHRSPERWD) + 1;
    else
        TEMPLGTH = STRGLGTH;
    if (LC > LCMAX)
        LCMAX = LC;
    LC = LLC;
    GATTR.TYPTR = NEWTYPE(AR);   /* NEW(TYPTR, ARRAYS, TRUE, TRUE) */
    *GATTR.TYPTR = *STRGPTR;
    GATTR.TYPTR->U.AR.MAXLENG = TEMPLGTH;
}

static void COPYDELETE(struct CALLCTX *c)
{
    ADDRRANGE LLC;
    STP LSP;
    SETS s;
    if (c->LKEY == 19) {
        LLC = LC;
        LC = LC + STRGLGTH / CHRSPERWD + 1;
    }
    if (c->LKEY != 43) {
        STRGVAR(SETADD(&s, c->FSYS, COMMA, -1), c->LKEY == 18);
        if (c->LKEY == 19) {
            LSP = GATTR.TYPTR;
            GEN2(50, 0, LLC);                   /* LDA */
        }
        if (SY == COMMA)
            INSYMBOL();
        else
            ERROR(20);
    }
    EXPRESSION(SETADD(&s, c->FSYS, COMMA, -1));
    LOAD();
    if (GATTR.TYPTR != NULL)
        if (GATTR.TYPTR != INTPTR)
            ERROR(125);
    if (SY == COMMA)
        INSYMBOL();
    else
        ERROR(20);
    EXPRESSION(SETADD(&s, c->FSYS, RPARENT, -1));
    LOAD();
    if (GATTR.TYPTR != NULL)
        if (GATTR.TYPTR != INTPTR)
            ERROR(125);
    if (c->LKEY == 19) {
        GEN2(77, 0, 25);                        /* CXP SYS SCOPY */
        GEN2(50, 0, LLC);                       /* LDA */
        if (LSP->U.AR.MAXLENG < STRGLGTH)
            LC = LLC + DIV(LSP->U.AR.MAXLENG, CHRSPERWD) + 1;
        if (LC > LCMAX)
            LCMAX = LC;
        LC = LLC;
        GATTR.TYPTR = LSP;
    } else if (c->LKEY == 43)
        GEN2(77, 0, 29);                        /* CXP SYS GOTOXY */
    else
        GEN2(77, 0, 26);                        /* CXP SYS SDELETE */
}

static void STR(struct CALLCTX *c)
{
    SETS s;
    if (COMPTYPES(LONGINTPTR, GATTR.TYPTR))
        ;
    else if (GATTR.TYPTR == INTPTR) {
        GENLDC(18);                             /* DCVT */
        GENNR(DECOPS);
        GATTR.TYPTR = LONGINTPTR;
    } else
        ERROR(125);
    if (SY == COMMA)
        INSYMBOL();
    else
        ERROR(20);
    STRGVAR(SETADD(&s, c->FSYS, RPARENT, -1), TRUE);
    if (STRGTYPE(GATTR.TYPTR)) {
        GENLDC(GATTR.TYPTR->U.AR.MAXLENG);
        GENLDC(12);                             /* DSTR */
        GENNR(DECOPS);
    } else
        ERROR(116);
}

static void CLOSESTMT(struct CALLCTX *c)        /* CLOSE */
{
    SETS s;
    VARIABLE(SETADD(&s, c->FSYS, COMMA, RPARENT, -1));
    LOADADDRESS();
    if (GATTR.TYPTR != NULL)
        if (GATTR.TYPTR->FORM != FILES)
            ERROR(125);
    if (SY == COMMA) {
        INSYMBOL();
        if (SY == IDENT) {
            if (memcmp(ID, "NORMAL  ", 8) == 0)
                GENLDC(0);
            else if (memcmp(ID, "LOCK    ", 8) == 0)
                GENLDC(1);
            else if (memcmp(ID, "PURGE   ", 8) == 0)
                GENLDC(2);
            else if (memcmp(ID, "CRUNCH  ", 8) == 0)
                GENLDC(3);
            else
                ERROR(2);
            INSYMBOL();
        } else
            ERROR(2);
    } else
        GENLDC(0);
    GEN2(77, 0, 6);                             /* CXP SYS FCLOSE */
    if (IOCHECK)
        GEN1(30, 0);                            /* CSP IOC */
}

static void GETPUTETC(struct CALLCTX *c)
{
    SETS s;
    VARIABLE(SETADD(&s, c->FSYS, COMMA, RPARENT, -1));
    LOADADDRESS();
    if (GATTR.TYPTR != NULL) {
        if (GATTR.TYPTR->FORM != FILES)
            ERROR(125);
        else if (GATTR.TYPTR->U.FILTYPE == NULL)
            ERROR(399);
    }
    switch (c->LKEY) {
    case 32:
        if (SY == COMMA) {
            INSYMBOL();
            EXPRESSION(SETADD(&s, c->FSYS, RPARENT, -1));
            LOAD();
            if (GATTR.TYPTR != INTPTR)
                ERROR(125);
        } else
            ERROR(125);
        GENNR(SEEK);
        break;
    case 34:
        GEN2(77, 0, 7);                         /* CXP SYS FGET */
        break;
    case 35:
        GEN2(77, 0, 8);                         /* CXP SYS FPUT */
        break;
    case 40:
        if (GATTR.TYPTR != NULL)
            if (GATTR.TYPTR->U.FILTYPE != CHARPTR)
                ERROR(399);
        GENLDC(12);
        GENLDC(0);
        GEN2(77, 0, 17);                        /* CXP SYS WRC */
        break;
    }
    if (IOCHECK)
        GEN1(30, 0);                            /* CSP IOC */
}

static void SCANSTMT(struct CALLCTX *c)         /* SCAN */
{
    SETS s;
    if (GATTR.TYPTR != NULL)
        if (GATTR.TYPTR != INTPTR)
            ERROR(125);
    if (SY == COMMA)
        INSYMBOL();
    else
        ERROR(20);
    if (SY == RELOP) {
        if (OP == EQOP)
            GENLDC(0);
        else if (OP == NEOP)
            GENLDC(1);
        else
            ERROR(125);
        INSYMBOL();
    } else
        ERROR(125);
    EXPRESSION(SETADD(&s, c->FSYS, COMMA, -1));
    LOAD();
    if (GATTR.TYPTR != NULL)
        if (GATTR.TYPTR != CHARPTR)
            ERROR(125);
    if (SY == COMMA)
        INSYMBOL();
    else
        ERROR(20);
    VARIABLE(SETADD(&s, c->FSYS, COMMA, RPARENT, -1));
    BYTEADDRESS();
    if (SY == COMMA) {
        INSYMBOL();
        EXPRESSION(SETADD(&s, c->FSYS, RPARENT, -1));
        LOAD();
    } else
        GENLDC(0);
    GEN1(30, 11);                               /* CSP SCN */
    GATTR.TYPTR = INTPTR;
}

static void BLOCKIO(struct CALLCTX *c)
{
    SETS s;
    VARIABLE(SETADD(&s, c->FSYS, COMMA, -1));
    LOADADDRESS();
    if (GATTR.TYPTR != NULL) {
        if (GATTR.TYPTR->FORM != FILES)
            ERROR(125);
        else if (GATTR.TYPTR->U.FILTYPE != NULL)
            ERROR(399);
    }
    if (SY == COMMA)
        INSYMBOL();
    else
        ERROR(20);
    VARIABLE(&s);
    BYTEADDRESS();
    if (SY == COMMA)
        INSYMBOL();
    else
        ERROR(20);
    EXPRESSION(SETADD(&s, c->FSYS, COMMA, RPARENT, -1));
    LOAD();
    if (GATTR.TYPTR != INTPTR)
        ERROR(125);
    if (SY == COMMA) {
        INSYMBOL();
        EXPRESSION(SETADD(&s, c->FSYS, RPARENT, -1));
        LOAD();
        if (GATTR.TYPTR != INTPTR)
            ERROR(125);
    } else
        GENLDC(-1);
    if (c->LKEY == 37)
        GENLDC(1);
    else
        GENLDC(0);
    GENLDC(0);
    GENLDC(0);
    GEN2(77, 0, 28);                            /* CXP SYS BLOCKIO */
    if (IOCHECK)
        GEN1(30, 0);                            /* CSP IOC */
    GATTR.TYPTR = INTPTR;
}

static void SIZEOF(void)
{
    CTP LCP;
    SETS s;
    if (SY == IDENT) {
        SEARCHID(SETADD(&s, &VARS, TYPES, FIELD, -1), &LCP);
        INSYMBOL();
        if (LCP->IDTYPE != NULL)
            GENLDC(LCP->IDTYPE->SIZE * CHRSPERWD);
    }
    GATTR.TYPTR = INTPTR;
}

static void ROUTINE(struct CALLCTX *c)
{
    switch (c->LKEY) {
    case 12:
        NEWSTMT(c);
        break;
    case 13: case 14:
        UNITIO(c);
        break;
    case 15:
        CONCAT(c);
        break;
    case 18: case 19: case 43:
        COPYDELETE(c);
        break;
    case 21: case 22: case 27:
        MOVE(c);
        break;
    case 23:
        EXITSTMT();
        break;
    case 31:
        CLOSESTMT(c);
        break;
    case 32: case 34: case 35: case 40:
        GETPUTETC(c);
        break;
    case 36:
        SCANSTMT(c);
        break;
    case 37: case 38:
        BLOCKIO(c);
        break;
    case 41:
        SIZEOF();
        break;
    case 42:
        STR(c);
        break;
    }
}

/* ======================== BODYPART.C: READ, WRITE, the other calls ======================== */

static void LOADIDADDR(CTP FCP)
{
    if (FCP->KLASS == ACTUALVARS) {
        if (FCP->U.V.VLEV == 1)
            GEN1(37, FCP->U.V.VADDR);           /* LAO */
        else
            GEN2(50, LEVEL - FCP->U.V.VLEV, FCP->U.V.VADDR);  /* LDA */
    } else {                                    /* FORMALVARS */
        if (FCP->U.V.VLEV == 1)
            GEN1(41, FCP->U.V.VADDR);           /* LDO */
        else
            GEN2(54, LEVEL - FCP->U.V.VLEV, FCP->U.V.VADDR);  /* LOD */
    }
}

/* READ, WRITE: a TEXT file first? */
static void FILEARG(SETOFIDS *FIDCLS, CTP *FILEPTR)
{
    CTP LCP;
    SEARCHID(FIDCLS, &LCP);
    if (LCP->IDTYPE != NULL)
        if (LCP->IDTYPE->FORM == FILES)
            if (LCP->IDTYPE->U.FILTYPE == CHARPTR) {
                INSYMBOL();
                *FILEPTR = LCP;
                if (!(SY == COMMA || SY == RPARENT))
                    ERROR(20);
                if (SY == COMMA)
                    INSYMBOL();
            }
}

static void READSTMT(struct CALLCTX *c)         /* READ */
{
    CTP FILEPTR;
    SETS s, f;
    FILEPTR = INPUTPTR;
    if (SY == IDENT && c->WASLPARENT)
        FILEARG(SETADD(&f, &VARS, FIELD, -1), &FILEPTR);
    else if (c->WASLPARENT)
        ERROR(2);
    if (c->WASLPARENT && SY != RPARENT) {
        do {
            LOADIDADDR(FILEPTR);
            VARIABLE(SETADD(&s, c->FSYS, COMMA, RPARENT, -1));
            if (GATTR.ACCESS == BYTE)
                ERROR(103);
            LOADADDRESS();
            if (GATTR.TYPTR != NULL) {
                if (COMPTYPES(INTPTR, GATTR.TYPTR))
                    GEN2(77, 0, 12);            /* CXP SYS FRDI */
                else if (COMPTYPES(REALPTR, GATTR.TYPTR))
                    GENNR(FREADREAL);
                else if (COMPTYPES(LONGINTPTR, GATTR.TYPTR)) {
                    GENLDC(GATTR.TYPTR->SIZE);
                    GENNR(FREADDEC);
                } else if (COMPTYPES(CHARPTR, GATTR.TYPTR))
                    GEN2(77, 0, 16);            /* CXP SYS FRDC */
                else if (STRGTYPE(GATTR.TYPTR)) {
                    GENLDC(GATTR.TYPTR->U.AR.MAXLENG);
                    GEN2(77, 0, 18);            /* CXP SYS FRDS */
                } else
                    ERROR(125);
            }
            if (IOCHECK)
                GEN1(30, 0);                    /* CSP IOC */
            TEST = SY != COMMA;
            if (!TEST)
                INSYMBOL();
        } while (!TEST);
    }
    if (c->LKEY == 2) {
        LOADIDADDR(FILEPTR);
        GEN2(77, 0, 21);                        /* CXP SYS FRLN */
        if (IOCHECK)
            GEN1(30, 0);                        /* CSP IOC */
    }
}

static void WRITESTMT(struct CALLCTX *c)        /* WRITE */
{
    STP LSP;
    BOOLEAN DEFAULT;
    CTP FILEPTR;
    INTEGER LMIN, LMAX;
    SETS s, f;
    FILEPTR = OUTPUTPTR;
    if (SY == IDENT && c->WASLPARENT)
        FILEARG(SETADD(&f, &VARS, FIELD, KONST, FUNC, -1), &FILEPTR);
    if (c->WASLPARENT && SY != RPARENT) {
        do {
            LOADIDADDR(FILEPTR);
            EXPRESSION(SETADD(&s, c->FSYS, COMMA, COLON, RPARENT, -1));
            LSP = GATTR.TYPTR;
            if (LSP != NULL) {
                if (LSP->FORM > LONGINT)
                    LOADADDRESS();
                else {
                    LOAD();
                    if (LSP->FORM == LONGINT) {
                        GENLDC(DECSIZE(MAXDEC));
                        GENLDC(0);              /* DAJ */
                        GENNR(DECOPS);
                    }
                }
            }
            if (SY == COLON) {
                INSYMBOL();
                EXPRESSION(&s);
                if (GATTR.TYPTR != NULL)
                    if (GATTR.TYPTR != INTPTR)
                        ERROR(20);
                LOAD();
                DEFAULT = FALSE;
            } else
                DEFAULT = TRUE;
            if (LSP == INTPTR) {
                if (DEFAULT)
                    GENLDC(0);
                GEN2(77, 0, 13);                /* CXP SYS FWRI */
            } else if (LSP == REALPTR) {
                if (DEFAULT)
                    GENLDC(0);
                if (SY == COLON) {
                    INSYMBOL();
                    EXPRESSION(SETADD(&s, c->FSYS, COMMA, RPARENT, -1));
                    LOAD();
                    if (GATTR.TYPTR != NULL)
                        if (GATTR.TYPTR != INTPTR)
                            ERROR(125);
                } else
                    GENLDC(0);
                GENNR(FWRITEREAL);
            } else if (COMPTYPES(LSP, LONGINTPTR)) {
                if (DEFAULT)
                    GENLDC(0);
                GENNR(FWRITEDEC);
            } else if (LSP == CHARPTR) {
                if (DEFAULT)
                    GENLDC(0);
                GEN2(77, 0, 17);                /* CXP SYS FWRC */
            } else if (STRGTYPE(LSP)) {
                if (DEFAULT)
                    GENLDC(0);
                GEN2(77, 0, 19);                /* CXP SYS FWRS */
            } else if (PAOFCHAR(LSP)) {
                LMAX = 0;
                if (LSP->U.AR.INXTYPE != NULL) {
                    GETBOUNDS(LSP->U.AR.INXTYPE, &LMIN, &LMAX);
                    LMAX = LMAX - LMIN + 1;
                }
                if (DEFAULT)
                    GENLDC(LMAX);
                GENLDC(LMAX);
                GEN2(77, 0, 20);                /* CXP SYS FWRB */
            } else
                ERROR(125);
            if (IOCHECK)
                GEN1(30, 0);                    /* CSP IOC */
            TEST = SY != COMMA;
            if (!TEST)
                INSYMBOL();
        } while (!TEST);
    }
    if (c->LKEY == 4) {                         /* WRITELN */
        LOADIDADDR(FILEPTR);
        GEN2(77, 0, 22);                        /* CXP SYS FWLN */
        if (IOCHECK)
            GEN1(30, 0);                        /* CSP IOC */
    }
}

static void CALLNONSPECIAL(struct CALLCTX *c)
{
    CTP NXT, FCP;
    STP LSP;
    BOOLEAN LB;
    INTEGER LMIN, LMAX;
    SETS s;
    FCP = c->FCP;
    NXT = FCP->NEXT;
    if (FCP->U.P.PFDECKIND == DECLARED)
        if (FCP->U.P.PFKIND != ACTUAL)
            ERROR(400);
    if (SY == LPARENT) {
        do {
            if (NXT == NULL)
                ERROR(126);
            INSYMBOL();
            EXPRESSION(SETADD(&s, c->FSYS, COMMA, RPARENT, -1));
            if (GATTR.TYPTR != NULL && NXT != NULL) {
                LSP = NXT->IDTYPE;
                if (NXT->KLASS == FORMALVARS || LSP != NULL) {
                    if (NXT->KLASS == ACTUALVARS) {
                        if (GATTR.TYPTR->FORM <= POWER) {
                            LB = GATTR.TYPTR == CHARPTR && GATTR.KIND == CST;
                            LOAD();
                            if (LSP->FORM == POWER)
                                GEN1(32, LSP->SIZE);    /* ADJ */
                            else if (LSP->FORM == LONGINT) {
                                if (GATTR.TYPTR == INTPTR) {
                                    GENLDC(18);         /* DCVT */
                                    GENNR(DECOPS);
                                    GATTR.TYPTR = LONGINTPTR;
                                }
                                GENLDC(LSP->SIZE);
                                GENLDC(0);              /* DAJ */
                                GENNR(DECOPS);
                            } else if (LSP->FORM == SUBRANGE && RANGECHECK) {
                                GENLDC(LSP->U.SR.MIN.IVAL);
                                GENLDC(LSP->U.SR.MAX.IVAL);
                                GEN0(8);                /* CHK */
                            } else if (GATTR.TYPTR == INTPTR && COMPTYPES(LSP, REALPTR)) {
                                GEN0(10);               /* FLT */
                                GATTR.TYPTR = REALPTR;
                            } else if (LB && STRGTYPE(LSP))
                                GATTR.TYPTR = STRGPTR;
                        } else {                        /* FORM > POWER */
                            LB = STRGTYPE(GATTR.TYPTR) && GATTR.KIND == CST;
                            LOADADDRESS();
                            if (LB && PAOFCHAR(LSP))
                                if (!LSP->U.AR.AISSTRNG) {
                                    STRGTOPA(STRGCSTIC);
                                    if (LSP->U.AR.INXTYPE != NULL) {
                                        GETBOUNDS(LSP->U.AR.INXTYPE, &LMIN, &LMAX);
                                        if (LMAX - LMIN + 1 != GATTR.TYPTR->U.AR.MAXLENG)
                                            ERROR(142);
                                    }
                                    GATTR.TYPTR = LSP;
                                }
                        }
                    } else {                            /* KLASS = FORMALVARS */
                        if (GATTR.KIND == VARBL) {
                            if (GATTR.ACCESS == BYTE)
                                ERROR(103);
                            LOADADDRESS();
                            if (LSP != NULL)
                                if (LSP->FORM == POWER || LSP->FORM == LONGINT)
                                    if (GATTR.TYPTR->SIZE != LSP->SIZE)
                                        ERROR(142);
                        } else
                            ERROR(154);
                    }
                    if (!COMPTYPES(LSP, GATTR.TYPTR))
                        ERROR(142);
                }
            }
            if (NXT != NULL)
                NXT = NXT->NEXT;
        } while (SY == COMMA);
        if (SY == RPARENT)
            INSYMBOL();
        else
            ERROR(4);
    }
    if (NXT != NULL)
        ERROR(126);
    if (FCP->U.P.PFDECKIND == DECLARED) {
        if (FCP->KLASS == FUNC) {
            GENLDC(0);
            GENLDC(0);
        }
        if (INMODULE) {
            if (SEPPROC) {
                if (FCP->U.P.PFSEG == SEG && FCP->U.P.PFLEV == 1) {
                    GEN1(79, 0);                /* CGP */
                    LINKERREF(PROC, -FCP->U.P.PFNAME, IC - 1);
                } else if (FCP->U.P.PFLEV == 0)
                    GEN2(77, FCP->U.P.PFSEG, FCP->U.P.PFNAME);  /* CXP */
                else
                    ERROR(405);                 /* call not allowed in a separate procedure */
            } else if (FCP->U.P.IMPORTED) {
                GEN2(77, 0, FCP->U.P.PFNAME);   /* CXP */
                LINKERREF(PROC, FCP->U.P.PFSEG, IC - 2);
            } else
                goto L1;
        } else {
        L1:
            if (FCP->U.P.PFSEG != SEG)
                GEN2(77, FCP->U.P.PFSEG, FCP->U.P.PFNAME);  /* CXP */
            else if (FCP->U.P.PFLEV == 0)
                GEN1(66, FCP->U.P.PFNAME);      /* CBP */
            else if (FCP->U.P.PFLEV == LEVEL)
                GEN1(78, FCP->U.P.PFNAME);      /* CLP */
            else if (FCP->U.P.PFLEV == 1)
                GEN1(79, FCP->U.P.PFNAME);      /* CGP */
            else
                GEN1(46, FCP->U.P.PFNAME);      /* CIP */
        }
    } else if (FCP->U.P.CSPNUM == 23)
        GEN1(30, 40);                           /* (temporary I.5 translation: MEM will be CSP 23 in II.0) */
    else if (FCP->U.P.CSPNUM != 21 && FCP->U.P.CSPNUM != 22)
        GEN1(30, FCP->U.P.CSPNUM);              /* CSP */
    GATTR.TYPTR = FCP->IDTYPE;
}

/* a file variable, and its address */
static void FILEVAR(SETOFSYS *FSYS)
{
    VARIABLE(FSYS);
    LOADADDRESS();
    if (GATTR.TYPTR != NULL)
        if (GATTR.TYPTR->FORM != FILES)
            ERROR(125);
}

static void CALL(SETOFSYS *FSYS, CTP FCP)
{
    struct CALLCTX cc, *c = &cc;
    INTEGER LKEY;
    SETS s;
    c->FSYS = FSYS;
    c->FCP = FCP;
    if (FCP->U.P.PFDECKIND == SPECIAL) {
        c->WASLPARENT = TRUE;
        c->LKEY = LKEY = FCP->U.P.KEY;
        if (SY == LPARENT)
            INSYMBOL();
        else if (LKEY == 2 || LKEY == 4 || LKEY == 5 || LKEY == 6)
            c->WASLPARENT = FALSE;
        else
            ERROR(9);
        switch (LKEY) {
        case 7: case 8: case 9: case 10: case 11: case 13: case 14:
        case 25: case 36: case 39: case 42:
            EXPRESSION(SETADD(&s, FSYS, COMMA, RPARENT, -1));
            LOAD();
            break;
        }
        switch (LKEY) {
        case 12: case 13: case 14: case 15: case 18: case 19: case 21:
        case 22: case 23: case 27: case 31: case 32: case 34: case 35:
        case 36: case 37: case 38: case 40: case 41: case 42: case 43:
            ROUTINE(c);
            break;
        case 1: case 2:
            READSTMT(c);
            break;
        case 3: case 4:
            WRITESTMT(c);
            break;
        case 5: case 6:                         /* EOF and EOLN */
            if (c->WASLPARENT) {
                VARIABLE(SETADD(&s, FSYS, RPARENT, -1));
                LOADADDRESS();
                if (GATTR.TYPTR != NULL) {
                    if (GATTR.TYPTR->FORM != FILES)
                        ERROR(125);
                    else if (GATTR.TYPTR->U.FILTYPE != CHARPTR && LKEY == 6)
                        ERROR(399);
                }
            } else
                LOADIDADDR(INPUTPTR);
            GENLDC(0);
            GENLDC(0);
            if (LKEY == 5)
                GEN2(77, 0, 10);                /* CXP SYS FEOF */
            else
                GEN2(77, 0, 11);                /* CXP SYS FEOLN */
            GATTR.TYPTR = BOOLPTR;
            break;
        case 7: case 8:                         /* PRED, SUCC */
            GENLDC(1);
            if (GATTR.TYPTR != NULL) {
                if (GATTR.TYPTR->FORM == SCALAR) {
                    if (LKEY == 8)
                        GEN0(2);                /* ADI */
                    else
                        GEN0(21);               /* SBI */
                } else
                    ERROR(115);
            }
            break;
        case 9:                                 /* ORD */
            if (GATTR.TYPTR != NULL)
                if (GATTR.TYPTR->FORM >= POWER)
                    ERROR(125);
            GATTR.TYPTR = INTPTR;
            break;
        case 10:                                /* SQR */
            if (GATTR.TYPTR != NULL) {
                if (GATTR.TYPTR == INTPTR)
                    GEN0(24);                   /* SQI */
                else if (GATTR.TYPTR == REALPTR)
                    GEN0(25);                   /* SQR */
                else {
                    ERROR(125);
                    GATTR.TYPTR = INTPTR;
                }
            }
            break;
        case 11:                                /* ABS */
            if (GATTR.TYPTR != NULL) {
                if (GATTR.TYPTR == INTPTR)
                    GEN0(0);                    /* ABI */
                else if (GATTR.TYPTR == REALPTR)
                    GEN0(1);                    /* ABR */
                else {
                    ERROR(125);
                    GATTR.TYPTR = INTPTR;
                }
            }
            break;
        case 16:                                /* LENGTH */
            STRGVAR(SETADD(&s, FSYS, RPARENT, -1), FALSE);
            GENLDC(0);                          /* (the index) */
            GEN0(62);                           /* LDB */
            GATTR.TYPTR = INTPTR;
            break;
        case 17:                                /* INSERT */
            STRGVAR(SETADD(&s, FSYS, COMMA, -1), FALSE);
            if (SY == COMMA)
                INSYMBOL();
            else
                ERROR(20);
            STRGVAR(&s, TRUE);
            GENLDC(GATTR.TYPTR->U.AR.MAXLENG);
            if (SY == COMMA)
                INSYMBOL();
            else
                ERROR(20);
            EXPRESSION(SETADD(&s, FSYS, RPARENT, -1));
            LOAD();
            if (GATTR.TYPTR != NULL)
                if (GATTR.TYPTR != INTPTR)
                    ERROR(125);
            GEN2(77, 0, 24);                    /* CXP SYS SINSERT */
            break;
        case 20:                                /* POS */
            STRGVAR(SETADD(&s, FSYS, COMMA, -1), FALSE);
            if (SY == COMMA)
                INSYMBOL();
            else
                ERROR(20);
            STRGVAR(SETADD(&s, FSYS, RPARENT, -1), FALSE);
            GENLDC(0);
            GENLDC(0);
            GEN2(77, 0, 27);                    /* CXP SYS SPOS */
            GATTR.TYPTR = INTPTR;
            break;
        case 24:                                /* IDSEARCH */
            VARIABLE(SETADD(&s, FSYS, COMMA, -1));
            LOADADDRESS();
            if (SY == COMMA)
                INSYMBOL();
            else
                ERROR(20);
            VARIABLE(SETADD(&s, FSYS, RPARENT, -1));
            LOADADDRESS();
            GEN1(30, 7);                        /* CSP IDS */
            break;
        case 25:                                /* TREESEARCH */
            if (SY == COMMA)
                INSYMBOL();
            else
                ERROR(20);
            VARIABLE(SETADD(&s, FSYS, COMMA, -1));
            LOADADDRESS();
            if (SY == COMMA)
                INSYMBOL();
            else
                ERROR(20);
            VARIABLE(SETADD(&s, FSYS, RPARENT, -1));
            LOADADDRESS();
            GATTR.TYPTR = INTPTR;
            GEN1(30, 8);                        /* CSP TRS */
            break;
        case 26:                                /* TIME */
            VARIABLE(SETADD(&s, FSYS, COMMA, -1));
            LOADADDRESS();
            if (GATTR.TYPTR != NULL)
                if (GATTR.TYPTR != INTPTR)
                    ERROR(125);
            if (SY == COMMA)
                INSYMBOL();
            else
                ERROR(20);
            VARIABLE(SETADD(&s, FSYS, RPARENT, -1));
            LOADADDRESS();
            if (GATTR.TYPTR != NULL)
                if (GATTR.TYPTR != INTPTR)
                    ERROR(125);
            GEN1(30, 9);                        /* CSP TIM */
            break;
        case 33: case 28: case 29: case 30:     /* OPEN, RESET, REWRITE */
            FILEVAR(SETADD(&s, FSYS, COMMA, RPARENT, -1));
            if (SY != COMMA) {
                if (LKEY == 33)
                    GEN2(77, 0, 4);             /* CXP SYS FRESET */
                else
                    ERROR(20);
            } else {
                INSYMBOL();
                STRGVAR(SETADD(&s, FSYS, RPARENT, -1), FALSE);
                if (LKEY == 28 || LKEY == 30)
                    GENLDC(0);
                else
                    GENLDC(1);
                GENLDC(0);
                GEN2(77, 0, 5);                 /* CXP SYS FOPEN */
            }
            if (IOCHECK)
                GEN1(30, 0);                    /* CSP IOC */
            break;
        case 39:                                /* TRUNC */
            if (GATTR.TYPTR == INTPTR) {
                GEN0(10);                       /* FLT */
                GATTR.TYPTR = REALPTR;
            }
            if (GATTR.TYPTR != NULL) {
                if (GATTR.TYPTR == REALPTR)
                    GEN1(30, 23);               /* CSP TRUNC (temporary: TRUNC will be CSP 14 in II.0) */
                else if (GATTR.TYPTR->FORM == LONGINT) {
                    GENLDC(20);                 /* DTNC */
                    GENNR(DECOPS);
                } else
                    ERROR(125);
            }
            GATTR.TYPTR = INTPTR;
            break;
        }
        if (c->WASLPARENT) {
            if (SY == RPARENT)
                INSYMBOL();
            else
                ERROR(4);
        }
    } else
        CALLNONSPECIAL(c);
}

/* ======================== BODYPART.D: expressions ======================== */

static void FLOATIT(STP *FSP, BOOLEAN FORCEFLOAT)
{
    if (GATTR.TYPTR == REALPTR || *FSP == REALPTR || FORCEFLOAT) {
        if (GATTR.TYPTR == INTPTR) {
            GEN0(10);                           /* FLT */
            GATTR.TYPTR = REALPTR;
        }
        if (*FSP == INTPTR) {
            GEN0(9);                            /* FLO */
            *FSP = REALPTR;
        }
    }
}

static void STRETCHIT(STP *FSP)
{
    if ((*FSP)->FORM == LONGINT || GATTR.TYPTR->FORM == LONGINT) {
        if (GATTR.TYPTR == INTPTR) {
            GENLDC(18);                         /* DCVT */
            GENNR(DECOPS);
            GATTR.TYPTR = LONGINTPTR;
        } else if (*FSP == INTPTR) {
            GENLDC(14);                         /* DCV */
            GENNR(DECOPS);
            *FSP = LONGINTPTR;
        }
    }
}

/* the Pascal reads NIL^.FORM here when a type is unknown (an error has been
   reported); that is never LONGINT and never POWER here */
#define FORMOF(p) ((p) != NULL ? (p)->FORM : -1)

static void FACTOR(SETOFSYS *FSYS)
{
    CTP LCP;
    BOOLEAN VARPART, ALLCONST;
    STP LSP;
    INTEGER HIGHVAL, LOWVAL, LIC, LOP, I;
    WORD CSTPART[8];                            /* SET OF 0..127 */
    SETS s, t;
    if (!IN(SY, &FACBEGSYS)) {
        ERROR(58);
        SKIP(SETPLUS(&s, FSYS, &FACBEGSYS));
        GATTR.TYPTR = NULL;
    }
    while (IN(SY, &FACBEGSYS)) {
        switch (SY) {
        case IDENT:
            SEARCHID(SETOF(&t, KONST, FORMALVARS, ACTUALVARS, FIELD, FUNC, -1), &LCP);
            INSYMBOL();
            if (LCP->KLASS == FUNC) {
                CALL(FSYS, LCP);
                GATTR.KIND = EXPR;
            } else if (LCP->KLASS == KONST) {
                GATTR.TYPTR = LCP->IDTYPE;
                GATTR.KIND = CST;
                GATTR.CVAL = LCP->U.VALUES;
            } else
                SELECTOR(FSYS, LCP);
            if (GATTR.TYPTR != NULL)
                if (GATTR.TYPTR->FORM == SUBRANGE)
                    GATTR.TYPTR = GATTR.TYPTR->U.SR.RANGETYPE;
            break;
        case INTCONST:
            GATTR.TYPTR = INTPTR;
            GATTR.KIND = CST;
            GATTR.CVAL = VAL;
            INSYMBOL();
            break;
        case REALCONST:
            GATTR.TYPTR = REALPTR;
            GATTR.KIND = CST;
            GATTR.CVAL = VAL;
            INSYMBOL();
            break;
        case STRINGCONST:
            if (LGTH == 1)
                GATTR.TYPTR = CHARPTR;
            else {
                LSP = NEWTYPE(AR);   /* NEW(LSP, ARRAYS, TRUE, TRUE) */
                *LSP = *STRGPTR;
                LSP->U.AR.MAXLENG = LGTH;
                GATTR.TYPTR = LSP;
            }
            GATTR.KIND = CST;
            GATTR.CVAL = VAL;
            INSYMBOL();
            break;
        case LONGCONST:
            LSP = NEWTYPE0();       /* NEW(LSP, LONGINT) */
            *LSP = *LONGINTPTR;
            LSP->SIZE = DECSIZE(LGTH);
            GATTR.TYPTR = LSP;
            GATTR.KIND = CST;
            GATTR.CVAL = VAL;
            INSYMBOL();
            break;
        case LPARENT:
            INSYMBOL();
            EXPRESSION(SETADD(&s, FSYS, RPARENT, -1));
            if (SY == RPARENT)
                INSYMBOL();
            else
                ERROR(4);
            break;
        case NOTSY:
            INSYMBOL();
            FACTOR(FSYS);
            if (GATTR.KIND == CST && GATTR.TYPTR == BOOLPTR)
                GATTR.CVAL.IVAL = !ODD(GATTR.CVAL.IVAL);
            else {
                LOAD();
                GEN0(19);                       /* NOT */
                if (GATTR.TYPTR != NULL)
                    if (GATTR.TYPTR != BOOLPTR) {
                        ERROR(135);
                        GATTR.TYPTR = NULL;
                    }
            }
            break;
        case LBRACK:
            INSYMBOL();
            FILLCHAR(CSTPART, sizeof CSTPART, 0);
            VARPART = FALSE;
            LSP = NEWTYPE(ELSET);       /* NEW(LSP, POWER) */
            LSP->U.ELSET = NULL;
            LSP->SIZE = 0;
            LSP->FORM = POWER;
            if (SY == RBRACK) {
                GATTR.TYPTR = LSP;
                GATTR.KIND = CST;
                INSYMBOL();
            } else {
                do {
                    EXPRESSION(SETADD(&s, FSYS, COMMA, RBRACK, COLON, -1));
                    if (GATTR.TYPTR != NULL) {
                        if (GATTR.TYPTR->FORM != SCALAR) {
                            ERROR(136);
                            GATTR.TYPTR = NULL;
                        } else if (COMPTYPES(LSP->U.ELSET, GATTR.TYPTR)) {
                            ALLCONST = FALSE;
                            LOP = 23;           /* SGS */
                            if (GATTR.KIND == CST && GATTR.CVAL.IVAL <= 127) {
                                ALLCONST = TRUE;
                                LOWVAL = GATTR.CVAL.IVAL;
                                HIGHVAL = LOWVAL;
                            }
                            LIC = IC;
                            LOAD();
                            if (SY == COLON) {
                                INSYMBOL();
                                LOP = 20;       /* SRS */
                                EXPRESSION(SETADD(&s, FSYS, COMMA, RBRACK, -1));
                                if (COMPTYPES(LSP->U.ELSET, GATTR.TYPTR))
                                    ;
                                else {
                                    ERROR(137);
                                    GATTR.TYPTR = NULL;
                                }
                                if (ALLCONST) {
                                    if (GATTR.KIND == CST && GATTR.CVAL.IVAL <= 127)
                                        HIGHVAL = GATTR.CVAL.IVAL;
                                    else {
                                        LOAD();
                                        ALLCONST = FALSE;
                                    }
                                } else
                                    LOAD();
                            }
                            if (ALLCONST) {
                                IC = LIC;       /* forget the first constant */
                                for (I = LOWVAL; I <= HIGHVAL; I++)  /* CSTPART + [LOWVAL..HIGHVAL] */
                                    if (I >= 0)
                                        CSTPART[DIV(I, 16)] |= SETBIT[MOD(I, 16)];
                            } else {
                                GEN0(LOP);
                                if (VARPART)
                                    GEN0(28);   /* UNI */
                                else
                                    VARPART = TRUE;
                            }
                            LSP->U.ELSET = GATTR.TYPTR;
                            GATTR.TYPTR = LSP;
                        } else
                            ERROR(137);
                    }
                    TEST = SY != COMMA;
                    if (!TEST)
                        INSYMBOL();
                } while (!TEST);
                if (SY == RBRACK)
                    INSYMBOL();
                else
                    ERROR(12);
            }
            if (VARPART) {
                for (I = 0; I < 8; I++)
                    if (CSTPART[I] != 0)
                        break;
                if (I < 8) {                    /* CSTPART <> [] */
                    MOVELEFT(CSTPART, SCONST->U.PVAL, sizeof CSTPART);
                    SCONST->CCLASS = PSET;
                    GATTR.CVAL.VALP = SCONST;
                    GATTR.KIND = CST;
                    LOAD();
                    GEN0(28);                   /* UNI */
                }
                GATTR.KIND = EXPR;
            } else {
                MOVELEFT(CSTPART, SCONST->U.PVAL, sizeof CSTPART);
                SCONST->CCLASS = PSET;
                GATTR.CVAL.VALP = SCONST;
                GATTR.KIND = CST;
            }
            break;
        }
        if (!IN(SY, FSYS)) {
            ERROR(6);
            SKIP(SETPLUS(&s, FSYS, &FACBEGSYS));
        }
    }
}

static void TERM(SETOFSYS *FSYS)
{
    ATTR LATTR;
    OPERATOR LOP;
    SETS s;
    FACTOR(SETADD(&s, FSYS, MULOP, -1));
    while (SY == MULOP) {
        LOAD();
        LATTR = GATTR;
        LOP = OP;
        INSYMBOL();
        FACTOR(&s);
        LOAD();
        if (LATTR.TYPTR != NULL && GATTR.TYPTR != NULL)
            switch (LOP) {
            case MUL:
                FLOATIT(&LATTR.TYPTR, FALSE);
                STRETCHIT(&LATTR.TYPTR);
                if (LATTR.TYPTR == INTPTR && GATTR.TYPTR == INTPTR)
                    GEN0(15);                   /* MPI */
                else if (LATTR.TYPTR == REALPTR && GATTR.TYPTR == REALPTR)
                    GEN0(16);                   /* MPR */
                else if (GATTR.TYPTR->FORM == LONGINT && LATTR.TYPTR->FORM == LONGINT) {
                    GENLDC(8);                  /* DMP */
                    GENNR(DECOPS);
                } else if (LATTR.TYPTR->FORM == POWER && COMPTYPES(LATTR.TYPTR, GATTR.TYPTR))
                    GEN0(12);                   /* INT */
                else {
                    ERROR(134);
                    GATTR.TYPTR = NULL;
                }
                break;
            case RDIV:
                FLOATIT(&LATTR.TYPTR, TRUE);
                if (LATTR.TYPTR == REALPTR && GATTR.TYPTR == REALPTR)
                    GEN0(7);                    /* DVR */
                else {
                    ERROR(134);
                    GATTR.TYPTR = NULL;
                }
                break;
            case IDIV:
                STRETCHIT(&LATTR.TYPTR);
                if (LATTR.TYPTR == INTPTR && GATTR.TYPTR == INTPTR)
                    GEN0(6);                    /* DVI */
                else if (LATTR.TYPTR->FORM == LONGINT && GATTR.TYPTR->FORM == LONGINT) {
                    GENLDC(10);                 /* DDV */
                    GENNR(DECOPS);
                } else {
                    ERROR(134);
                    GATTR.TYPTR = NULL;
                }
                break;
            case IMOD:
                if (LATTR.TYPTR == INTPTR && GATTR.TYPTR == INTPTR)
                    GEN0(14);                   /* MOD */
                else {
                    ERROR(134);
                    GATTR.TYPTR = NULL;
                }
                break;
            case ANDOP:
                if (LATTR.TYPTR == BOOLPTR && GATTR.TYPTR == BOOLPTR)
                    GEN0(4);                    /* AND */
                else {
                    ERROR(134);
                    GATTR.TYPTR = NULL;
                }
                break;
            }
        else
            GATTR.TYPTR = NULL;
    }
}

static void SIMPLEEXPRESSION(SETOFSYS *FSYS)
{
    ATTR LATTR;
    OPERATOR LOP;
    BOOLEAN SIGNED;
    SETS s;
    SIGNED = FALSE;
    if (SY == ADDOP && (OP == PLUS || OP == MINUS)) {
        SIGNED = OP == MINUS;
        INSYMBOL();
    }
    TERM(SETADD(&s, FSYS, ADDOP, -1));
    if (SIGNED) {
        LOAD();
        if (GATTR.TYPTR == INTPTR)
            GEN0(17);                           /* NGI */
        else if (GATTR.TYPTR == REALPTR)
            GEN0(18);                           /* NGR */
        else if (FORMOF(GATTR.TYPTR) == LONGINT) {
            GENLDC(6);                          /* DNG */
            GENNR(DECOPS);
        } else {
            ERROR(134);
            GATTR.TYPTR = NULL;
        }
    }
    while (SY == ADDOP) {
        LOAD();
        LATTR = GATTR;
        LOP = OP;
        INSYMBOL();
        TERM(&s);
        LOAD();
        if (LATTR.TYPTR != NULL && GATTR.TYPTR != NULL)
            switch (LOP) {
            case PLUS:
                FLOATIT(&LATTR.TYPTR, FALSE);
                STRETCHIT(&LATTR.TYPTR);
                if (LATTR.TYPTR == INTPTR && GATTR.TYPTR == INTPTR)
                    GEN0(2);                    /* ADI */
                else if (LATTR.TYPTR == REALPTR && GATTR.TYPTR == REALPTR)
                    GEN0(3);                    /* ADR */
                else if (GATTR.TYPTR->FORM == LONGINT && LATTR.TYPTR->FORM == LONGINT) {
                    GENLDC(2);                  /* DAD */
                    GENNR(DECOPS);
                } else if (LATTR.TYPTR->FORM == POWER && COMPTYPES(LATTR.TYPTR, GATTR.TYPTR))
                    GEN0(28);                   /* UNI */
                else {
                    ERROR(134);
                    GATTR.TYPTR = NULL;
                }
                break;
            case MINUS:
                FLOATIT(&LATTR.TYPTR, FALSE);
                STRETCHIT(&LATTR.TYPTR);
                if (LATTR.TYPTR == INTPTR && GATTR.TYPTR == INTPTR)
                    GEN0(21);                   /* SBI */
                else if (LATTR.TYPTR == REALPTR && GATTR.TYPTR == REALPTR)
                    GEN0(22);                   /* SBR */
                else if (GATTR.TYPTR->FORM == LONGINT && LATTR.TYPTR->FORM == LONGINT) {
                    GENLDC(4);                  /* DSB */
                    GENNR(DECOPS);
                } else if (LATTR.TYPTR->FORM == POWER && COMPTYPES(LATTR.TYPTR, GATTR.TYPTR))
                    GEN0(5);                    /* DIF */
                else {
                    ERROR(134);
                    GATTR.TYPTR = NULL;
                }
                break;
            case OROP:
                if (LATTR.TYPTR == BOOLPTR && GATTR.TYPTR == BOOLPTR)
                    GEN0(13);                   /* IOR */
                else {
                    ERROR(134);
                    GATTR.TYPTR = NULL;
                }
                break;
            }
        else
            GATTR.TYPTR = NULL;
    }
}

static void MAKEPA(STP *STRGFSP, STP PAFSP)
{
    INTEGER LMIN, LMAX;
    if (PAFSP->U.AR.INXTYPE != NULL) {
        GETBOUNDS(PAFSP->U.AR.INXTYPE, &LMIN, &LMAX);
        if (LMAX - LMIN + 1 != (*STRGFSP)->U.AR.MAXLENG)
            ERROR(129);
    }
    *STRGFSP = PAFSP;
}

/* EXPRESSION: the value as an operand of a comparison */
static void OPERAND(void)
{
    if (GATTR.TYPTR != NULL) {
        if (GATTR.TYPTR->FORM <= POWER)
            LOAD();
        else
            LOADADDRESS();
    }
}

static void EXPRESSION(SETOFSYS *FSYS)
{
    ATTR LATTR;
    OPERATOR LOP;
    INTEGER TYPIND;
    ADDRRANGE LSTRGIC, LSIZE;
    BOOLEAN LSTRING, GSTRING;
    INTEGER LMIN, LMAX;
    SETS s;
    SIMPLEEXPRESSION(SETADD(&s, FSYS, RELOP, -1));
    if (SY == RELOP) {
        LSTRING = GATTR.KIND == CST && (STRGTYPE(GATTR.TYPTR) || GATTR.TYPTR == CHARPTR);
        OPERAND();
        LATTR = GATTR;
        LOP = OP;
        LSTRGIC = STRGCSTIC;
        INSYMBOL();
        SIMPLEEXPRESSION(FSYS);
        GSTRING = GATTR.KIND == CST && (STRGTYPE(GATTR.TYPTR) || GATTR.TYPTR == CHARPTR);
        OPERAND();
        if (LATTR.TYPTR != NULL && GATTR.TYPTR != NULL) {
            if (LOP == INOP) {
                if (GATTR.TYPTR->FORM == POWER) {
                    if (COMPTYPES(LATTR.TYPTR, GATTR.TYPTR->U.ELSET))
                        GEN0(11);               /* INN */
                    else {
                        ERROR(129);
                        GATTR.TYPTR = NULL;
                    }
                } else {
                    ERROR(130);
                    GATTR.TYPTR = NULL;
                }
            } else {
                if (LATTR.TYPTR != GATTR.TYPTR) {
                    FLOATIT(&LATTR.TYPTR, FALSE);
                    STRETCHIT(&LATTR.TYPTR);
                }
                if (LSTRING) {
                    if (PAOFCHAR(GATTR.TYPTR))
                        if (!GATTR.TYPTR->U.AR.AISSTRNG) {
                            STRGTOPA(LSTRGIC);
                            MAKEPA(&LATTR.TYPTR, GATTR.TYPTR);
                        }
                } else if (GSTRING) {
                    if (PAOFCHAR(LATTR.TYPTR))
                        if (!LATTR.TYPTR->U.AR.AISSTRNG) {
                            STRGTOPA(STRGCSTIC);
                            MAKEPA(&GATTR.TYPTR, LATTR.TYPTR);
                        }
                }
                if ((LSTRING && STRGTYPE(GATTR.TYPTR)) || (GSTRING && STRGTYPE(LATTR.TYPTR)))
                    goto L1;                    /* (the string compare kludge) */
                if (COMPTYPES(LATTR.TYPTR, GATTR.TYPTR)) {
                    LSIZE = LATTR.TYPTR->SIZE;  /* (invalid for long integers) */
                    switch (LATTR.TYPTR->FORM) {
                    case SCALAR:
                        if (LATTR.TYPTR == REALPTR)
                            TYPIND = 1;
                        else if (LATTR.TYPTR == BOOLPTR)
                            TYPIND = 3;
                        else
                            TYPIND = 0;
                        break;
                    case POINTER:
                        if (LOP == LTOP || LOP == LEOP || LOP == GTOP || LOP == GEOP)
                            ERROR(131);
                        TYPIND = 0;
                        break;
                    case LONGINT:
                        TYPIND = 7;
                        break;
                    case POWER:
                        if (LOP == LTOP || LOP == GTOP)
                            ERROR(132);
                        TYPIND = 4;
                        break;
                    case ARRAYS:
                        TYPIND = 6;
                        if (PAOFCHAR(LATTR.TYPTR)) {
                            if (LATTR.TYPTR->U.AR.AISSTRNG) {
                            L1:
                                TYPIND = 2;
                            } else {
                                TYPIND = 5;
                                if (LATTR.TYPTR->U.AR.INXTYPE != NULL) {
                                    GETBOUNDS(LATTR.TYPTR->U.AR.INXTYPE, &LMIN, &LMAX);
                                    LSIZE = LMAX - LMIN + 1;
                                }
                            }
                        } else if (LOP == LTOP || LOP == LEOP || LOP == GTOP || LOP == GEOP)
                            ERROR(131);
                        break;
                    case RECORDS:
                        if (LOP == LTOP || LOP == LEOP || LOP == GTOP || LOP == GEOP)
                            ERROR(131);
                        TYPIND = 6;
                        break;
                    case FILES:
                        ERROR(133);
                        TYPIND = 0;
                        break;
                    }
                    if (TYPIND == 7) {
                        GENLDC(LOP);
                        GENLDC(16);             /* DCMP */
                        GENNR(DECOPS);
                    } else
                        switch (LOP) {
                        case LTOP:
                            GEN2(53, TYPIND, LSIZE);    /* LES */
                            break;
                        case LEOP:
                            GEN2(52, TYPIND, LSIZE);    /* LEQ */
                            break;
                        case GTOP:
                            GEN2(49, TYPIND, LSIZE);    /* GRT */
                            break;
                        case GEOP:
                            GEN2(48, TYPIND, LSIZE);    /* GEQ */
                            break;
                        case NEOP:
                            GEN2(55, TYPIND, LSIZE);    /* NEQ */
                            break;
                        case EQOP:
                            GEN2(47, TYPIND, LSIZE);    /* EQU */
                            break;
                        }
                } else
                    ERROR(129);
            }
        }
        GATTR.TYPTR = BOOLPTR;
        GATTR.KIND = EXPR;
    }
}

/* ======================== BODYPART.E: statements ======================== */

static void STATEMENT(SETOFSYS *FSYS);

/* a range check for a subrange variable */
static void CHECKRANGE(STP FSP)
{
    if (FSP != NULL)
        if (FSP->FORM == SUBRANGE && RANGECHECK) {
            GENLDC(FSP->U.SR.MIN.IVAL);
            GENLDC(FSP->U.SR.MAX.IVAL);
            GEN0(8);                            /* CHK */
        }
}

static void ASSIGNMENT(SETOFSYS *FSYS, CTP FCP)
{
    ATTR LATTR;
    BOOLEAN CSTRING, PAONLEFT;
    INTEGER LMIN, LMAX;
    SETS s;
    SELECTOR(SETADD(&s, FSYS, BECOMES, -1), FCP);
    if (SY == BECOMES) {
        LMAX = 0;
        CSTRING = FALSE;
        if (GATTR.TYPTR != NULL)
            if (GATTR.ACCESS == INDRCT || GATTR.TYPTR->FORM > POWER)
                LOADADDRESS();
        PAONLEFT = PAOFCHAR(GATTR.TYPTR);
        LATTR = GATTR;
        INSYMBOL();
        EXPRESSION(FSYS);
        if (GATTR.KIND == CST)
            CSTRING = GATTR.TYPTR == CHARPTR || STRGTYPE(GATTR.TYPTR);
        OPERAND();
        if (LATTR.TYPTR != NULL && GATTR.TYPTR != NULL) {
            if (GATTR.TYPTR == INTPTR)
                if (COMPTYPES(REALPTR, LATTR.TYPTR)) {
                    GEN0(10);                   /* FLT */
                    GATTR.TYPTR = REALPTR;
                }
            if (COMPTYPES(LONGINTPTR, LATTR.TYPTR)) {
                if (GATTR.TYPTR == INTPTR) {
                    GENLDC(18);                 /* DCVT */
                    GENNR(DECOPS);
                    GATTR.TYPTR = LONGINTPTR;
                }
                if (GATTR.TYPTR->FORM != LONGINT) {
                    ERROR(129);
                    GATTR.TYPTR = LONGINTPTR;
                }
            }
            if (PAONLEFT) {
                if (LATTR.TYPTR->U.AR.AISSTRNG) {
                    if (CSTRING && GATTR.TYPTR == CHARPTR)
                        GATTR.TYPTR = STRGPTR;
                } else if (LATTR.TYPTR->U.AR.INXTYPE != NULL) {
                    GETBOUNDS(LATTR.TYPTR->U.AR.INXTYPE, &LMIN, &LMAX);
                    LMAX = LMAX - LMIN + 1;
                    if (CSTRING && GATTR.TYPTR != CHARPTR) {
                        STRGTOPA(STRGCSTIC);
                        if (LMAX != GATTR.TYPTR->U.AR.MAXLENG)
                            ERROR(129);
                        GATTR.TYPTR = LATTR.TYPTR;
                    }
                } else
                    GATTR.TYPTR = LATTR.TYPTR;
            }
            if (COMPTYPES(LATTR.TYPTR, GATTR.TYPTR))
                switch (LATTR.TYPTR->FORM) {
                case SUBRANGE:
                    CHECKRANGE(LATTR.TYPTR);
                    STORE(&LATTR);
                    break;
                case POWER:
                    GEN1(32, LATTR.TYPTR->SIZE);        /* ADJ */
                    STORE(&LATTR);
                    break;
                case SCALAR:
                case POINTER:
                    STORE(&LATTR);
                    break;
                case LONGINT:
                    GENLDC(LATTR.TYPTR->SIZE);
                    GENLDC(0);                          /* DAJ */
                    GENNR(DECOPS);
                    STORE(&LATTR);
                    break;
                case ARRAYS:
                    if (PAONLEFT) {
                        if (LATTR.TYPTR->U.AR.AISSTRNG)
                            GEN1(42, LATTR.TYPTR->U.AR.MAXLENG);    /* SAS */
                        else
                            GEN1(40, DIV(LMAX + 1, 2));               /* MOV */
                    } else
                        GEN1(40, LATTR.TYPTR->SIZE);                /* MOV */
                    break;
                case RECORDS:
                    GEN1(40, LATTR.TYPTR->SIZE);                    /* MOV */
                    break;
                case FILES:
                    ERROR(146);
                    break;
                }
            else
                ERROR(129);
        }
    } else
        ERROR(51);
}

static void GOTOSTATEMENT(void)
{
    LABELP LLP;
    BOOLEAN FOUND;
    DISPRANGE TTOP;
    if (!GOTOOK)
        ERROR(6);
    if (SY == INTCONST) {
        FOUND = FALSE;
        TTOP = TOP;
        while (DISPLAY[TTOP].OCCUR != BLCK)
            TTOP = TTOP - 1;
        LLP = DISPLAY[TTOP].FLABEL;
        while (LLP != NULL && !FOUND)
            if (LLP->LABVAL == VAL.IVAL) {
                FOUND = TRUE;
                GENJMP(57, LLP->CODELBP);       /* UJP */
            } else
                LLP = LLP->NEXTLAB;
        if (!FOUND)
            ERROR(167);
        INSYMBOL();
    } else
        ERROR(15);
}

/* statements, up to a symbol that does not begin one; then ; and another */
static void STATEMENTS(SETOFSYS *FSYS, SYMBOL FSY)
{
    SETS s;
    SETADD(&s, FSYS, SEMICOLON, FSY, -1);
    do {
        do
            STATEMENT(&s);
        while (IN(SY, &STATBEGSYS));
        TEST = SY != SEMICOLON;
        if (!TEST)
            INSYMBOL();
    } while (!TEST);
}

static void COMPOUNDSTATEMENT(SETOFSYS *FSYS)
{
    STATEMENTS(FSYS, ENDSY);
    if (SY == ENDSY)
        INSYMBOL();
    else
        ERROR(13);
}

static void IFSTATEMENT(SETOFSYS *FSYS)
{
    LBP LCIX1, LCIX2;
    INTEGER LIC;
    BOOLEAN CONDCOMPILE, NOTHENCLAUSE;
    SETS s;
    CONDCOMPILE = FALSE;
    EXPRESSION(SETADD(&s, FSYS, THENSY, -1));
    if (GATTR.KIND == CST)
        if (GATTR.TYPTR == BOOLPTR) {
            CONDCOMPILE = TRUE;
            NOTHENCLAUSE = !ODD(GATTR.CVAL.IVAL);
            LIC = IC;
        }
    if (!CONDCOMPILE) {
        GENLABEL(&LCIX1);
        GENFJP(LCIX1);
    }
    if (SY == THENSY)
        INSYMBOL();
    else
        ERROR(52);
    STATEMENT(SETADD(&s, FSYS, ELSESY, -1));
    if (CONDCOMPILE) {
        if (NOTHENCLAUSE)
            IC = LIC;
        else
            LIC = IC;
    }
    if (SY == ELSESY) {
        if (!CONDCOMPILE) {
            GENLABEL(&LCIX2);
            GENJMP(57, LCIX2);                  /* UJP */
            PUTLABEL(LCIX1);
        }
        INSYMBOL();
        STATEMENT(FSYS);
        if (CONDCOMPILE) {
            if (!NOTHENCLAUSE)
                IC = LIC;
        } else
            PUTLABEL(LCIX2);
    } else if (!CONDCOMPILE)
        PUTLABEL(LCIX1);
}

struct CASEINFO {
    struct CASEINFO *NEXT;
    INTEGER CSSTART;
    INTEGER CSLAB;
};
typedef struct CASEINFO *CIP;

static void CASESTATEMENT(SETOFSYS *FSYS)
{
    STP LSP, LSP1;
    CIP FSTPTR, LPT1, LPT2, LPT3;
    VALU LVAL;
    LBP LADDR, LCIX;
    INTEGER NULSTMT, LMIN, LMAX;
    SETS s;
    EXPRESSION(SETADD(&s, FSYS, OFSY, COMMA, COLON, -1));
    LOAD();
    GENLABEL(&LCIX);
    GENJMP(57, LCIX);                           /* UJP */
    LSP = GATTR.TYPTR;
    if (LSP != NULL)
        if (LSP->FORM != SCALAR || LSP == REALPTR) {
            ERROR(144);
            LSP = NULL;
        }
    if (SY == OFSY)
        INSYMBOL();
    else
        ERROR(8);
    FSTPTR = NULL;
    GENLABEL(&LADDR);
    do {
        LPT3 = NULL;
        do {
            CONSTANT(SETADD(&s, FSYS, COMMA, COLON, -1), &LSP1, &LVAL);
            if (LSP != NULL) {
                if (COMPTYPES(LSP, LSP1)) {
                    LPT1 = FSTPTR;
                    LPT2 = NULL;
                    while (LPT1 != NULL) {
                        if (LPT1->CSLAB <= LVAL.IVAL) {
                            if (LPT1->CSLAB == LVAL.IVAL)
                                ERROR(156);
                            break;              /* GOTO 1 */
                        }
                        LPT2 = LPT1;
                        LPT1 = LPT1->NEXT;
                    }
                    LPT3 = PNEW(sizeof(struct CASEINFO));
                    LPT3->NEXT = LPT1;
                    LPT3->CSLAB = LVAL.IVAL;
                    LPT3->CSSTART = IC;
                    if (LPT2 == NULL)
                        FSTPTR = LPT3;
                    else
                        LPT2->NEXT = LPT3;
                } else
                    ERROR(147);
            }
            TEST = SY != COMMA;
            if (!TEST)
                INSYMBOL();
        } while (!TEST);
        if (SY == COLON)
            INSYMBOL();
        else
            ERROR(5);
        SETADD(&s, FSYS, SEMICOLON, -1);
        do
            STATEMENT(&s);
        while (IN(SY, &STATBEGSYS));
        if (LPT3 != NULL)
            GENJMP(57, LADDR);                  /* UJP */
        TEST = SY != SEMICOLON;
        if (!TEST)
            INSYMBOL();
    } while (!(TEST || SY == ENDSY));
    PUTLABEL(LCIX);
    if (FSTPTR != NULL) {
        LMAX = FSTPTR->CSLAB;
        LPT1 = FSTPTR;
        FSTPTR = NULL;
        do {
            LPT2 = LPT1->NEXT;
            LPT1->NEXT = FSTPTR;
            FSTPTR = LPT1;
            LPT1 = LPT2;
        } while (LPT1 != NULL);
        LMIN = FSTPTR->CSLAB;
        GEN0(44);                               /* XJP */
        GENWORD(LMIN);
        GENWORD(LMAX);
        NULSTMT = IC;
        GENJMP(57, LADDR);                      /* UJP */
        do {
            while (FSTPTR->CSLAB > LMIN) {
                GENWORD(IC - NULSTMT);
                LMIN = LMIN + 1;
            }
            GENWORD(IC - FSTPTR->CSSTART);
            FSTPTR = FSTPTR->NEXT;
            LMIN = LMIN + 1;
        } while (FSTPTR != NULL);
        PUTLABEL(LADDR);
    }
    if (SY == ENDSY)
        INSYMBOL();
    else
        ERROR(13);
}

static void REPEATSTATEMENT(SETOFSYS *FSYS)
{
    LBP LADDR;
    GENLABEL(&LADDR);
    PUTLABEL(LADDR);
    STATEMENTS(FSYS, UNTILSY);
    if (SY == UNTILSY) {
        INSYMBOL();
        EXPRESSION(FSYS);
        GENFJP(LADDR);
    } else
        ERROR(53);
}

static void WHILESTATEMENT(SETOFSYS *FSYS)
{
    LBP LADDR, LCIX;
    SETS s;
    GENLABEL(&LADDR);
    PUTLABEL(LADDR);
    EXPRESSION(SETADD(&s, FSYS, DOSY, -1));
    GENLABEL(&LCIX);
    GENFJP(LCIX);
    if (SY == DOSY)
        INSYMBOL();
    else
        ERROR(54);
    STATEMENT(FSYS);
    GENJMP(57, LADDR);                          /* UJP */
    PUTLABEL(LCIX);
}

static void FORSTATEMENT(SETOFSYS *FSYS)
{
    ATTR LATTR;
    SYMBOL LSY;
    LBP LCIX, LADDR;
    CTP LCP;
    SETS s;
    if (SY == IDENT) {
        SEARCHID(&VARS, &LCP);
        LATTR.TYPTR = LCP->IDTYPE;
        LATTR.KIND = VARBL;
        if (LCP->KLASS == ACTUALVARS) {
            LATTR.ACCESS = DRCT;
            LATTR.VLEVEL = LCP->U.V.VLEV;
            LATTR.DPLMT = LCP->U.V.VADDR;
        } else {
            ERROR(155);
            LATTR.TYPTR = NULL;
        }
        if (LATTR.TYPTR != NULL)
            if (LATTR.TYPTR->FORM > SUBRANGE || COMPTYPES(REALPTR, LATTR.TYPTR)) {
                ERROR(143);
                LATTR.TYPTR = NULL;
            }
        INSYMBOL();
    } else {
        ERROR(2);
        SKIP(SETADD(&s, FSYS, BECOMES, TOSY, DOWNTOSY, DOSY, -1));
    }
    if (SY == BECOMES) {
        INSYMBOL();
        EXPRESSION(SETADD(&s, FSYS, TOSY, DOWNTOSY, DOSY, -1));
        if (GATTR.TYPTR != NULL) {
            if (GATTR.TYPTR->FORM != SCALAR)
                ERROR(144);
            else if (COMPTYPES(LATTR.TYPTR, GATTR.TYPTR)) {
                LOAD();
                CHECKRANGE(LATTR.TYPTR);
                STORE(&LATTR);
            } else
                ERROR(145);
        }
    } else {
        ERROR(51);
        SKIP(SETADD(&s, FSYS, TOSY, DOWNTOSY, DOSY, -1));
    }
    GENLABEL(&LADDR);
    if (SY == TOSY || SY == DOWNTOSY) {
        LSY = SY;
        INSYMBOL();
        EXPRESSION(SETADD(&s, FSYS, DOSY, -1));
        if (GATTR.TYPTR != NULL) {
            if (GATTR.TYPTR->FORM != SCALAR)
                ERROR(144);
            else if (COMPTYPES(LATTR.TYPTR, GATTR.TYPTR)) {
                LOAD();
                CHECKRANGE(LATTR.TYPTR);
                GEN2(56, 0, LC);                /* STR */
                PUTLABEL(LADDR);
                GATTR = LATTR;
                LOAD();
                GEN2(54, 0, LC);                /* LOD */
                LC = LC + INTSIZE;
                if (LC > LCMAX)
                    LCMAX = LC;
                if (LSY == TOSY)
                    GEN2(52, 0, INTSIZE);       /* LEQ */
                else
                    GEN2(48, 0, INTSIZE);       /* GEQ */
            } else
                ERROR(145);
        }
    } else {
        ERROR(55);
        SKIP(SETADD(&s, FSYS, DOSY, -1));
    }
    GENLABEL(&LCIX);
    GENJMP(33, LCIX);                           /* FJP */
    if (SY == DOSY)
        INSYMBOL();
    else
        ERROR(54);
    STATEMENT(FSYS);
    GATTR = LATTR;
    LOAD();
    GENLDC(1);
    if (LSY == TOSY)
        GEN0(2);                                /* ADI */
    else
        GEN0(21);                               /* SBI */
    STORE(&LATTR);
    GENJMP(57, LADDR);                          /* UJP */
    PUTLABEL(LCIX);
    LC = LC - INTSIZE;
}

static void WITHSTATEMENT(SETOFSYS *FSYS)
{
    CTP LCP;
    DISPRANGE LCNT1, LCNT2;
    SETS s;
    LCNT1 = 0;
    LCNT2 = 0;
    do {
        if (SY == IDENT) {
            SEARCHID(SETADD(&s, &VARS, FIELD, -1), &LCP);
            INSYMBOL();
        } else {
            ERROR(2);
            LCP = UVARPTR;
        }
        SELECTOR(SETADD(&s, FSYS, COMMA, DOSY, -1), LCP);
        if (GATTR.TYPTR != NULL) {
            if (GATTR.TYPTR->FORM == RECORDS) {
                if (TOP < DISPLIMIT) {
                    TOP = TOP + 1;
                    LCNT1 = LCNT1 + 1;
                    DISPLAY[TOP].FNAME = GATTR.TYPTR->U.RE.FSTFLD;
                    if (GATTR.ACCESS == DRCT) {
                        DISPLAY[TOP].OCCUR = CREC;
                        DISPLAY[TOP].CLEV = GATTR.VLEVEL;
                        DISPLAY[TOP].CDSPL = GATTR.DPLMT;
                    } else {
                        LOADADDRESS();
                        GEN2(56, 0, LC);        /* STR */
                        DISPLAY[TOP].OCCUR = VREC;
                        DISPLAY[TOP].VDSPL = LC;
                        LC = LC + PTRSIZE;
                        LCNT2 = LCNT2 + PTRSIZE;
                        if (LC > LCMAX)
                            LCMAX = LC;
                    }
                } else
                    ERROR(250);
            } else
                ERROR(140);
        }
        TEST = SY != COMMA;
        if (!TEST)
            INSYMBOL();
    } while (!TEST);
    if (SY == DOSY)
        INSYMBOL();
    else
        ERROR(54);
    STATEMENT(FSYS);
    TOP = TOP - LCNT1;
    LC = LC - LCNT2;
}

static void STATEMENT(SETOFSYS *FSYS)
{
    CTP LCP;
    DISPRANGE TTOP;
    LABELP LLP;
    void *HEAP;
    SETS s, t;
    STMTLEV = STMTLEV + 1;
    if (SY == INTCONST) {                       /* a label */
        TTOP = TOP;
        while (DISPLAY[TTOP].OCCUR != BLCK)
            TTOP = TTOP - 1;
        LLP = DISPLAY[TTOP].FLABEL;
        while (LLP != NULL)
            if (LLP->LABVAL == VAL.IVAL) {
                if (LLP->CODELBP->DEFINED)
                    ERROR(165);
                PUTLABEL(LLP->CODELBP);
                goto L1;
            } else
                LLP = LLP->NEXTLAB;
        ERROR(167);
    L1:
        INSYMBOL();
        if (SY == COLON)
            INSYMBOL();
        else
            ERROR(5);
    }
    if (DEBUGGING) {
        GEN1(85, SCREENDOTS + 1);               /* BPT */
        BPTONLINE = TRUE;
    }
    if (!IN(SY, SETADD(&s, FSYS, IDENT, -1))) {
        ERROR(6);
        SKIP(FSYS);
    }
    if (IN(SY, SETADD(&s, &STATBEGSYS, IDENT, -1))) {
        PMARK(&HEAP);                           /* (for the label cleanup) */
        switch (SY) {
        case IDENT:
            SEARCHID(SETADD(&t, &VARS, FIELD, FUNC, PROC, -1), &LCP);
            INSYMBOL();
            if (LCP->KLASS == PROC)
                CALL(FSYS, LCP);
            else
                ASSIGNMENT(FSYS, LCP);
            break;
        case BEGINSY:
            INSYMBOL();
            COMPOUNDSTATEMENT(FSYS);
            break;
        case GOTOSY:
            INSYMBOL();
            GOTOSTATEMENT();
            break;
        case IFSY:
            INSYMBOL();
            IFSTATEMENT(FSYS);
            break;
        case CASESY:
            INSYMBOL();
            CASESTATEMENT(FSYS);
            break;
        case WHILESY:
            INSYMBOL();
            WHILESTATEMENT(FSYS);
            break;
        case REPEATSY:
            INSYMBOL();
            REPEATSTATEMENT(FSYS);
            break;
        case FORSY:
            INSYMBOL();
            FORSTATEMENT(FSYS);
            break;
        case WITHSY:
            INSYMBOL();
            WITHSTATEMENT(FSYS);
            break;
        }
        PRELEASE(HEAP);
        if (IC + 100 > MAXCODE) {
            ERROR(253);
            IC = 0;
        }
        if (!(SY == SEMICOLON || SY == ENDSY || SY == ELSESY || SY == UNTILSY)) {
            ERROR(6);
            SKIP(FSYS);
        }
    }
    STMTLEV = STMTLEV - 1;
}

static void BODY(SETOFSYS *FSYS, CTP FPROCP)
{
    ADDRRANGE LLC1, EXITIC;
    CTP LCP;
    OPRANGE LOP;
    LABELP LLP;
    INTEGER LMIN, LMAX, AVAIL;
    JTABRANGE JTINX;
    STP T;
    if (NOSWAP && STARTINGUP) {
        DECLARATIONPART(FSYS);                  /* (bring in DECLARATIONPART) */
        return;                                 /* EXIT(BODYPART) */
    }
    NEXTJTAB = 1;
    if (NOISY) {
        WLN(NULL);
        /* (IF NOT NOSWAP THEN UNITWRITE(3, DUMMYVAR[-1600], 35): shows the stack
           and heap on a graphics screen, unit 3 -- not here) */
        AVAIL = MEMAVAIL();
        if (AVAIL < SMALLESTSPACE)
            SMALLESTSPACE = AVAIL;
        if (FPROCP != NULL) {
            WPA(NULL, FPROCP->NAME, 8);
            WSTR(NULL, " [");
            WINT(NULL, AVAIL, 5);
            WSTR(NULL, " words]");
            WLN(NULL);
        }
        WSTR(NULL, "<");
        WINT(NULL, SCREENDOTS, 4);
        WSTR(NULL, ">");
    }
    if (FPROCP != NULL) {
        LLC1 = FPROCP->U.P.LOCALLC;
        LCP = FPROCP->NEXT;
        while (LCP != NULL) {
            if (LCP->IDTYPE != NULL) {
                if (LCP->KLASS == ACTUALVARS) {
                    if (LCP->IDTYPE->FORM > POWER) {
                        LLC1 = LLC1 - PTRSIZE;
                        GEN2(50, 0, LCP->U.V.VADDR);    /* LDA */
                        GEN2(54, 0, LLC1);              /* LOD */
                        if (PAOFCHAR(LCP->IDTYPE)) {
                            T = LCP->IDTYPE;
                            if (T->U.AR.AISSTRNG)
                                GEN1(42, T->U.AR.MAXLENG);  /* SAS */
                            else if (T->U.AR.INXTYPE != NULL) {
                                GETBOUNDS(T->U.AR.INXTYPE, &LMIN, &LMAX);
                                GEN1(40, DIV(LMAX - LMIN + 1 + 1, 2));    /* MOV */
                            }
                        } else
                            GEN1(40, LCP->IDTYPE->SIZE);    /* MOV */
                    } else
                        LLC1 = LLC1 - LCP->IDTYPE->SIZE;
                } else if (LCP->KLASS == FORMALVARS)
                    LLC1 = LLC1 - PTRSIZE;
            }
            LCP = LCP->NEXT;
        }
    }
    STARTDOTS = SCREENDOTS;
    LCMAX = LC;
    LLP = DISPLAY[TOP].FLABEL;
    while (LLP != NULL) {
        GENLABEL(&LLP->CODELBP);
        LLP = LLP->NEXTLAB;
    }
    if (!INMODULE)
        if (LEVEL == 1) {
            LCP = USINGLIST;
            while (LCP != NULL) {
                if (LCP->U.SEGID >= 0) {
                    GENLDC(LCP->U.SEGID);
                    GEN1(30, 21);               /* CSP GETSEG */
                }
                LCP = LCP->NEXT;
            }
            if (USERINFO.STUPID)
                GEN2(77, 6, 1);                 /* CXP TURTLE INIT */
        }
    LCP = DISPLAY[TOP].FFILE;
    while (LCP != NULL) {
        T = LCP->IDTYPE;                        /* WITH LCP^, IDTYPE^ */
        GEN2(50, 0, LCP->U.V.VADDR);            /* LDA */
        GEN2(50, 0, LCP->U.V.VADDR + FILESIZE); /* LDA */
        if (T->U.FILTYPE == NULL)
            GENLDC(-1);
        else if (LCP->IDTYPE == INTRACTVPTR)
            GENLDC(0);
        else if (T->U.FILTYPE == CHARPTR)
            GENLDC(-2);
        else
            GENLDC(T->U.FILTYPE->SIZE);
        GEN2(77, 0, 3);                         /* CXP SYS FINIT */
        LCP = LCP->NEXT;
    }
    if (LEVEL == 1 && !SYSCOMP)
        GEN1(85, SCREENDOTS + 1);               /* BPT */
    STATEMENTS(FSYS, ENDSY);
    if (SY == ENDSY)
        INSYMBOL();
    else
        ERROR(13);
    EXITIC = IC;
    LCP = DISPLAY[TOP].FFILE;
    while (LCP != NULL) {
        GEN2(50, 0, LCP->U.V.VADDR);            /* LDA */
        GENLDC(0);
        GEN2(77, 0, 6);                         /* CXP SYS FCLOSE */
        LCP = LCP->NEXT;
    }
    if (!INMODULE)
        if (LEVEL == 1) {
            LCP = USINGLIST;
            while (LCP != NULL) {
                if (LCP->U.SEGID >= 0) {
                    GENLDC(LCP->U.SEGID);
                    GEN1(30, 22);               /* CSP RELSEG */
                }
                LCP = LCP->NEXT;
            }
        }
    if (FPROCP == NULL)
        GEN0(86);                               /* XIT */
    else {
        if (FPROCP->U.P.PFLEV == 0)
            LOP = 65;                           /* RBP */
        else
            LOP = 45;                           /* RNP */
        if (FPROCP->IDTYPE == NULL)
            GEN1(LOP, 0);
        else
            GEN1(LOP, FPROCP->IDTYPE->SIZE);
    }
    LLP = DISPLAY[TOP].FLABEL;                  /* check for undefined labels */
    while (LLP != NULL) {
        if (!LLP->CODELBP->DEFINED)
            if (LLP->CODELBP->REFLIST != MAXADDR)
                ERROR(168);
        LLP = LLP->NEXTLAB;
    }
    JTINX = NEXTJTAB - 1;
    if (ODD(IC))
        IC = IC + 1;
    while (JTINX > 0) {
        GENWORD(IC - JTAB[JTINX]);
        JTINX = JTINX - 1;
    }
    if (FPROCP == NULL) {
        GENWORD((LCMAX - LCAFTERMARKSTACK) * 2);
        GENWORD(0);
    } else {
        GENWORD((LCMAX - FPROCP->U.P.LOCALLC) * 2);
        GENWORD((FPROCP->U.P.LOCALLC - LCAFTERMARKSTACK) * 2);
    }
    GENWORD(IC - EXITIC);
    GENWORD(IC);
    GENBYTE(CURPROC);
    GENBYTE(LEVEL - 1);
    if (!CODEINSEG) {
        CODEINSEG = TRUE;
        SEGTABLE[SEG].DISKADDR = CURBLK;
    }
    WRITECODE(FALSE);
    SEGINX = SEGINX + IC;
    PROCTABLE[CURPROC] = SEGINX - 2;
}

void BODYPART(SETOFSYS *FSYS, CTP FPROCP)
{
    BODY(FSYS, FPROCP);
}
