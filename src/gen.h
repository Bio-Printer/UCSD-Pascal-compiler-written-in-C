/* gen.h -- what BODYPART's code generator (bodypart.c) and DOUBLE's (double.c)
   share: BODYPART's GEN0 ... PUTLABEL, and DOUBLE's code for BODYPART. */
#ifndef GEN_H
#define GEN_H

#pragma segment BODYPART
void GEN0(OPRANGE FOP);
void GENLDC(INTEGER IVAL);
void GEN1(OPRANGE FOP, INTEGER FP2);
void GEN2(OPRANGE FOP, INTEGER FP1, INTEGER FP2);
void GENCSP(INTEGER N);                 /* GEN1(30(*CSP*), N) */
void GENSYS(INTEGER N);                 /* GEN2(77(*CXP*), 0(*SYS*), N) */
void GENJMP(OPRANGE FOP, LBP FLBP);
void GENLABEL(LBP *FLBP);
void PUTLABEL(LBP FLBP);

#pragma segment DIRS                    /* (double.c) */
#define ISDBL2(a, b) ((a) == DOUBLEPTR || (b) == DOUBLEPTR)
#define DWTEMP (1 + 21)                 /* WRITE's temporary: the width, a STRING[40] */
void GENDLDC(WORD *D);                  /* LDC 4: a DOUBLE, its last word first */
ADDRRANGE DBLTEMP(INTEGER N);           /* N words of the frame for a while */
void DBLCST(void);                      /* a REAL constant where a DOUBLE is: its DOUBLE */
INTEGER DBLREL(OPERATOR LOP);           /* DCMP's relation */
BOOLEAN TODBL(void);                    /* GATTR, loaded, as a DOUBLE */
BOOLEAN DBLBOTH(STP *FSP, CSP LCST);    /* a binary operator's operands, both DOUBLEs */
void DBLNEG(void);                      /* -C of a REAL or DOUBLE constant */
void DBLSTD(INTEGER N);                 /* a standard function (CSP 24..31) of a DOUBLE */
void DBLSQR(void);                      /* SQR */
void DBLWRITE1(ADDRRANGE T, BOOLEAN DEFAULT, BOOLEAN FRAC);     /* WRITE ... */
void DBLWRITE2(ADDRRANGE T, BOOLEAN DEFAULT);
void DBLREAD(void);                     /* READ */

#endif
