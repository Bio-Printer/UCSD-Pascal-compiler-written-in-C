/* double.c -- DOUBLE (not the Pascal's): IEEE binary64, 4 words, done by
   the emulator's CSPs 100..137 (CSP 100 DADD ...), the code BODYPART
   (bodypart.c) makes for it: called only when a DOUBLE is there.  A REAL
   literal or constant keeps the same number as a DOUBLE (RD.D, made from its
   digits), so it is exact where it meets one; 1.5D0 is a DOUBLE.  A module
   of its own, so that CC compiles BODYPART.C in less memory; in the
   library's segment DIRS (what finds a file in a sub-volume, in memory only
   while a path is looked up): a code file has at most 10 segments. */
#include "compglbl.h"
#include "gen.h"

#pragma segment DIRS
void GENDLDC(WORD *D)            /* LDC 4: a DOUBLE, its last word first */
{
    INTEGER J;
    GENBYTE(51 + 128);
    GENBYTE(DOUBLESIZE);
    for (J = DOUBLESIZE - 1; J >= 0; J--)
        GENWORD(D[J]);
}

static void GENDHALF(void)              /* LDC 4: 0.5 */
{
    WORD D[DOUBLESIZE];
    D[0] = D[1] = D[2] = 0;
    D[3] = 0x3FE0;
    GENDLDC(D);
}

/* N words of the frame for a while: the first of them (LC := the result when
   done with them) */
ADDRRANGE DBLTEMP(INTEGER N)
{
    ADDRRANGE T;
    T = LC;
    LC = LC + N;
    if (LC > LCMAX)
        LCMAX = LC;
    return T;
}

static void DBLPOP(ADDRRANGE T, INTEGER N)      /* the N words on top into T, T+1 ...: STR (STL) */
{
    INTEGER I;
    for (I = 0; I < N; I++)
        GEN2(56, 0, T + I);
}

static void DBLPUSH(ADDRRANGE T)                /* LDA (LLA) T, LDM 4 */
{
    GEN2(50, 0, T);
    GEN1(60, DOUBLESIZE);
}

void DBLCST(void)                /* a REAL constant where a DOUBLE is: its DOUBLE */
{
    if (GATTR.KIND == CST && GATTR.TYPTR == REALPTR)
        GATTR.TYPTR = DOUBLEPTR;
}

INTEGER DBLREL(OPERATOR LOP)     /* DCMP's relation: 0 = 1 <> 2 < 3 <= 4 > 5 >= */
{
    switch (LOP) {
    case LTOP: return 2;
    case LEOP: return 3;
    case GTOP: return 4;
    case GEOP: return 5;
    case NEOP: return 1;
    }
    return 0;
}

/* GATTR, loaded, as a DOUBLE: an INTEGER or a REAL widens (ITOD, FTOD);
   FALSE if it is no number */
BOOLEAN TODBL(void)
{
    if (GATTR.TYPTR == INTPTR)
        GENCSP(108);                            /* ITOD */
    else if (GATTR.TYPTR == REALPTR)
        GENCSP(106);                            /* FTOD */
    else
        return GATTR.TYPTR == DOUBLEPTR;
    GATTR.TYPTR = DOUBLEPTR;
    return TRUE;
}

/* a binary operator's operands, the right one (GATTR) on top, one of them a
   DOUBLE: both become DOUBLEs.  The left one is under the right one: the
   right one goes to a temporary while it widens (a REAL constant is replaced
   by its DOUBLE, LCST), and comes back.  FALSE if one is no number */
BOOLEAN DBLBOTH(STP *FSP, CSP LCST)
{
    ADDRRANGE T;
    if (*FSP != DOUBLEPTR && *FSP != INTPTR && *FSP != REALPTR)
        return FALSE;
    if (!TODBL())
        return FALSE;
    if (*FSP != DOUBLEPTR) {
        T = DBLTEMP(DOUBLESIZE + REALSIZE);
        DBLPOP(T, DOUBLESIZE);
        if (LCST != NULL) {
            DBLPOP(T + DOUBLESIZE, REALSIZE);
            GENDLDC(LCST->U.RD.D);
        } else if (*FSP == INTPTR)
            GENCSP(108);                        /* ITOD */
        else
            GENCSP(106);                        /* FTOD */
        DBLPUSH(T);
        LC = T;
        *FSP = DOUBLEPTR;
    }
    return TRUE;
}

/* -C of a REAL or DOUBLE constant: a new constant.  A REAL one is still
   loaded as the Pascal does it, LDC C then NGR (NREEL); its DOUBLE is -C */
void DBLNEG(void)
{
    CSP P;
    P = NEWCONST(RD);
    P->CCLASS = GATTR.CVAL.VALP->CCLASS == REEL ? NREEL : GATTR.CVAL.VALP->CCLASS;
    P->U.RD = GATTR.CVAL.VALP->U.RD;
    P->U.RD.D[3] = P->U.RD.D[3] ^ 0x8000;
    GATTR.CVAL.VALP = P;
}

/* a standard function (CSP 24..31) of a DOUBLE: its DOUBLE CSP; ROUND
   inline: TRUNC(X + 1 - 0.5) or TRUNC(X - 0.5) as X's sign is + or - */
void DBLSTD(INTEGER N)
{
    ADDRRANGE T;
    GATTR.TYPTR = DOUBLEPTR;
    switch (N) {
    case 24:                                    /* ROUND */
        T = DBLTEMP(DOUBLESIZE);
        DBLPOP(T, DOUBLESIZE);
        DBLPUSH(T);
        GEN2(54, 0, T + 3);                     /* LOD (LDL): the sign word */
        GENLDC(0);
        GEN0(68);                               /* GEQI */
        GENCSP(108);                            /* ITOD */
        GENCSP(100);                            /* DADD */
        GENDHALF();
        GENCSP(101);                            /* DSUB */
        GENCSP(109);                            /* DTOI */
        LC = T;
        GATTR.TYPTR = INTPTR;
        break;
    case 25: GENCSP(113); break;                /* SIN: DSIN */
    case 26: GENCSP(114); break;                /* COS: DCOS */
    case 27: GENCSP(122); break;                /* LOG: DLOG10 */
    case 28: GENCSP(118); break;                /* ATAN: DATAN */
    case 29: GENCSP(121); break;                /* LN: DLN */
    case 30: GENCSP(120); break;                /* EXP: DEXP */
    case 31: GENCSP(112); break;                /* SQRT: DSQRT */
    }
}

void DBLSQR(void)                        /* SQR: X * X */
{
    ADDRRANGE T;
    T = DBLTEMP(DOUBLESIZE);
    DBLPOP(T, DOUBLESIZE);
    DBLPUSH(T);
    DBLPUSH(T);
    GENCSP(102);                                /* DMUL */
    LC = T;
}

/* WRITE of a DOUBLE: the value and then the width (none: DEFAULT) are on
   the stack.  Its text: CSP 133 (DTOA: X, 'f' and the fraction digits after
   :W:F, else 'e' and 15 -- 16 significant digits) into a STRING[40] in the
   temporary T, its length by SCAN; written by FWRITESTRING in the width, or
   as it is when that is less (FRAC: :F follows) */
void DBLWRITE1(ADDRRANGE T, BOOLEAN DEFAULT, BOOLEAN FRAC)
{
    if (!DEFAULT)
        GEN2(56, 0, T);                         /* STR: the width */
    if (FRAC)
        GENLDC('f');
    else {
        GENLDC('e');
        GENLDC(15);
    }
}

void DBLWRITE2(ADDRRANGE T, BOOLEAN DEFAULT)
{
    ADDRRANGE S;
    S = T + 1;
    GEN2(50, 0, S);                             /* LDA: S[1] */
    GENLDC(1);
    GEN0(2);                                    /* ADI */
    GENCSP(133);                                /* DTOA */
    GEN2(50, 0, S);                             /* S[0] := SCAN(41, = CHR(0), S[1]) */
    GENLDC(0);
    GENLDC(41);
    GENLDC(0);
    GENLDC(0);
    GEN2(50, 0, S);
    GENLDC(1);
    GENLDC(0);
    GENCSP(11);                                 /* SCN */
    GEN0(63);                                   /* STB */
    GEN2(50, 0, S);                             /* FWRITESTRING(F, S, W * ORD(W > LENGTH(S))) */
    if (DEFAULT)
        GENLDC(0);
    else {
        GEN2(54, 0, T);
        GEN2(54, 0, T);
        GEN2(50, 0, S);
        GENLDC(0);
        GEN0(62);                               /* LDB */
        GEN0(69);                               /* GRTI */
        GEN0(15);                               /* MPI */
    }
    GENSYS(19);                                 /* CXP SYS FWRS */
    if (IC + 100 > MAXCODE) {                   /* (as STATEMENT checks: a WRITE of DOUBLEs is long) */
        ERROR(253);
        IC = 0;
    }
}

static void DBLWINDOW(ADDRRANGE F)              /* F^: FWINDOW^[0] */
{
    GEN2(54, 0, F);
    GEN1(35, 0);                                /* IND */
    GENLDC(0);
    GEN0(62);                                   /* LDB */
}

static void DBLNOTEOF(ADDRRANGE F)              /* ... AND NOT FEOF */
{
    GEN2(54, 0, F);
    GEN1(35, 1);                                /* IND */
    GEN0(19);                                   /* NOT */
    GEN0(4);                                    /* AND */
}

/* READ of a DOUBLE, the file's and the variable's addresses on the stack:
   as FREADREAL reads a REAL, but the number's characters (digits, + - . E e)
   go into a STRING[40] and CSP 135 (ATODM: C's strtod) makes the DOUBLE --
   the character after them stays in the window, F^.  The FIB: F^ is
   FWINDOW^[0] (word 0), FEOF word 1, FSTATE word 3 (FNEEDCHAR = 1) */
void DBLREAD(void)
{
    ADDRRANGE T, F, A, N, S;
    LBP L1, L2, L3, L4, L5;
    T = DBLTEMP(3 + 21);
    F = T;
    A = T + 1;
    N = T + 2;
    S = T + 3;
    GEN2(56, 0, A);                             /* STR: the variable's address */
    GEN2(56, 0, F);                             /* STR: the file's */
    GEN2(54, 0, F);                             /* IF FSTATE = FNEEDCHAR THEN FGET(F) */
    GEN1(35, 3);                                /* IND */
    GENLDC(1);
    GEN0(67);                                   /* EQUI */
    GENLABEL(&L1);
    GENJMP(33, L1);                             /* FJP */
    GEN2(54, 0, F);
    GENSYS(7);                                  /* CXP SYS FGET */
    PUTLABEL(L1);
    GENLABEL(&L2);                              /* WHILE (F^ = ' ') AND NOT FEOF DO FGET(F) */
    PUTLABEL(L2);
    DBLWINDOW(F);
    GENLDC(' ');
    GEN0(67);                                   /* EQUI */
    DBLNOTEOF(F);
    GENLABEL(&L3);
    GENJMP(33, L3);
    GEN2(54, 0, F);
    GENSYS(7);
    GENJMP(57, L2);                             /* UJP */
    PUTLABEL(L3);
    GENLDC(0);                                  /* N := 0 */
    GEN2(56, 0, N);
    GENLABEL(&L4);                              /* WHILE (F^ IN NUMBER) AND NOT FEOF AND (N < 40) DO */
    PUTLABEL(L4);
    DBLWINDOW(F);
    GENBYTE(51 + 128);                          /* LDC: ['+', '-', '.', '0'..'9', 'E', 'e'] */
    GENBYTE(7);
    GENWORD(0x0020);                            /* (word 6: 'e' = 101) */
    GENWORD(0);
    GENWORD(0x0020);                            /* (word 4: 'E' = 69) */
    GENWORD(0x03FF);                            /* (word 3: '0'..'9' = 48..57) */
    GENWORD(0x6800);                            /* (word 2: '+' 43, '-' 45, '.' 46) */
    GENWORD(0);
    GENWORD(0);
    GENLDC(7);
    GEN0(11);                                   /* INN */
    DBLNOTEOF(F);
    GEN2(54, 0, N);
    GENLDC(40);
    GEN0(73);                                   /* LESI */
    GEN0(4);                                    /* AND */
    GENLABEL(&L5);
    GENJMP(33, L5);
    GEN2(54, 0, N);                             /* N := N + 1; S[N] := F^; FGET(F) */
    GENLDC(1);
    GEN0(2);                                    /* ADI */
    GEN2(56, 0, N);
    GEN2(50, 0, S);
    GEN2(54, 0, N);
    DBLWINDOW(F);
    GEN0(63);                                   /* STB */
    GEN2(54, 0, F);
    GENSYS(7);
    GENJMP(57, L4);
    PUTLABEL(L5);
    GEN2(50, 0, S);                             /* S[N + 1] := CHR(0) */
    GEN2(54, 0, N);
    GENLDC(1);
    GEN0(2);
    GENLDC(0);
    GEN0(63);
    GEN2(50, 0, S);                             /* ATODM(S[1], the variable) */
    GENLDC(1);
    GEN0(2);
    GEN2(54, 0, A);
    GENCSP(135);
    GEN2(56, 0, N);                             /* (the characters it used: not needed) */
    LC = T;
}

