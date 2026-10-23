/* compglbl.h -- COMPGLBLS.TEXT: the Pascal compiler's constants, types and
   global variables (UCSD Pascal II.0, U132.A revision 1.08).

   Pascal names are kept, in capitals, so that this reads beside the Pascal.
   INTEGER is 16 bits on both builds (Tiny-C's int; int16_t on the host).
   A Pascal set is a few 16-bit words (SETS: up to 64 members). */
#ifndef COMPGLBL_H
#define COMPGLBL_H

#ifdef __TINYC__
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
typedef int INTEGER;
typedef unsigned WORD;
typedef unsigned char CHAR;
typedef float REAL;
#else
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <setjmp.h>
typedef int16_t INTEGER;
typedef uint16_t WORD;
typedef unsigned char CHAR;
typedef struct { unsigned char b[4]; } REAL;   /* the P-machine's real, as in memory (preal.c) */
#endif
typedef INTEGER BOOLEAN;
#define TRUE 1
#define FALSE 0

/* ---- CONST ---- */
#define DISPLIMIT 12
#define MAXLEVEL 8
#define MAXADDR 28000
#define INTSIZE 1
#define REALSIZE 2
#define BITSPERWD 16
#define CHARSIZE 1
#define BOOLSIZE 1
#define PTRSIZE 1
#define FILESIZE 300
#define NILFILESIZE 40
#define BITSPERCHR 8
#define CHRSPERWD 2
#define STRINGSIZE 0
#define STRGLGTH 255
#define MAXINT 32767
#define MAXDEC 36
#define DEFSTRGLGTH 80
#define LCAFTERMARKSTACK 1
#define REFSPERBLK 128
#define EOL 13
#define MAXCURSOR 1023
#define MAXCODE 1299
#define MAXJTAB 24
#define MAXSEG 15
#define MAXPROCNUM 149

/* ---- TYPE ---- */
/* basic symbols, in the order of IDSEARCH's table */
enum { IDENT, COMMA, COLON, SEMICOLON, LPARENT, RPARENT, DOSY, TOSY,
       DOWNTOSY, ENDSY, UNTILSY, OFSY, THENSY, ELSESY, BECOMES, LBRACK,
       RBRACK, ARROW, PERIOD, BEGINSY, IFSY, CASESY, REPEATSY, WHILESY,
       FORSY, WITHSY, GOTOSY, LABELSY, CONSTSY, TYPESY, VARSY, PROCSY,
       FUNCSY, PROGSY, FORWARDSY, INTCONST, REALCONST, STRINGCONST,
       NOTSY, MULOP, ADDOP, RELOP, SETSY, PACKEDSY, ARRAYSY, RECORDSY,
       FILESY, OTHERSY, LONGCONST, USESSY, UNITSY, INTERSY, IMPLESY,
       EXTERNLSY, SEPARATSY };
typedef INTEGER SYMBOL;

enum { MUL, RDIV, ANDOP, IDIV, IMOD, PLUS, MINUS, OROP, LTOP, LEOP,
       GEOP, GTOP, NEOP, EQOP, INOP, NOOP };
typedef INTEGER OPERATOR;

typedef struct { unsigned w[4]; } SETS;  /* SETOFSYS (55 members), SETOFIDS */
typedef SETS SETOFSYS;
typedef SETS SETOFIDS;

enum { SEEK, FREADREAL, FWRITEREAL, FREADDEC, FWRITEDEC, DECOPS };
typedef INTEGER NONRESIDENT;
typedef INTEGER NONRESPFLIST[DECOPS + 1];

/* constants */
enum { REEL, PSET, STRG, TRIX, LONG };
typedef INTEGER CSTCLASS;
typedef struct CONSTREC *CSP;
struct CONSTREC {
    CSTCLASS CCLASS;
    union {
        struct { INTEGER LLENG, LLAST; INTEGER LONGVAL[9 + 1]; } L;  /* LONG ([1..9]) */
        INTEGER CSTVAL0[8];             /* TRIX: CSTVAL[1..8] (CSTVAL(P, I)): overlaps the following */
        REAL RVAL;                      /* REEL */
        WORD PVAL[8];                   /* PSET: SET OF 0..127, 16 members a word */
        struct { INTEGER SLGTH; CHAR SVAL[STRGLGTH + 1]; } S;  /* STRG ([1..STRGLGTH]) */
    } U;
};

#define CSTVAL(p, i) ((p)->U.CSTVAL0[(i) - 1])     /* Pascal's CSTVAL[I], I in 1..8 */

typedef union { INTEGER IVAL; CSP VALP; } VALU;

/* data structures */
typedef INTEGER BITRANGE, OPRANGE, CURSRANGE, PROCRANGE, LEVRANGE, ADDRRANGE,
                JTABRANGE, SEGRANGE, DISPRANGE;

enum { SCALAR, SUBRANGE, POINTER, LONGINT, POWER, ARRAYS,
       RECORDS, FILES, TAGFLD, VARIANT };
typedef INTEGER STRUCTFORM;
enum { STANDARD, DECLARED, SPECIAL };
typedef INTEGER DECLKIND;

typedef struct STRUCTURE *STP;
typedef struct IDENTIFIER *CTP;

struct STRUCTURE {
    ADDRRANGE SIZE;
    STRUCTFORM FORM;
    union {
        struct { DECLKIND SCALKIND; CTP FCONST; } SC;             /* SCALAR */
        struct { STP RANGETYPE; VALU MIN, MAX; } SR;             /* SUBRANGE */
        STP ELTYPE;                                              /* POINTER */
        STP ELSET;                                               /* POWER */
        struct { STP AELTYPE, INXTYPE; BOOLEAN AISPACKD;
                 BITRANGE ELSPERWD, ELWIDTH; BOOLEAN AISSTRNG;
                 INTEGER MAXLENG; } AR;                          /* ARRAYS */
        struct { CTP FSTFLD; STP RECVAR; } RE;                   /* RECORDS */
        STP FILTYPE;                                             /* FILES */
        struct { CTP TAGFIELDP; STP FSTVAR; } TF;                /* TAGFLD */
        struct { STP NXTVAR, SUBVAR; VALU VARVAL; } VA;          /* VARIANT */
    } U;
};

enum { TYPES, KONST, FORMALVARS, ACTUALVARS, FIELD, PROC, FUNC, MODULE };
typedef INTEGER IDCLASS;
enum { ACTUAL, FORMAL };
typedef INTEGER IDKIND;
typedef CHAR ALPHA[8];

struct IDENTIFIER {
    ALPHA NAME;
    CTP LLINK, RLINK;
    STP IDTYPE;
    CTP NEXT;
    IDCLASS KLASS;
    union {
        VALU VALUES;                                             /* KONST */
        struct { LEVRANGE VLEV; ADDRRANGE VADDR; BOOLEAN PUBLIC; } V;   /* FORMALVARS, ACTUALVARS */
        struct { ADDRRANGE FLDADDR; BOOLEAN FISPACKD;
                 BITRANGE FLDRBIT, FLDWIDTH; } F;                /* FIELD */
        struct { DECLKIND PFDECKIND;
                 INTEGER KEY;                                    /* SPECIAL */
                 INTEGER CSPNUM;                                 /* STANDARD */
                 LEVRANGE PFLEV; PROCRANGE PFNAME; SEGRANGE PFSEG;  /* DECLARED */
                 IDKIND PFKIND;
                 ADDRRANGE LOCALLC; BOOLEAN FORWDECL, EXTURNAL, INSCOPE, IMPORTED; } P;  /* PROC, FUNC */
        INTEGER SEGID;                                           /* MODULE */
    } U;
};

enum { BLCK, CREC, VREC, REC };
typedef INTEGER WHERE;

/* expressions */
enum { CST, VARBL, EXPR };
typedef INTEGER ATTRKIND;
enum { DRCT, INDRCT, PACKD, MULTI, BYTE };
typedef INTEGER VACCESS;

typedef struct {
    STP TYPTR;
    ATTRKIND KIND;
    VALU CVAL;                                  /* CST */
    VACCESS ACCESS;                             /* VARBL */
    LEVRANGE VLEVEL; ADDRRANGE DPLMT;           /*   DRCT */
    ADDRRANGE IDPLMT;                           /*   INDRCT */
} ATTR;

typedef struct TESTPOINTER *TESTP;
struct TESTPOINTER { STP ELT1, ELT2; TESTP LASTTESTP; };

/* labels */
typedef struct CODELABEL *LBP;
struct CODELABEL {
    BOOLEAN DEFINED;
    ADDRRANGE REFLIST;                          /* FALSE */
    ADDRRANGE OCCURIC; JTABRANGE JTABINX;       /* TRUE */
};
typedef struct USERLABEL *LABELP;
struct USERLABEL { INTEGER LABVAL; LABELP NEXTLAB; LBP CODELBP; };

typedef struct { INTEGER KEY, OFFSET; } REFARRAY[REFSPERBLK + 1];   /* [1..REFSPERBLK] */
typedef CHAR CODEARRAY[MAXCODE + 1];
typedef CHAR SYMBUFARRAY[MAXCURSOR + 1];

enum { WORKCODE, SYSLIBRARY };
typedef INTEGER UNITFILE;

struct LEXSTKREC {
    DISPRANGE DOLDTOP;
    INTEGER DOLDLEV;
    PROCRANGE POLDPROC, SOLDPROC;
    SEGRANGE DOLDSEG;
    ADDRRANGE DLLC;
    SYMBOL BFSY;
    CTP DFPROCP;
    void *DMARKP;
    BOOLEAN ISSEGMENT;
    struct LEXSTKREC *PREVLEXSTACKP;
};

/* the system's: what the operating system passes the compiler (INFOREC of COMPILER.TEXT;
   the work files are this compiler's own I/O handles, pcio.h) */
typedef struct {
    struct PFILE *WORKSYM, *WORKCODE;
    INTEGER ERRSYM, ERRBLK, ERRNUM;
    BOOLEAN SLOWTERM, STUPID;
    CHAR ALTMODE;
} INFOREC;

/* ---- VAR ---- */
extern INFOREC USERINFO;
extern CHAR *CODEP;                     /* ^CODEARRAY: the code buffer until written out */
extern CHAR *SYMBUFP;                   /* ^SYMBUFARRAY: the source buffer */
extern ATTR GATTR;
extern DISPRANGE TOP;
extern ADDRRANGE LC, IC;
extern BOOLEAN TEST;
extern STP INTPTR;
extern SEGRANGE SEG;
extern CURSRANGE SYMCURSOR;             /* the scanner's: SYMCURSOR, SY, OP, ID (IDSEARCH) */
extern SYMBOL SY;
extern OPERATOR OP;
extern ALPHA ID;
extern INTEGER LGTH;
extern VALU VAL;
extern DISPRANGE DISX;
extern ADDRRANGE LCMAX;
extern BOOLEAN PRTERR, GOTOOK, RANGECHECK, DEBUGGING,
               NOISY, CODEINSEG, IOCHECK, BPTONLINE,
               CLINKERINFO, DLINKERINFO, LIST, TINY, LSEPPROC,
               DP, INCLUDING, USING, NOSWAP, SEPPROC,
               STARTINGUP, INMODULE, ININTERFACE, FLIPBYTES,
               LIBNOTOPEN, SYSCOMP, PUBLICPROCS, GETSTMTLEV;
extern STP REALPTR, LONGINTPTR, CHARPTR, BOOLPTR, TEXTPTR, NILPTR, INTRACTVPTR, STRGPTR;
extern CTP UTYPPTR, UCSTPTR, UVARPTR, UFLDPTR, UPRCPTR, UFCTPTR,
           MODPTR, INPUTPTR, OUTPUTPTR, OUTERBLOCK, FWPTR, USINGLIST;
extern TESTP GLOBTESTP;
extern LEVRANGE LEVEL;
extern INTEGER BEGSTMTLEV, STMTLEV;
extern void *MARKP;
extern struct LEXSTKREC *TOS;
extern DISPRANGE GLEV;
extern BOOLEAN NEWBLOCK;
extern SEGRANGE NEXTSEG;
extern INTEGER SEGINX;
extern CSP SCONST;
extern ADDRRANGE STRGCSTIC;
extern INTEGER LOWTIME, LINEINFO, SCREENDOTS, STARTDOTS, SYMBLK, SMALLESTSPACE;
extern CURSRANGE LINESTART;
extern PROCRANGE CURPROC, NEXTPROC;
extern SETOFSYS CONSTBEGSYS, SIMPTYPEBEGSYS, TYPEBEGSYS,
                BLOCKBEGSYS, SELECTSYS, FACBEGSYS, STATBEGSYS, TYPEDELS;
extern SETOFIDS VARS;

struct DISPREC {
    CTP FNAME;
    WHERE OCCUR;
    CTP FFILE; LABELP FLABEL;                   /* BLCK */
    LEVRANGE CLEV; ADDRRANGE CDSPL;             /* CREC */
    ADDRRANGE VDSPL;                            /* VREC */
};
extern struct DISPREC DISPLAY[DISPLIMIT + 1];
extern NONRESPFLIST PFNUMOF;
extern INTEGER PROCTABLE[MAXPROCNUM + 1];
struct SEGREC {
    INTEGER DISKADDR, CODELENG;
    ALPHA SEGNAME;
    INTEGER SEGKIND, TEXTADDR;
};
extern struct SEGREC SEGTABLE[MAXSEG + 1];
extern CHAR *COMMENT;                           /* ^STRING: a Pascal string, [0] its length */
extern CHAR SYSTEMLIB[41];                      /* STRING[40] */
extern JTABRANGE NEXTJTAB;
extern INTEGER JTAB[MAXJTAB + 1];
extern struct PFILE *REFFILE;
extern INTEGER NREFS, REFBLK;
extern REFARRAY *REFLIST;
extern INTEGER OLDSYMBLK, PREVSYMBLK;
extern CURSRANGE OLDSYMCURSOR, OLDLINESTART, PREVSYMCURSOR, PREVLINESTART;
extern UNITFILE USEFILE;
extern struct PFILE *INCLFILE, *LIBRARY;
extern struct PFILE *LP;                        /* the listing (TEXT) */
extern INTEGER CURBYTE, CURBLK;
extern CHAR DISKBUF[512];

/* EXIT(PASCALCOMPILER): back to the system (pc.c) */
extern jmp_buf EXITCOMPILER;

/* ---- sets (sets.c) ---- */
SETS *SETOF(SETS *s, int x, ...);           /* s := [x, ...] (the list ends with -1) */
SETS *SETPLUS(SETS *s, SETS *a, SETS *b);   /* s := a + b */
SETS *SETMINUS(SETS *s, SETS *a, SETS *b);  /* s := a - b */
SETS *SETADD(SETS *s, SETS *a, int x, ...); /* s := a + [x, ...] */
#define IN(x, s) (((s)->w[(x) >> 4] >> ((x) & 15)) & 1)

/* ---- forward declared procedures (COMPGLBLS) ---- */
void ERROR(INTEGER ERRORNUM);
void GETNEXTPAGE(void);
void PRINTLINE(void);
void ENTERID(CTP FCP);
void INSYMBOL(void);
void SEARCHSECTION(CTP FCP, CTP *FCP1);
void SEARCHID(SETOFIDS *FIDCLS, CTP *FCP);
void GETBOUNDS(STP FSP, INTEGER *FMIN, INTEGER *FMAX);
void SKIP(SETOFSYS *FSYS);
BOOLEAN PAOFCHAR(STP FSP);
BOOLEAN STRGTYPE(STP FSP);
INTEGER DECSIZE(INTEGER I);
void CONSTANT(SETOFSYS *FSYS, STP *FSP, VALU *FVALU);
BOOLEAN COMPTYPES(STP FSP1, STP FSP2);
void GENBYTE(INTEGER FBYTE);
void GENWORD(INTEGER FWORD);
void WRITETEXT(void);
void WRITECODE(BOOLEAN FORCEBUF);
void BLOCK(SETOFSYS *FSYS);
void FINISHSEG(void);

/* the segment procedures */
void PASCALCOMPILER(void);                      /* block.c: the compiler */
void COMPINIT(void);                            /* compinit.c */
void DECLARATIONPART(SETOFSYS *FSYS);          /* decpart.c */
void BODYPART(SETOFSYS *FSYS, CTP FPROCP);      /* bodypart.c */
void WRITELINKERINFO(BOOLEAN DECSTUFF);         /* unitpart.c */
void UNITPART(SETOFSYS *FSYS);

/* the interpreter's built-ins (builtin.c) */
void IDSEARCH(void);                            /* the identifier at SYMBUFP^[SYMCURSOR] */
INTEGER TREESEARCH(CTP ROOT, CTP *FOUND, CHAR *TARGET);
INTEGER SCAN(INTEGER LIMIT, BOOLEAN NE, CHAR CH, CHAR *P);  /* SCAN(LIMIT, =CH or <>CH, P^) */

/* REAL arithmetic: Tiny-C's float on the P-System (the P-machine's real); its
   bit-exact emulation on the host (preal.c) */
#ifdef __TINYC__
#define RFLOAT(i) ((REAL)(i))
#define RADD(a, b) ((a) + (b))
#define RSUB(a, b) ((a) - (b))
#define RMUL(a, b) ((a) * (b))
#define RDIVR(a, b) ((a) / (b))
#define RNEG(a) (-(a))
#define RPOT(i) __cspf(36, i)               /* PWROFTEN */
#define RTRUNC(a) ((INTEGER)(a))
#define RROUND(a) __cspi(24, a)             /* ROUND (CSP 24; TRUNC is 23) */
#define RLT(a, b) ((a) < (b))
#else
REAL RFLOAT(INTEGER i);
REAL RADD(REAL a, REAL b);
REAL RSUB(REAL a, REAL b);
REAL RMUL(REAL a, REAL b);
REAL RDIVR(REAL a, REAL b);
REAL RNEG(REAL a);
REAL RPOT(INTEGER i);
INTEGER RTRUNC(REAL a);
INTEGER RROUND(REAL a);
BOOLEAN RLT(REAL a, REAL b);
#endif

/* NEW, MARK, RELEASE (heap.c) */
void *PNEW(int bytes);
void PMARK(void **p);
void PRELEASE(void *p);
INTEGER MEMAVAIL(void);

#endif
