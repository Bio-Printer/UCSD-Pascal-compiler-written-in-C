/* decpart.c -- DECPART.A.TEXT, DECPART.B.TEXT, DECPART.C.TEXT: the segment
   procedure DECLARATIONPART -- types, USES, labels, constants, types,
   variables, procedure headings.

   Pascal's nested procedures use their parents' variables, and they recurse
   (TYP -> FIELDLIST -> TYP; DECLARATIONPART -> USESDECLARATION -> DECLARATIONPART),
   so each parent's variables are a struct (DECCTX, TYPCTX, FLCTX) whose address
   its nested procedures get. */
#include "compglbl.h"
#include "pcio.h"

#pragma segment DECLARAT

struct DECCTX {                         /* DECLARATIONPART's */
    SETOFSYS *FSYS;
    SYMBOL LSY;
    BOOLEAN NOTDONE;
};

#define ALPHAEQ(a, b) (memcmp(a, b, 8) == 0)

/* ======================== DECPART.A: TYP ======================== */

struct TYPCTX {                         /* TYP's */
    struct DECCTX *d;
    STP *FSP;
    STP LSP, LSP1, LSP2;
    DISPRANGE OLDTOP;
    CTP LCP;
    ADDRRANGE LSIZE, DISPL;
    INTEGER LMIN, LMAX;
    BOOLEAN PACKING;
    BITRANGE NEXTBIT, NUMBITS;
};

static void TYP(struct DECCTX *d, SETOFSYS *FSYS, STP *FSP, ADDRRANGE *FSIZE);

static void SIMPLETYPE(SETOFSYS *FSYS, STP *FSP, ADDRRANGE *FSIZE)
{
    STP LSP, LSP1;
    CTP LCP, LCP1;
    DISPRANGE TTOP;
    INTEGER LCNT;
    VALU LVALU;
    SETS s, t;
    LSP = NULL;
    *FSIZE = 1;
    if (!IN(SY, &SIMPTYPEBEGSYS)) {
        ERROR(1);
        SKIP(SETPLUS(&s, FSYS, &SIMPTYPEBEGSYS));
    }
    if (IN(SY, &SIMPTYPEBEGSYS)) {
        if (SY == LPARENT) {
            TTOP = TOP;
            while (DISPLAY[TOP].OCCUR != BLCK)
                TOP = TOP - 1;
            LSP = NEWTYPE(SC);                    /* NEW(LSP, SCALAR, DECLARED) */
            LSP->SIZE = INTSIZE;
            LSP->FORM = SCALAR;
            LSP->U.SC.SCALKIND = DECLARED;
            LCP1 = NULL;
            LCNT = 0;
            do {
                INSYMBOL();
                if (SY == IDENT) {
                    LCP = NEWID(VALUES);              /* NEW(LCP, KONST) */
                    memcpy(LCP->NAME, ID, 8);
                    LCP->IDTYPE = LSP;
                    LCP->NEXT = LCP1;
                    LCP->U.VALUES.IVAL = LCNT;
                    LCP->KLASS = KONST;
                    ENTERID(LCP);
                    LCNT = LCNT + 1;
                    LCP1 = LCP;
                    INSYMBOL();
                } else
                    ERROR(2);
                if (!IN(SY, SETADD(&s, FSYS, COMMA, RPARENT, -1))) {
                    ERROR(6);
                    SKIP(&s);
                }
            } while (SY == COMMA);
            LSP->U.SC.FCONST = LCP1;
            TOP = TTOP;
            if (SY == RPARENT)
                INSYMBOL();
            else
                ERROR(4);
        } else {
            if (SY == IDENT) {
                SEARCHID(SETOF(&s, TYPES, KONST, -1), &LCP);
                INSYMBOL();
                if (LCP->KLASS == KONST) {
                    LSP = NEWTYPE(SR);            /* NEW(LSP, SUBRANGE) */
                    LSP->U.SR.RANGETYPE = LCP->IDTYPE;
                    LSP->FORM = SUBRANGE;
                    if (STRGTYPE(LSP->U.SR.RANGETYPE)) {
                        ERROR(148);
                        LSP->U.SR.RANGETYPE = NULL;
                    }
                    LSP->U.SR.MIN = LCP->U.VALUES;
                    LSP->SIZE = INTSIZE;
                    if (SY == COLON)
                        INSYMBOL();
                    else
                        ERROR(5);
                    CONSTANT(FSYS, &LSP1, &LVALU);
                    LSP->U.SR.MAX = LVALU;
                    if (LSP->U.SR.RANGETYPE != LSP1)
                        ERROR(107);
                } else {
                    LSP = LCP->IDTYPE;
                    if (LSP == STRGPTR && SY == LBRACK) {
                        INSYMBOL();
                        CONSTANT(SETADD(&s, FSYS, RBRACK, -1), &LSP1, &LVALU);
                        if (LSP1 == INTPTR) {
                            if (LVALU.IVAL <= 0 || LVALU.IVAL > STRGLGTH) {
                                ERROR(203);
                                LVALU.IVAL = DEFSTRGLGTH;
                            }
                            if (LVALU.IVAL != DEFSTRGLGTH) {
                                LSP = NEWTYPE(AR);    /* NEW(LSP, ARRAYS, TRUE, TRUE) */
                                *LSP = *STRGPTR;
                                LSP->U.AR.MAXLENG = LVALU.IVAL;
                                LSP->SIZE = (LVALU.IVAL + CHRSPERWD) / CHRSPERWD;
                            }
                        } else
                            ERROR(15);
                        if (SY == RBRACK)
                            INSYMBOL();
                        else
                            ERROR(12);
                    } else if (LSP == INTPTR) {
                        /* (the Pascal's ELSE below belongs to IF SY = LBRACK: the FILES
                           test is under LSP = INTPTR, so it is never true) */
                        if (SY == LBRACK) {
                            INSYMBOL();
                            LSP = NEWTYPE0();        /* NEW(LSP, LONGINT) */
                            *LSP = *LONGINTPTR;
                            CONSTANT(SETADD(&s, FSYS, RBRACK, -1), &LSP1, &LVALU);
                            if (LSP1 == INTPTR) {
                                if (LVALU.IVAL <= 0 || LVALU.IVAL > MAXDEC)
                                    ERROR(203);
                                else
                                    LSP->SIZE = DECSIZE(LVALU.IVAL);
                            } else
                                ERROR(15);
                            if (SY == RBRACK)
                                INSYMBOL();
                            else
                                ERROR(12);
                        } else if (LSP->FORM == FILES) {
                            if (INMODULE)
                                if (!ININTERFACE)
                                    ERROR(191);     /* no private files */
                        }
                    }
                    if (LSP != NULL)
                        *FSIZE = LSP->SIZE;
                }
            } else {                            /* (SY = IDENT) */
                LSP = NEWTYPE(SR);                /* NEW(LSP, SUBRANGE) */
                LSP->FORM = SUBRANGE;
                CONSTANT(SETADD(&s, FSYS, COLON, -1), &LSP1, &LVALU);
                if (STRGTYPE(LSP1)) {
                    ERROR(148);
                    LSP1 = NULL;
                }
                LSP->U.SR.RANGETYPE = LSP1;
                LSP->U.SR.MIN = LVALU;
                LSP->SIZE = INTSIZE;
                if (SY == COLON)
                    INSYMBOL();
                else
                    ERROR(5);
                CONSTANT(FSYS, &LSP1, &LVALU);
                LSP->U.SR.MAX = LVALU;
                if (LSP->U.SR.RANGETYPE != LSP1)
                    ERROR(107);
            }
            if (LSP != NULL)
                if (LSP->FORM == SUBRANGE)
                    if (LSP->U.SR.RANGETYPE != NULL) {
                        if (LSP->U.SR.RANGETYPE == REALPTR)
                            ERROR(399);
                        else if (LSP->U.SR.MIN.IVAL > LSP->U.SR.MAX.IVAL) {
                            ERROR(102);
                            LSP->U.SR.MAX.IVAL = LSP->U.SR.MIN.IVAL;
                        }
                    }
        }
        *FSP = LSP;
        if (!IN(SY, FSYS)) {
            ERROR(6);
            SKIP(FSYS);
        }
    } else
        *FSP = NULL;
    (void)t;
}

static BOOLEAN PACKABLE(struct TYPCTX *t, STP FSP)
{
    INTEGER LMIN, LMAX;
    BOOLEAN r = FALSE;
    if (FSP != NULL && t->PACKING)
        switch (FSP->FORM) {
        case SUBRANGE:
        case SCALAR:
            if (FSP != INTPTR && FSP != REALPTR) {
                GETBOUNDS(FSP, &LMIN, &LMAX);
                if (LMIN >= 0) {
                    r = TRUE;
                    t->NUMBITS = 1;
                    LMIN = 1;
                    while (LMIN < LMAX) {
                        LMIN = LMIN + 1;
                        LMIN = LMIN + LMIN - 1;
                        t->NUMBITS = t->NUMBITS + 1;
                    }
                }
            }
            break;
        case POWER:
            if (PACKABLE(t, FSP->U.ELSET)) {
                GETBOUNDS(FSP->U.ELSET, &LMIN, &LMAX);
                LMAX = LMAX + 1;
                if (LMAX < BITSPERWD) {
                    r = TRUE;
                    t->NUMBITS = LMAX;
                }
            }
            break;
        }
    return r;
}

struct FLCTX {                          /* FIELDLIST's */
    struct TYPCTX *t;
    SETOFSYS *FSYS;
    STP *FRECVAR;
    CTP LCP, LCP1, NXT, NXT1, LAST;
    STP LSP, LSP1, LSP2, LSP3, LSP4;
    ADDRRANGE MINSIZE, MAXSIZE, LSIZE;
    VALU LVALU;
    BITRANGE MAXBIT, MINBIT;
};

static void FIELDLIST(struct TYPCTX *t, SETOFSYS *FSYS, STP *FRECVAR);

static void ALLOCATE(struct FLCTX *f, CTP FCP)
{
    struct TYPCTX *t = f->t;
    BOOLEAN ONBOUND;
    ONBOUND = FALSE;
    if (PACKABLE(t, FCP->IDTYPE)) {
        if (t->NUMBITS + t->NEXTBIT > BITSPERWD) {
            t->DISPL = t->DISPL + 1;
            t->NEXTBIT = 0;
            ONBOUND = TRUE;
        }
        FCP->U.F.FLDADDR = t->DISPL;
        FCP->U.F.FISPACKD = TRUE;
        FCP->U.F.FLDWIDTH = t->NUMBITS;
        FCP->U.F.FLDRBIT = t->NEXTBIT;
        t->NEXTBIT = t->NEXTBIT + t->NUMBITS;
    } else {
        t->DISPL = t->DISPL + (t->NEXTBIT > 0);
        t->NEXTBIT = 0;
        ONBOUND = TRUE;
        FCP->U.F.FISPACKD = FALSE;
        FCP->U.F.FLDADDR = t->DISPL;
        if (FCP->IDTYPE != NULL)
            t->DISPL = t->DISPL + FCP->IDTYPE->SIZE;
    }
    if (ONBOUND && f->LAST != NULL) {
        CTP L = f->LAST;
        if (L->U.F.FISPACKD) {
            if (L->U.F.FLDRBIT == 0)
                L->U.F.FISPACKD = FALSE;
            else if (L->U.F.FLDWIDTH <= 8 && L->U.F.FLDRBIT <= 8) {
                L->U.F.FLDWIDTH = 8;
                L->U.F.FLDRBIT = 8;
            }
        }
    }
}

static void VARIANTLIST(struct FLCTX *f)
{
    struct TYPCTX *t = f->t;
    BOOLEAN GOTTAGNAME;
    SETS s, u;
    f->LSP = NEWTYPE(TF);                         /* NEW(LSP, TAGFLD) */
    f->LSP->U.TF.TAGFIELDP = NULL;
    f->LSP->U.TF.FSTVAR = NULL;
    f->LSP->FORM = TAGFLD;
    *f->FRECVAR = f->LSP;
    INSYMBOL();
    if (SY == IDENT) {
        f->LCP = f->t->PACKING ? NEWID(F) : NEWIDN(2);    /* NEW(LCP, FIELD, PACKING) */
        f->LCP->IDTYPE = NULL;
        f->LCP->KLASS = FIELD;
        f->LCP->NEXT = NULL;
        f->LCP->U.F.FISPACKD = FALSE;
        GOTTAGNAME = FALSE;
        PRTERR = FALSE;
        SEARCHID(SETOF(&s, TYPES, -1), &f->LCP1);
        PRTERR = TRUE;
        if (f->LCP1 == NULL) {
            GOTTAGNAME = TRUE;
            memcpy(f->LCP->NAME, ID, 8);
            ENTERID(f->LCP);
            INSYMBOL();
            if (SY == COLON)
                INSYMBOL();
            else
                ERROR(5);
        }
        if (SY == IDENT) {
            SEARCHID(SETOF(&s, TYPES, -1), &f->LCP1);
            f->LSP1 = f->LCP1->IDTYPE;
            if (f->LSP1 != NULL) {
                if (f->LSP1->FORM <= SUBRANGE) {
                    if (COMPTYPES(REALPTR, f->LSP1))
                        ERROR(109);
                    f->LCP->IDTYPE = f->LSP1;
                    f->LSP->U.TF.TAGFIELDP = f->LCP;
                    if (GOTTAGNAME)
                        ALLOCATE(f, f->LCP);
                } else
                    ERROR(110);
            }
            INSYMBOL();
        } else {
            ERROR(2);
            SKIP(SETADD(&s, f->FSYS, OFSY, LPARENT, -1));
        }
    } else {
        ERROR(2);
        SKIP(SETADD(&s, f->FSYS, OFSY, LPARENT, -1));
    }
    f->LSP->SIZE = t->DISPL + (t->NEXTBIT > 0);
    if (SY == OFSY)
        INSYMBOL();
    else
        ERROR(8);
    f->LSP1 = NULL;
    f->MINSIZE = t->DISPL;
    f->MAXSIZE = t->DISPL;
    f->MINBIT = t->NEXTBIT;
    f->MAXBIT = t->NEXTBIT;
    do {
        f->LSP2 = NULL;
        do {
            CONSTANT(SETADD(&s, f->FSYS, COMMA, COLON, LPARENT, -1), &f->LSP3, &f->LVALU);
            if (f->LSP->U.TF.TAGFIELDP != NULL)
                if (!COMPTYPES(f->LSP->U.TF.TAGFIELDP->IDTYPE, f->LSP3))
                    ERROR(111);
            f->LSP3 = NEWTYPE(VA);                /* NEW(LSP3, VARIANT) */
            f->LSP3->U.VA.NXTVAR = f->LSP1;
            f->LSP3->U.VA.SUBVAR = f->LSP2;
            f->LSP3->U.VA.VARVAL = f->LVALU;
            f->LSP3->FORM = VARIANT;
            f->LSP1 = f->LSP3;
            f->LSP2 = f->LSP3;
            TEST = SY != COMMA;
            if (!TEST)
                INSYMBOL();
        } while (!TEST);
        if (SY == COLON)
            INSYMBOL();
        else
            ERROR(5);
        if (SY == LPARENT)
            INSYMBOL();
        else
            ERROR(9);
        if (SY == RPARENT)
            f->LSP2 = NULL;
        else
            FIELDLIST(t, SETADD(&s, f->FSYS, RPARENT, SEMICOLON, -1), &f->LSP2);
        if (t->DISPL > f->MAXSIZE) {
            f->MAXSIZE = t->DISPL;
            f->MAXBIT = t->NEXTBIT;
        } else if (t->DISPL == f->MAXSIZE && t->NEXTBIT > f->MAXBIT)
            f->MAXBIT = t->NEXTBIT;
        while (f->LSP3 != NULL) {
            f->LSP4 = f->LSP3->U.VA.SUBVAR;
            f->LSP3->U.VA.SUBVAR = f->LSP2;
            f->LSP3->SIZE = t->DISPL + (t->NEXTBIT > 0);
            f->LSP3 = f->LSP4;
        }
        if (SY == RPARENT) {
            INSYMBOL();
            if (!IN(SY, SETADD(&u, f->FSYS, SEMICOLON, -1))) {
                ERROR(6);
                SKIP(&u);
            }
        } else
            ERROR(4);
        TEST = SY != SEMICOLON;
        if (!TEST) {
            INSYMBOL();
            t->DISPL = f->MINSIZE;
            t->NEXTBIT = f->MINBIT;
        }
    } while (!(TEST || SY == ENDSY));           /* (<<<< SMF 2-28-78) */
    t->DISPL = f->MAXSIZE;
    t->NEXTBIT = f->MAXBIT;
    f->LSP->U.TF.FSTVAR = f->LSP1;
}

static void FIELDLIST(struct TYPCTX *t, SETOFSYS *FSYS, STP *FRECVAR)
{
    struct FLCTX fl, *f = &fl;
    SETS s;
    memset(f, 0, sizeof fl);
    f->t = t;
    f->FSYS = FSYS;
    f->FRECVAR = FRECVAR;
    f->NXT1 = NULL;
    f->LSP = NULL;
    f->LAST = NULL;
    if (!(SY == IDENT || SY == CASESY)) {
        ERROR(19);
        SKIP(SETADD(&s, FSYS, IDENT, CASESY, -1));
    }
    while (SY == IDENT) {
        f->NXT = f->NXT1;
        do {
            if (SY == IDENT) {
                f->LCP = f->t->PACKING ? NEWID(F) : NEWIDN(2);    /* NEW(LCP, FIELD, PACKING) */
                memcpy(f->LCP->NAME, ID, 8);
                f->LCP->IDTYPE = NULL;
                f->LCP->NEXT = f->NXT;
                f->LCP->KLASS = FIELD;
                f->LCP->U.F.FISPACKD = FALSE;
                f->NXT = f->LCP;
                ENTERID(f->LCP);
                INSYMBOL();
            } else
                ERROR(2);
            if (!(SY == COMMA || SY == COLON)) {
                ERROR(6);
                SKIP(SETADD(&s, FSYS, COMMA, COLON, SEMICOLON, CASESY, -1));
            }
            TEST = SY != COMMA;
            if (!TEST)
                INSYMBOL();
        } while (!TEST);
        if (SY == COLON)
            INSYMBOL();
        else
            ERROR(5);
        TYP(t->d, SETADD(&s, FSYS, CASESY, SEMICOLON, -1), &f->LSP, &f->LSIZE);
        if (f->LSP != NULL)
            if (f->LSP->FORM == FILES)
                ERROR(108);
        while (f->NXT != f->NXT1) {
            CTP W = f->NXT;                     /* WITH NXT^ DO */
            W->IDTYPE = f->LSP;
            ALLOCATE(f, f->NXT);
            if (W->NEXT == f->NXT1)
                f->LAST = f->NXT;
            f->NXT = W->NEXT;
        }
        f->NXT1 = f->LCP;
        if (SY == SEMICOLON) {
            INSYMBOL();
            if (!(SY == IDENT || SY == ENDSY || SY == CASESY)) {   /* (<<<< SMF 2-28-78) */
                ERROR(19);
                SKIP(SETADD(&s, FSYS, IDENT, CASESY, -1));
            }
        }
    }
    f->NXT = NULL;
    while (f->NXT1 != NULL) {
        CTP W = f->NXT1;                        /* WITH NXT1^ DO */
        f->LCP = W->NEXT;
        W->NEXT = f->NXT;
        f->NXT = f->NXT1;
        f->NXT1 = f->LCP;
    }
    if (SY == CASESY)
        VARIANTLIST(f);
    else
        *FRECVAR = NULL;
}

static void POINTERTYPE(struct TYPCTX *t)
{
    SETS s;
    t->LSP = NEWTYPE(ELTYPE);                         /* NEW(LSP, POINTER) */
    *t->FSP = t->LSP;
    t->LSP->U.ELTYPE = NULL;
    t->LSP->SIZE = PTRSIZE;
    t->LSP->FORM = POINTER;
    INSYMBOL();
    if (SY == IDENT) {
        PRTERR = FALSE;
        SEARCHID(SETOF(&s, TYPES, -1), &t->LCP);
        PRTERR = TRUE;
        if (t->LCP == NULL) {                   /* a forward referenced type id */
            t->LCP = NEWID0();                   /* NEW(LCP, TYPES) */
            memcpy(t->LCP->NAME, ID, 8);
            t->LCP->IDTYPE = t->LSP;
            t->LCP->NEXT = FWPTR;
            t->LCP->KLASS = TYPES;
            FWPTR = t->LCP;
        } else {
            if (t->LCP->IDTYPE != NULL) {
                if (t->LCP->IDTYPE->FORM != FILES || SYSCOMP)
                    t->LSP->U.ELTYPE = t->LCP->IDTYPE;
                else
                    ERROR(108);
            }
        }
        INSYMBOL();
    } else
        ERROR(2);
}

static void TYP(struct DECCTX *d, SETOFSYS *FSYS, STP *FSP, ADDRRANGE *FSIZE)
{
    struct TYPCTX ty, *t = &ty;
    SETS s;
    memset(t, 0, sizeof ty);
    t->d = d;
    t->FSP = FSP;
    t->PACKING = FALSE;
    if (!IN(SY, &TYPEBEGSYS)) {
        ERROR(10);
        SKIP(SETPLUS(&s, FSYS, &TYPEBEGSYS));
    }
    if (IN(SY, &TYPEBEGSYS)) {
        if (IN(SY, &SIMPTYPEBEGSYS))
            SIMPLETYPE(FSYS, FSP, FSIZE);
        else if (SY == ARROW)                   /* ^ */
            POINTERTYPE(t);
        else {
            if (SY == PACKEDSY) {
                INSYMBOL();
                t->PACKING = TRUE;
                if (!IN(SY, &TYPEDELS)) {
                    ERROR(10);
                    SKIP(SETPLUS(&s, FSYS, &TYPEDELS));
                }
            }
            if (SY == ARRAYSY) {                /* ARRAY */
                INSYMBOL();
                if (SY == LBRACK)
                    INSYMBOL();
                else
                    ERROR(11);
                t->LSP1 = NULL;
                do {
                    t->LSP = NEWTYPE(AR);         /* NEW(LSP, ARRAYS, PACKING ...) */
                    t->LSP->U.AR.AELTYPE = t->LSP1;
                    t->LSP->U.AR.INXTYPE = NULL;
                    if (t->PACKING)
                        t->LSP->U.AR.AISSTRNG = FALSE;
                    t->LSP->U.AR.AISPACKD = FALSE;
                    t->LSP->FORM = ARRAYS;
                    t->LSP1 = t->LSP;
                    SIMPLETYPE(SETADD(&s, FSYS, COMMA, RBRACK, OFSY, -1), &t->LSP2, &t->LSIZE);
                    t->LSP1->SIZE = t->LSIZE;
                    if (t->LSP2 != NULL) {
                        if (t->LSP2->FORM <= SUBRANGE) {
                            if (t->LSP2 == REALPTR) {
                                ERROR(109);
                                t->LSP2 = NULL;
                            } else if (t->LSP2 == INTPTR) {
                                ERROR(149);
                                t->LSP2 = NULL;
                            }
                            t->LSP->U.AR.INXTYPE = t->LSP2;
                        } else {
                            ERROR(113);
                            t->LSP2 = NULL;
                        }
                    }
                    TEST = SY != COMMA;
                    if (!TEST)
                        INSYMBOL();
                } while (!TEST);
                if (SY == RBRACK)
                    INSYMBOL();
                else
                    ERROR(12);
                if (SY == OFSY)
                    INSYMBOL();
                else
                    ERROR(8);
                TYP(d, FSYS, &t->LSP, &t->LSIZE);
                if (t->LSP != NULL)
                    if (t->LSP->FORM == FILES)
                        ERROR(108);
                if (PACKABLE(t, t->LSP))
                    if (t->NUMBITS + t->NUMBITS <= BITSPERWD) {
                        t->LSP1->U.AR.AISPACKD = TRUE;
                        t->LSP1->U.AR.ELSPERWD = BITSPERWD / t->NUMBITS;
                        t->LSP1->U.AR.ELWIDTH = t->NUMBITS;
                    }
                do {
                    STP W = t->LSP1;            /* WITH LSP1^ DO */
                    t->LSP2 = W->U.AR.AELTYPE;
                    W->U.AR.AELTYPE = t->LSP;
                    if (W->U.AR.INXTYPE != NULL) {
                        GETBOUNDS(W->U.AR.INXTYPE, &t->LMIN, &t->LMAX);
                        if (W->U.AR.AISPACKD)
                            t->LSIZE = (t->LMAX - t->LMIN + W->U.AR.ELSPERWD) / W->U.AR.ELSPERWD;
                        else
                            t->LSIZE = t->LSIZE * (t->LMAX - t->LMIN + 1);
                        if (t->LSIZE <= 0) {
                            ERROR(398);
                            t->LSIZE = 1;
                        }
                        W->SIZE = t->LSIZE;
                    }
                    t->LSP = t->LSP1;
                    t->LSP1 = t->LSP2;
                } while (t->LSP1 != NULL);
            } else if (SY == RECORDSY) {        /* RECORD */
                INSYMBOL();
                t->OLDTOP = TOP;
                if (TOP < DISPLIMIT) {
                    TOP = TOP + 1;
                    DISPLAY[TOP].FNAME = NULL;
                    DISPLAY[TOP].OCCUR = REC;
                } else
                    ERROR(250);
                t->DISPL = 0;
                t->NEXTBIT = 0;
                {
                    SETS a, b;
                    SETOF(&a, SEMICOLON, -1);
                    FIELDLIST(t, SETADD(&b, SETMINUS(&s, FSYS, &a), ENDSY, -1), &t->LSP1);
                }
                t->DISPL = t->DISPL + (t->NEXTBIT > 0);
                t->LSP = NEWTYPE(RE);             /* NEW(LSP, RECORDS) */
                t->LSP->U.RE.FSTFLD = DISPLAY[TOP].FNAME;
                t->LSP->U.RE.RECVAR = t->LSP1;
                t->LSP->SIZE = t->DISPL;
                t->LSP->FORM = RECORDS;
                TOP = t->OLDTOP;
                if (SY == ENDSY)
                    INSYMBOL();
                else
                    ERROR(13);
            } else if (SY == SETSY) {           /* SET */
                INSYMBOL();
                if (SY == OFSY)
                    INSYMBOL();
                else
                    ERROR(8);
                SIMPLETYPE(FSYS, &t->LSP1, &t->LSIZE);
                if (t->LSP1 != NULL) {
                    if (t->LSP1->FORM > SUBRANGE || t->LSP1 == INTPTR || t->LSP1 == REALPTR) {
                        ERROR(115);
                        t->LSP1 = NULL;
                    } else if (t->LSP1 == REALPTR) {
                        ERROR(114);
                        t->LSP1 = NULL;
                    }
                }
                t->LSP = NEWTYPE(ELSET);             /* NEW(LSP, POWER) */
                t->LSP->U.ELSET = t->LSP1;
                t->LSP->FORM = POWER;
                if (t->LSP1 != NULL) {
                    GETBOUNDS(t->LSP1, &t->LMIN, &t->LMAX);
                    t->LSP->SIZE = (t->LMAX + BITSPERWD) / BITSPERWD;
                    if (t->LSP->SIZE > 255) {
                        ERROR(169);
                        t->LSP->SIZE = 1;
                    }
                } else
                    t->LSP->SIZE = 0;
            } else if (SY == FILESY) {          /* FILE */
                if (INMODULE)
                    if (!ININTERFACE)
                        ERROR(191);             /* no private files */
                INSYMBOL();
                t->LSP = NEWTYPE(FILTYPE);             /* NEW(LSP, FILES) */
                t->LSP->FORM = FILES;
                t->LSP->U.FILTYPE = NULL;
                if (SY == OFSY) {
                    INSYMBOL();
                    TYP(d, FSYS, &t->LSP1, &t->LSIZE);
                } else
                    t->LSP1 = NULL;
                t->LSP->U.FILTYPE = t->LSP1;
                if (t->LSP1 != NULL)
                    t->LSP->SIZE = FILESIZE + t->LSP1->SIZE;
                else
                    t->LSP->SIZE = NILFILESIZE;
            }
            *FSP = t->LSP;
        }
        if (!IN(SY, FSYS)) {
            ERROR(6);
            SKIP(FSYS);
        }
    } else
        *FSP = NULL;
    if (*FSP == NULL)
        *FSIZE = 1;
    else
        *FSIZE = (*FSP)->SIZE;
}

/* ======================== DECPART.B ======================== */

/* the library's segment dictionary (block 0 of a code file) */
#define SD_SEGNAME(b, i) ((b) + 64 + 8 * (i))
#define SD_WORD(b, off) ((INTEGER)((b)[off] | ((b)[(off) + 1] << 8)))
#define SD_SEGKIND(b, i) SD_WORD(b, 192 + 2 * (i))
#define SD_TEXTADDR(b, i) SD_WORD(b, 224 + 2 * (i))

struct USESCTX {                        /* USESDECLARATION's (what GETTEXT uses) */
    BOOLEAN MAGIC;
    CHAR SEGDICT[512];
    INTEGER BEGADDR;
    ALPHA LNAME;
};

static void GETTEXT(struct USESCTX *u, BOOLEAN *FOUND)
{
    CTP LCP;
    INTEGER SEGINDEX;
    *FOUND = FALSE;
    LCP = MODPTR;
    while (LCP != NULL && !*FOUND)
        if (ALPHAEQ(LCP->NAME, ID))
            *FOUND = TRUE;
        else
            LCP = LCP->NEXT;
    if (*FOUND) {
        LSEPPROC = SEGTABLE[LCP->U.SEGID].SEGKIND == 4;
        if (!LSEPPROC) {
            SEG = LCP->U.SEGID;
            NEXTPROC = 1;
        }
        u->BEGADDR = SEGTABLE[LCP->U.SEGID].TEXTADDR;
        USEFILE = WORKCODE;
    } else {
        *FOUND = TRUE;
        if (LIBNOTOPEN) {
            LIBRARY = POPENOLD(SYSTEMLIB);      /* RESET(LIBRARY, SYSTEMLIB) */
            if (!LIBRARY) {
                ERROR(187);
                *FOUND = FALSE;
            } else if (PBLOCKREAD(LIBRARY, u->SEGDICT, 1, 0) != 1) {
                ERROR(187);
                *FOUND = FALSE;
            }
        }
        if (*FOUND) {
            LIBNOTOPEN = FALSE;
            SEGINDEX = 0;
            *FOUND = FALSE;
            while (SEGINDEX <= MAXSEG && !*FOUND)
                if (u->MAGIC) {
                    if (ALPHAEQ(SD_SEGNAME(u->SEGDICT, SEGINDEX), u->LNAME))
                        *FOUND = TRUE;
                    else
                        SEGINDEX = SEGINDEX + 1;
                } else if (ALPHAEQ(SD_SEGNAME(u->SEGDICT, SEGINDEX), ID))
                    *FOUND = TRUE;
                else
                    SEGINDEX = SEGINDEX + 1;
            if (*FOUND) {
                USEFILE = SYSLIBRARY;
                u->BEGADDR = SD_TEXTADDR(u->SEGDICT, SEGINDEX);
                LSEPPROC = SD_SEGKIND(u->SEGDICT, SEGINDEX) == 4;
                if (!LSEPPROC) {
                    if (u->MAGIC)
                        SEG = 6;
                    else {
                        SEG = NEXTSEG;
                        NEXTSEG = NEXTSEG + 1;
                        if (NEXTSEG > MAXSEG)
                            ERROR(250);
                    }
                    SEGTABLE[SEG].DISKADDR = 0;
                    SEGTABLE[SEG].CODELENG = 0;
                    memcpy(SEGTABLE[SEG].SEGNAME, SD_SEGNAME(u->SEGDICT, SEGINDEX), 8);
                    if (INMODULE || u->MAGIC)
                        SEGTABLE[SEG].SEGKIND = 0;
                    else
                        SEGTABLE[SEG].SEGKIND = SD_SEGKIND(u->SEGDICT, SEGINDEX);
                    SEGTABLE[SEG].TEXTADDR = 0;
                    NEXTPROC = 1;
                }
            } else
                ERROR(190);                     /* not in the library */
        }
    }
    if (u->BEGADDR == 0) {
        ERROR(195);
        *FOUND = FALSE;
    }
    if (*FOUND) {
        USING = TRUE;
        PREVSYMCURSOR = SYMCURSOR;
        PREVLINESTART = LINESTART;
        PREVSYMBLK = SYMBLK - 2;
        SYMBLK = u->BEGADDR;
        GETNEXTPAGE();
        INSYMBOL();
    }
}

static void USESDECLARATION(struct DECCTX *d, BOOLEAN MAGIC)
{
    struct USESCTX uc, *u = &uc;
    BOOLEAN FOUND;
    CTP LCP;
    struct LEXSTKREC LLEXSTK;
    SYMBOL LSY = 0;
    OPERATOR LOP = 0;
    ALPHA LID;
    SETS s;
    memset(u, 0, sizeof uc);
    memset(&LLEXSTK, 0, sizeof LLEXSTK);
    memset(LID, ' ', 8);
    u->MAGIC = MAGIC;
    if (LEVEL != 1)
        ERROR(189);
    if (INMODULE && !ININTERFACE)
        ERROR(192);
    if (!MAGIC)
        DLINKERINFO = TRUE;
    if (!USING)
        USINGLIST = NULL;
    do {
        if (!MAGIC && SY != IDENT)
            ERROR(2);
        else if (USING) {
            LCP = USINGLIST;
            while (LCP != NULL)
                if (ALPHAEQ(LCP->NAME, ID))
                    goto l1;
                else
                    LCP = LCP->NEXT;
            ERROR(188);                         /* the unit must be predeclared in the main program */
        l1:;
        } else {
            if (MAGIC) {
                memcpy(u->LNAME, "TURTLE  ", 8);
                LSY = SY;
                LOP = OP;
                memcpy(LID, ID, 8);
            } else {
                memcpy(u->LNAME, ID, 8);
                WLN(NULL);
                WPA(NULL, ID, 8);
                WSTR(NULL, " [");
                WINT(NULL, MEMAVAIL(), 5);
                WSTR(NULL, " words]");
                WLN(NULL);
                WCH(NULL, '<');
                WINT(NULL, SCREENDOTS, 4);
                WCH(NULL, '>');
            }
            LLEXSTK.DOLDSEG = SEG;
            LLEXSTK.SOLDPROC = NEXTPROC;
            GETTEXT(u, &FOUND);
            if (FOUND) {
                LCP = NEWID(SEGID);                  /* NEW(LCP, MODULE) */
                memcpy(LCP->NAME, u->LNAME, 8);
                LCP->NEXT = USINGLIST;
                LCP->IDTYPE = NULL;
                LCP->KLASS = MODULE;
                if (LSEPPROC)
                    LCP->U.SEGID = -1;          /* no segment */
                else
                    LCP->U.SEGID = SEG;
                ENTERID(LCP);
                USINGLIST = LCP;
                DECLARATIONPART(SETADD(&s, d->FSYS, ENDSY, -1));
                if (NEXTPROC == 1)              /* no procedures declared */
                    LCP->U.SEGID = -1;
                SYMBLK = 9999;                  /* (force a return to the source file) */
                GETNEXTPAGE();
            }
            if (!LSEPPROC) {
                SEG = LLEXSTK.DOLDSEG;
                NEXTPROC = LLEXSTK.SOLDPROC;
            }
            LSEPPROC = FALSE;
        }
        if (!MAGIC) {
            INSYMBOL();
            TEST = SY != COMMA;
            if (TEST) {
                if (SY != SEMICOLON)
                    ERROR(20);
            } else
                INSYMBOL();
        }
    } while (!(TEST || MAGIC));
    if (!MAGIC) {
        if (SY == SEMICOLON)
            INSYMBOL();
        else
            ERROR(14);
    } else {
        SY = LSY;
        OP = LOP;
        memcpy(ID, LID, 8);
    }
    if (!USING) {
        if (INMODULE)
            USINGLIST = NULL;
        if (LIBRARY)
            PCLOSE(LIBRARY, TRUE);              /* CLOSE(LIBRARY, LOCK) */
        LIBRARY = NULL;
        LIBNOTOPEN = TRUE;
    }
}

static void LABELDECLARATION(struct DECCTX *d)
{
    LABELP LLP;
    BOOLEAN REDEF;
    SETS s;
    do {
        if (SY == INTCONST) {
            struct DISPREC *W = &DISPLAY[TOP];  /* WITH DISPLAY[TOP] DO */
            LLP = W->FLABEL;
            REDEF = FALSE;
            while (LLP != NULL && !REDEF)
                if (LLP->LABVAL != VAL.IVAL)
                    LLP = LLP->NEXTLAB;
                else {
                    REDEF = TRUE;
                    ERROR(166);
                }
            if (!REDEF) {
                LLP = PNEW(sizeof(struct USERLABEL));
                LLP->LABVAL = VAL.IVAL;
                LLP->CODELBP = NULL;
                LLP->NEXTLAB = W->FLABEL;
                W->FLABEL = LLP;
            }
            INSYMBOL();
        } else
            ERROR(15);
        if (!IN(SY, SETADD(&s, d->FSYS, COMMA, SEMICOLON, -1))) {
            ERROR(6);
            SKIP(&s);
        }
        TEST = SY != COMMA;
        if (!TEST)
            INSYMBOL();
    } while (!TEST);
    if (SY == SEMICOLON)
        INSYMBOL();
    else
        ERROR(14);
}

static void CONSTDECLARATION(struct DECCTX *d)
{
    CTP LCP;
    STP LSP;
    VALU LVALU;
    SETS s;
    if (SY != IDENT) {
        ERROR(2);
        SKIP(SETADD(&s, d->FSYS, IDENT, -1));
    }
    while (SY == IDENT) {
        LCP = NEWID(VALUES);                          /* NEW(LCP, KONST) */
        memcpy(LCP->NAME, ID, 8);
        LCP->IDTYPE = NULL;
        LCP->NEXT = NULL;
        LCP->KLASS = KONST;
        INSYMBOL();
        if (SY == RELOP && OP == EQOP)
            INSYMBOL();
        else
            ERROR(16);
        CONSTANT(SETADD(&s, d->FSYS, SEMICOLON, -1), &LSP, &LVALU);
        ENTERID(LCP);
        LCP->IDTYPE = LSP;
        LCP->U.VALUES = LVALU;
        if (SY == SEMICOLON) {
            INSYMBOL();
            if (!IN(SY, SETADD(&s, d->FSYS, IDENT, -1))) {
                ERROR(6);
                SKIP(&s);
            }
        } else if (!(SY == ENDSY && INMODULE))
            ERROR(14);
    }
}

static void TYPEDECLARATION(struct DECCTX *d)
{
    CTP LCP, LCP1, LCP2;
    STP LSP;
    ADDRRANGE LSIZE;
    SETS s;
    LCP2 = NULL;
    if (SY != IDENT) {
        ERROR(2);
        SKIP(SETADD(&s, d->FSYS, IDENT, -1));
    }
    while (SY == IDENT) {
        LCP = NEWID0();                          /* NEW(LCP, TYPES) */
        memcpy(LCP->NAME, ID, 8);
        LCP->IDTYPE = NULL;
        LCP->KLASS = TYPES;
        INSYMBOL();
        if (SY == RELOP && OP == EQOP)
            INSYMBOL();
        else
            ERROR(16);
        TYP(d, SETADD(&s, d->FSYS, SEMICOLON, -1), &LSP, &LSIZE);
        ENTERID(LCP);
        LCP->IDTYPE = LSP;
        LCP1 = FWPTR;
        while (LCP1 != NULL) {
            if (ALPHAEQ(LCP1->NAME, LCP->NAME)) {
                LCP1->IDTYPE->U.ELTYPE = LCP->IDTYPE;
                if (LCP1 != FWPTR)
                    LCP2->NEXT = LCP1->NEXT;
                else
                    FWPTR = LCP1->NEXT;
            }
            LCP2 = LCP1;
            LCP1 = LCP1->NEXT;
        }
        if (SY == SEMICOLON) {
            INSYMBOL();
            if (!IN(SY, SETADD(&s, d->FSYS, IDENT, -1))) {
                ERROR(6);
                SKIP(&s);
            }
        } else if (!(SY == ENDSY && INMODULE))
            ERROR(14);
    }
    if (FWPTR != NULL) {
        ERROR(117);
        FWPTR = NULL;
    }
}

static void VARDECLARATION(struct DECCTX *d)
{
    CTP LCP, NXT, IDLIST;
    STP LSP;
    ADDRRANGE LSIZE;
    SETS s, u;
    NXT = NULL;
    do {
        do {
            if (SY == IDENT) {
                LCP = INMODULE ? NEWID(V) : NEWIDN(2);   /* NEW(LCP, ACTUALVARS, INMODULE) */
                memcpy(LCP->NAME, ID, 8);
                LCP->NEXT = NXT;
                LCP->KLASS = ACTUALVARS;
                LCP->IDTYPE = NULL;
                LCP->U.V.VLEV = LEVEL;
                if (INMODULE) {
                    if (ININTERFACE)
                        LCP->U.V.PUBLIC = TRUE;
                    else
                        LCP->U.V.PUBLIC = FALSE;
                }
                ENTERID(LCP);
                NXT = LCP;
                INSYMBOL();
            } else
                ERROR(2);
            if (!IN(SY, SETPLUS(&u, SETADD(&s, d->FSYS, COMMA, COLON, -1), &TYPEDELS))) {
                ERROR(6);
                SKIP(SETPLUS(&u, SETADD(&s, d->FSYS, COMMA, COLON, SEMICOLON, -1), &TYPEDELS));
            }
            TEST = SY != COMMA;
            if (!TEST)
                INSYMBOL();
        } while (!TEST);
        if (SY == COLON)
            INSYMBOL();
        else
            ERROR(5);
        IDLIST = NXT;
        TYP(d, SETPLUS(&u, SETADD(&s, d->FSYS, SEMICOLON, -1), &TYPEDELS), &LSP, &LSIZE);
        while (NXT != NULL) {
            CTP W = NXT;                        /* WITH NXT^ DO */
            W->IDTYPE = LSP;
            W->U.V.VADDR = LC;
            LC = LC + LSIZE;
            NXT = W->NEXT;
            if (W->NEXT == NULL)
                if (LSP != NULL)
                    if (LSP->FORM == FILES) {   /* put IDLIST into the local file list */
                        W->NEXT = DISPLAY[TOP].FFILE;
                        DISPLAY[TOP].FFILE = IDLIST;
                    }
        }
        if (SY == SEMICOLON) {
            INSYMBOL();
            if (!IN(SY, SETADD(&s, d->FSYS, IDENT, -1))) {
                ERROR(6);
                SKIP(&s);
            }
        } else if (!(SY == ENDSY && INMODULE))
            ERROR(14);
    } while (!(SY != IDENT && !IN(SY, &TYPEDELS)));
    if (FWPTR != NULL) {
        ERROR(117);
        FWPTR = NULL;
    }
}

/* ======================== DECPART.C ======================== */

struct PDCTX {                          /* PROCDECLARATION's (what PARAMETERLIST uses) */
    struct DECCTX *d;
    BOOLEAN EXTONLY, FORW;
};

static void PARAMETERLIST(struct PDCTX *p, SETOFSYS *FSY, CTP *FPAR, CTP FCP)
{
    SETOFSYS *FSYS = p->d->FSYS;
    CTP LCP, LCP1, LCP2, LCP3;
    STP LSP;
    IDKIND LKIND;
    ADDRRANGE LLC, LEN;
    INTEGER COUNT;
    SETS s, u;
    LCP1 = NULL;
    LLC = LC;
    if (!IN(SY, SETADD(&s, FSY, LPARENT, -1))) {
        ERROR(7);
        SKIP(SETADD(&u, SETPLUS(&s, FSYS, FSY), LPARENT, -1));
    }
    if (SY == LPARENT) {
        if (p->FORW)
            ERROR(119);
        INSYMBOL();
        if (!(SY == IDENT || SY == VARSY)) {
            ERROR(7);
            SKIP(SETADD(&s, FSYS, IDENT, RPARENT, -1));
        }
        while (SY == IDENT || SY == VARSY) {
            if (SY == VARSY) {
                LKIND = FORMAL;
                INSYMBOL();
            } else
                LKIND = ACTUAL;
            LCP2 = NULL;
            COUNT = 0;
            do {
                if (SY != IDENT)
                    ERROR(2);
                else {
                    LCP = NEWIDN(2);    /* NEW(LCP, FORMALVARS, FALSE): may be ACTUAL (the same size) */
                    memcpy(LCP->NAME, ID, 8);
                    LCP->IDTYPE = NULL;
                    LCP->NEXT = LCP2;
                    if (LKIND == FORMAL)
                        LCP->KLASS = FORMALVARS;
                    else
                        LCP->KLASS = ACTUALVARS;
                    LCP->U.V.VLEV = LEVEL;
                    ENTERID(LCP);
                    LCP2 = LCP;
                    COUNT = COUNT + 1;
                    INSYMBOL();
                }
                if (!IN(SY, SETADD(&s, FSYS, COMMA, SEMICOLON, COLON, -1))) {
                    ERROR(7);
                    SKIP(SETADD(&s, FSYS, COMMA, SEMICOLON, RPARENT, COLON, -1));
                }
                TEST = SY != COMMA;
                if (!TEST)
                    INSYMBOL();
            } while (!TEST);
            LSP = NULL;
            if (SY == COLON) {
                INSYMBOL();
                if (SY == IDENT) {
                    SEARCHID(SETOF(&s, TYPES, -1), &LCP);
                    INSYMBOL();
                    LSP = LCP->IDTYPE;
                    LEN = PTRSIZE;
                    if (LSP != NULL)
                        if (LKIND == ACTUAL) {
                            if (LSP->FORM == FILES)
                                ERROR(121);
                            else if (LSP->FORM <= POWER)
                                LEN = LSP->SIZE;
                        }
                    LC = LC + COUNT * LEN;
                } else
                    ERROR(2);
            } else if (LKIND == FORMAL)
                p->EXTONLY = TRUE;
            else
                ERROR(5);
            if (!IN(SY, SETADD(&s, FSYS, SEMICOLON, RPARENT, -1))) {
                ERROR(7);
                SKIP(&s);
            }
            LCP3 = LCP2;
            LCP = NULL;
            while (LCP2 != NULL) {
                LCP = LCP2;
                LCP2->IDTYPE = LSP;             /* WITH LCP2^ DO */
                LCP2 = LCP2->NEXT;
            }
            if (LCP != NULL) {
                LCP->NEXT = LCP1;
                LCP1 = LCP3;
            }
            if (SY == SEMICOLON) {
                INSYMBOL();
                if (!IN(SY, SETADD(&s, FSYS, IDENT, VARSY, -1))) {
                    ERROR(7);
                    SKIP(SETADD(&s, FSYS, IDENT, RPARENT, -1));
                }
            }
        }
        if (SY == RPARENT) {
            INSYMBOL();
            if (!IN(SY, SETPLUS(&s, FSY, FSYS))) {
                ERROR(6);
                SKIP(&s);
            }
        } else
            ERROR(4);
        FCP->U.P.LOCALLC = LC;
        LCP3 = NULL;
        while (LCP1 != NULL) {
            CTP W = LCP1;                       /* WITH LCP1^ DO */
            LCP2 = W->NEXT;
            W->NEXT = LCP3;
            if (W->IDTYPE != NULL) {
                if (W->KLASS == FORMALVARS) {
                    W->U.V.VADDR = LLC;
                    LLC = LLC + PTRSIZE;
                } else if (W->KLASS == ACTUALVARS) {
                    if (W->IDTYPE->FORM <= POWER) {
                        W->U.V.VADDR = LLC;
                        LLC = LLC + W->IDTYPE->SIZE;
                    } else {
                        W->U.V.VADDR = LC;
                        LC = LC + W->IDTYPE->SIZE;
                        LLC = LLC + PTRSIZE;
                    }
                }
            }
            LCP3 = LCP1;
            LCP1 = LCP2;
        }
        *FPAR = LCP3;
    } else
        *FPAR = NULL;
}

static void PROCDECLARATION(struct DECCTX *d, SYMBOL FSY, BOOLEAN SEGDEC)
{
    struct PDCTX pc, *p = &pc;
    SETOFSYS *FSYS = d->FSYS;
    SYMBOL LSY;
    CTP LCP, LCP1;
    STP LSP;
    ADDRRANGE LCM;
    struct LEXSTKREC LLEXSTK;
    SETS s;
    p->d = d;
    p->FORW = FALSE;
    memset(&LLEXSTK, 0, sizeof LLEXSTK);
    if (SEGDEC) {                               /* a segment declaration */
        if (CODEINSEG) {
            ERROR(399);
            SEGINX = 0;
            CURBYTE = 0;
        }
        LLEXSTK.DOLDSEG = SEG;
        SEG = NEXTSEG;
        LLEXSTK.SOLDPROC = NEXTPROC;
        NEXTPROC = 1;
        LSY = SY;
        if (SY == PROCSY || SY == FUNCSY)
            INSYMBOL();
        else {
            ERROR(399);
            LSY = PROCSY;
        }
        FSY = LSY;
    }
    LLEXSTK.DLLC = LC;
    LC = LCAFTERMARKSTACK;
    if (FSY == FUNCSY)
        LC = LC + REALSIZE;
    LINEINFO = LC;
    DP = TRUE;
    p->EXTONLY = FALSE;
    if (SY == IDENT) {
        if (USING || (INMODULE && ININTERFACE))
            p->FORW = FALSE;
        else {
            SEARCHSECTION(DISPLAY[TOP].FNAME, &LCP);
            if (LCP != NULL) {
                if (LCP->KLASS == PROC)
                    p->FORW = LCP->U.P.FORWDECL && FSY == PROCSY && LCP->U.P.PFKIND == ACTUAL;
                else if (LCP->KLASS == FUNC)
                    p->FORW = LCP->U.P.FORWDECL && FSY == FUNCSY && LCP->U.P.PFKIND == ACTUAL;
                else
                    p->FORW = FALSE;
                if (!p->FORW)
                    ERROR(160);
            } else
                p->FORW = FALSE;
        }
        if (!p->FORW) {
            LCP = INMODULE ? NEWID(P) : NEWIDN(11);  /* NEW(LCP, PROC/FUNC, DECLARED, ACTUAL, INMODULE) */
            memcpy(LCP->NAME, ID, 8);
            LCP->IDTYPE = NULL;
            LCP->U.P.LOCALLC = LC;
            LCP->U.P.PFDECKIND = DECLARED;
            LCP->U.P.PFKIND = ACTUAL;
            LCP->U.P.INSCOPE = FALSE;
            LCP->U.P.PFLEV = LEVEL;
            LCP->U.P.PFNAME = NEXTPROC;
            LCP->U.P.PFSEG = SEG;
            if (USING)
                PROCTABLE[NEXTPROC] = 0;
            if (INMODULE) {
                if (USING)
                    LCP->U.P.IMPORTED = TRUE;
                else
                    LCP->U.P.IMPORTED = FALSE;
            }
            if (SEGDEC) {
                if (NEXTSEG > MAXSEG)
                    ERROR(250);
                NEXTSEG = NEXTSEG + 1;
                memcpy(SEGTABLE[SEG].SEGNAME, ID, 8);
            }
            if (NEXTPROC == MAXPROCNUM)
                ERROR(251);
            else
                NEXTPROC = NEXTPROC + 1;
            if (FSY == PROCSY)
                LCP->KLASS = PROC;
            else
                LCP->KLASS = FUNC;
            ENTERID(LCP);
        } else {
            LCP1 = LCP->NEXT;
            while (LCP1 != NULL) {
                if (LCP1->IDTYPE == NULL)
                    p->EXTONLY = TRUE;
                else if (LCP1->KLASS == FORMALVARS) {
                    LCM = LCP1->U.V.VADDR + PTRSIZE;
                    if (LCM > LC)
                        LC = LCM;
                } else if (LCP1->KLASS == ACTUALVARS) {
                    LCM = LCP1->U.V.VADDR + LCP1->IDTYPE->SIZE;
                    if (LCM > LC)
                        LC = LCM;
                }
                LCP1 = LCP1->NEXT;
            }
            if (SEG != LCP->U.P.PFSEG) {
                SEG = LCP->U.P.PFSEG;
                NEXTPROC = 2;
                if (!SEGDEC)
                    ERROR(399);
            }
        }
        INSYMBOL();
    } else {
        ERROR(2);
        LCP = UPRCPTR;
    }
    LLEXSTK.DOLDLEV = LEVEL;
    LLEXSTK.DOLDTOP = TOP;
    LLEXSTK.POLDPROC = CURPROC;
    LLEXSTK.DFPROCP = LCP;
    CURPROC = LCP->U.P.PFNAME;
    if (LEVEL < MAXLEVEL)
        LEVEL = LEVEL + 1;
    else
        ERROR(251);
    if (TOP < DISPLIMIT) {
        TOP = TOP + 1;
        if (p->FORW)
            DISPLAY[TOP].FNAME = LCP->NEXT;
        else
            DISPLAY[TOP].FNAME = NULL;
        DISPLAY[TOP].FLABEL = NULL;
        DISPLAY[TOP].FFILE = NULL;
        DISPLAY[TOP].OCCUR = BLCK;
    } else
        ERROR(250);
    if (FSY == PROCSY) {
        PARAMETERLIST(p, SETOF(&s, SEMICOLON, -1), &LCP1, LCP);
        if (!p->FORW)
            LCP->NEXT = LCP1;
    } else {
        PARAMETERLIST(p, SETOF(&s, SEMICOLON, COLON, -1), &LCP1, LCP);
        if (!p->FORW)
            LCP->NEXT = LCP1;
        if (SY == COLON) {
            INSYMBOL();
            if (SY == IDENT) {
                if (p->FORW)
                    ERROR(122);
                SEARCHID(SETOF(&s, TYPES, -1), &LCP1);
                LSP = LCP1->IDTYPE;
                LCP->IDTYPE = LSP;
                if (LSP != NULL)
                    if (!(LSP->FORM == SCALAR || LSP->FORM == SUBRANGE || LSP->FORM == POINTER)) {
                        ERROR(120);
                        LCP->IDTYPE = NULL;
                    }
                INSYMBOL();
            } else {
                ERROR(2);
                SKIP(SETADD(&s, FSYS, SEMICOLON, -1));
            }
        } else if (!p->FORW)
            ERROR(123);
    }
    if (SY == SEMICOLON)
        INSYMBOL();
    else
        ERROR(14);
    LCP->U.P.EXTURNAL = FALSE;
    if (SY == EXTERNLSY || (USING && LSEPPROC)) {
        if (LEVEL != 2)
            ERROR(183);                         /* external procedures must be in the outermost block */
        if (INMODULE)
            if (ININTERFACE && !USING)
                ERROR(184);                     /* no external declaration in an interface */
        if (SEGDEC)
            ERROR(399);
        LCP->U.P.EXTURNAL = TRUE;
        LCP->U.P.FORWDECL = FALSE;
        WLN(NULL);
        WPA(NULL, LCP->NAME, 8);
        WSTR(NULL, " [");
        WINT(NULL, MEMAVAIL(), 5);
        WSTR(NULL, " words]");
        WLN(NULL);
        WCH(NULL, '<');
        WINT(NULL, SCREENDOTS, 4);
        WCH(NULL, '>');
        PROCTABLE[CURPROC] = 0;
        DLINKERINFO = TRUE;
        if (SY == EXTERNLSY) {
            INSYMBOL();
            if (SY == SEMICOLON)
                INSYMBOL();
            else
                ERROR(14);
            if (!IN(SY, FSYS)) {
                ERROR(6);
                SKIP(FSYS);
            }
        }
    } else if (USING) {
        LCP->U.P.FORWDECL = FALSE;
    } else if (SY == FORWARDSY || (INMODULE && ININTERFACE)) {
        if (p->FORW)
            ERROR(161);
        else
            LCP->U.P.FORWDECL = TRUE;
        if (SY == FORWARDSY) {
            INSYMBOL();
            if (SY == SEMICOLON)
                INSYMBOL();
            else
                ERROR(14);
        }
        if (!IN(SY, FSYS)) {
            ERROR(6);
            SKIP(FSYS);
        }
    } else {
        if (p->EXTONLY)
            ERROR(7);
        NEWBLOCK = TRUE;
        d->NOTDONE = TRUE;
        PMARK(&LLEXSTK.DMARKP);
        LCP->U.P.FORWDECL = FALSE;
        LCP->U.P.INSCOPE = TRUE;
        LCP->U.P.EXTURNAL = FALSE;
        LLEXSTK.BFSY = SEMICOLON;
        LLEXSTK.ISSEGMENT = SEGDEC;
        LLEXSTK.PREVLEXSTACKP = TOS;
        TOS = PNEW(sizeof(struct LEXSTKREC));
        *TOS = LLEXSTK;
        return;                                 /* EXIT(PROCDECLARATION) */
    }
    /* a forward or external declaration: restore the state */
    LEVEL = LLEXSTK.DOLDLEV;
    TOP = LLEXSTK.DOLDTOP;
    LC = LLEXSTK.DLLC;
    CURPROC = LLEXSTK.POLDPROC;
    if (SEGDEC) {
        NEXTPROC = LLEXSTK.SOLDPROC;
        SEG = LLEXSTK.DOLDSEG;
    }
}

void DECLARATIONPART(SETOFSYS *FSYS)
{
    struct DECCTX dc, *d = &dc;
    SETS s, u, w;
    d->FSYS = FSYS;
    d->NOTDONE = FALSE;
    if (NOSWAP && STARTINGUP) {
        STARTINGUP = FALSE;                     /* (all segments are in by this time) */
        BLOCK(FSYS);
        return;                                 /* EXIT(DECLARATIONPART) */
    }
    /* (IF NOISY THEN UNITWRITE(3, DUMMYVAR[-1600], 35): shows the stack and heap on
       a graphics screen, unit 3 -- not here) */
    do {
        d->NOTDONE = FALSE;
        if (USERINFO.STUPID)
            if (!CODEINSEG)
                if (LEVEL == 1 && NEXTSEG == 10)
                    if (!(INMODULE || USING))
                        USESDECLARATION(d, TRUE);   /* (to get turtle graphics) */
        if (SY == USESSY) {
            INSYMBOL();
            USESDECLARATION(d, FALSE);
        }
        if (SY == LABELSY) {
            if (INMODULE && ININTERFACE) {
                ERROR(186);
                SKIP(SETMINUS(&s, FSYS, SETOF(&u, LABELSY, -1)));
            } else
                INSYMBOL();
            LABELDECLARATION(d);
        }
        if (SY == CONSTSY) {
            INSYMBOL();
            CONSTDECLARATION(d);
        }
        if (SY == TYPESY) {
            INSYMBOL();
            TYPEDECLARATION(d);
        }
        if (SY == VARSY) {
            INSYMBOL();
            VARDECLARATION(d);
        }
        if (LEVEL == 1)
            GLEV = TOP;
        if (SY == PROCSY || SY == FUNCSY || SY == PROGSY) {
            if (INMODULE)
                if (ININTERFACE && !USING)
                    PUBLICPROCS = TRUE;
            do {
                d->LSY = SY;
                INSYMBOL();
                if (d->LSY == PROGSY) {
                    if (INMODULE) {
                        ERROR(185);             /* a segment declaration is not allowed in a unit */
                        PROCDECLARATION(d, PROCSY, FALSE);
                    } else
                        PROCDECLARATION(d, d->LSY, TRUE);
                } else
                    PROCDECLARATION(d, d->LSY, FALSE);
            } while (SY == PROCSY || SY == FUNCSY || SY == PROGSY);
        }
        if (SY != BEGINSY)
            if (!((USING || INMODULE) && (SY == IMPLESY || SY == ENDSY))
                && !(SY == SEPARATSY || SY == UNITSY))
                if (!(INCLUDING || d->NOTDONE) || !IN(SY, &BLOCKBEGSYS)) {
                    ERROR(18);
                    SKIP(SETMINUS(&s, FSYS, SETOF(&u, UNITSY, INTERSY, -1)));
                }
    } while (!IN(SY, SETADD(&w, &STATBEGSYS, SEPARATSY, UNITSY, IMPLESY, ENDSY, -1)));
    NEWBLOCK = FALSE;
}
