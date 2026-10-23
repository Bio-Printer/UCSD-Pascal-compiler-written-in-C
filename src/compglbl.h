/* compglbl.h -- COMPGLBLS.TEXT: the Pascal compiler's constants, types and
   global variables (UCSD Pascal II.0, U132.A revision 1.08).

   Pascal names are kept, in capitals, so that this reads beside the Pascal.
   INTEGER is 16 bits on both builds (Tiny-C's int; int16_t on the host).
   A Pascal set is a few 16-bit words (SETS: up to 64 members). */
#ifndef COMPGLBL_H
#define COMPGLBL_H

#ifdef __TINYC__
#include <stddef.h>
#include <string.h>
typedef int INTEGER;
typedef unsigned WORD;
typedef unsigned char CHAR;
typedef float REAL;
#else
#include <stdio.h>
#include <stddef.h>
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
/* (The values of the enumerated types are macros, not enums: CC on the
   P-System keeps every enum constant of a header in the memory it compiles
   a module in, and these are 170; the preprocessor has room for them.
   UNITFILE's stay an enum: WORKCODE is also a field of INFOREC.) */
/* basic symbols, in the order of IDSEARCH's table */
#define IDENT 0
#define COMMA 1
#define COLON 2
#define SEMICOLON 3
#define LPARENT 4
#define RPARENT 5
#define DOSY 6
#define TOSY 7
#define DOWNTOSY 8
#define ENDSY 9
#define UNTILSY 10
#define OFSY 11
#define THENSY 12
#define ELSESY 13
#define BECOMES 14
#define LBRACK 15
#define RBRACK 16
#define ARROW 17
#define PERIOD 18
#define BEGINSY 19
#define IFSY 20
#define CASESY 21
#define REPEATSY 22
#define WHILESY 23
#define FORSY 24
#define WITHSY 25
#define GOTOSY 26
#define LABELSY 27
#define CONSTSY 28
#define TYPESY 29
#define VARSY 30
#define PROCSY 31
#define FUNCSY 32
#define PROGSY 33
#define FORWARDSY 34
#define INTCONST 35
#define REALCONST 36
#define STRINGCONST 37
#define NOTSY 38
#define MULOP 39
#define ADDOP 40
#define RELOP 41
#define SETSY 42
#define PACKEDSY 43
#define ARRAYSY 44
#define RECORDSY 45
#define FILESY 46
#define OTHERSY 47
#define LONGCONST 48
#define USESSY 49
#define UNITSY 50
#define INTERSY 51
#define IMPLESY 52
#define EXTERNLSY 53
#define SEPARATSY 54
typedef INTEGER SYMBOL;

#define MUL 0
#define RDIV 1
#define ANDOP 2
#define IDIV 3
#define IMOD 4
#define PLUS 5
#define MINUS 6
#define OROP 7
#define LTOP 8
#define LEOP 9
#define GEOP 10
#define GTOP 11
#define NEOP 12
#define EQOP 13
#define INOP 14
#define NOOP 15
typedef INTEGER OPERATOR;

typedef struct { unsigned w[4]; } SETS;  /* SETOFSYS (55 members), SETOFIDS */
typedef SETS SETOFSYS;
typedef SETS SETOFIDS;

#define SEEK 0
#define FREADREAL 1
#define FWRITEREAL 2
#define FREADDEC 3
#define FWRITEDEC 4
#define DECOPS 5
typedef INTEGER NONRESIDENT;
typedef INTEGER NONRESPFLIST[DECOPS + 1];

/* constants */
#define REEL 0
#define PSET 1
#define STRG 2
#define TRIX 3
#define LONG 4
typedef INTEGER CSTCLASS;
typedef struct CONSTREC *CSP;
/* (each variant's type has a name, CST_<variant>, ST_..., ID_..., for NEWCONST,
   NEWTYPE, NEWID: their sizes) */
typedef struct { INTEGER LLENG, LLAST; INTEGER LONGVAL[9 + 1]; } CST_L;   /* LONG ([1..9]) */
typedef REAL CST_RVAL;                                                  /* REEL */
struct CONSTREC {
    CSTCLASS CCLASS;
    union CSTVARIANT {
        CST_L L;
        INTEGER CSTVAL0[8];             /* TRIX: CSTVAL[1..8] (CSTVAL(P, I)): overlaps the following */
        CST_RVAL RVAL;
        WORD PVAL[8];                   /* PSET: SET OF 0..127, 16 members a word */
        struct { INTEGER SLGTH; CHAR SVAL[STRGLGTH + 1]; } S;  /* STRG ([1..STRGLGTH]) */
    } U;
};

#define CSTVAL(p, i) ((p)->U.CSTVAL0[(i) - 1])     /* Pascal's CSTVAL[I], I in 1..8 */

typedef union { INTEGER IVAL; CSP VALP; } VALU;

/* data structures */
typedef INTEGER BITRANGE, OPRANGE, CURSRANGE, PROCRANGE, LEVRANGE, ADDRRANGE,
                JTABRANGE, SEGRANGE, DISPRANGE;

#define SCALAR 0
#define SUBRANGE 1
#define POINTER 2
#define LONGINT 3
#define POWER 4
#define ARRAYS 5
#define RECORDS 6
#define FILES 7
#define TAGFLD 8
#define VARIANT 9
typedef INTEGER STRUCTFORM;
#define STANDARD 0
#define DECLARED 1
#define SPECIAL 2
typedef INTEGER DECLKIND;

typedef struct STRUCTURE *STP;
typedef struct IDENTIFIER *CTP;

typedef struct { DECLKIND SCALKIND; CTP FCONST; } ST_SC;              /* SCALAR */
typedef struct { STP RANGETYPE; VALU MIN, MAX; } ST_SR;              /* SUBRANGE */
typedef STP ST_ELTYPE;                                               /* POINTER */
typedef STP ST_ELSET;                                                /* POWER */
typedef struct { STP AELTYPE, INXTYPE; BOOLEAN AISPACKD;
                 BITRANGE ELSPERWD, ELWIDTH; BOOLEAN AISSTRNG;
                 INTEGER MAXLENG; } ST_AR;                           /* ARRAYS */
typedef struct { CTP FSTFLD; STP RECVAR; } ST_RE;                    /* RECORDS */
typedef STP ST_FILTYPE;                                              /* FILES */
typedef struct { CTP TAGFIELDP; STP FSTVAR; } ST_TF;                 /* TAGFLD */
typedef struct { STP NXTVAR, SUBVAR; VALU VARVAL; } ST_VA;           /* VARIANT */
struct STRUCTURE {
    ADDRRANGE SIZE;
    STRUCTFORM FORM;
    union STVARIANT {
        ST_SC SC;
        ST_SR SR;
        ST_ELTYPE ELTYPE;
        ST_ELSET ELSET;
        ST_AR AR;
        ST_RE RE;
        ST_FILTYPE FILTYPE;
        ST_TF TF;
        ST_VA VA;
    } U;
};

#define TYPES 0
#define KONST 1
#define FORMALVARS 2
#define ACTUALVARS 3
#define FIELD 4
#define PROC 5
#define FUNC 6
#define MODULE 7
typedef INTEGER IDCLASS;
#define ACTUAL 0
#define FORMAL 1
typedef INTEGER IDKIND;
typedef CHAR ALPHA[8];

/* the variants of IDENTIFIER: each a word for each field, in order (NEWIDN) */
typedef VALU ID_VALUES;                                              /* KONST */
typedef struct { LEVRANGE VLEV; ADDRRANGE VADDR; BOOLEAN PUBLIC; } ID_V;  /* FORMALVARS, ACTUALVARS */
typedef struct { ADDRRANGE FLDADDR; BOOLEAN FISPACKD;
                 BITRANGE FLDRBIT, FLDWIDTH; } ID_F;                 /* FIELD */
typedef struct { DECLKIND PFDECKIND;
                 INTEGER KEY;                                        /* SPECIAL */
                 INTEGER CSPNUM;                                     /* STANDARD */
                 LEVRANGE PFLEV; PROCRANGE PFNAME; SEGRANGE PFSEG;   /* DECLARED */
                 IDKIND PFKIND;
                 ADDRRANGE LOCALLC; BOOLEAN FORWDECL, EXTURNAL, INSCOPE, IMPORTED; } ID_P;  /* PROC, FUNC */
typedef INTEGER ID_SEGID;                                            /* MODULE */
struct IDENTIFIER {
    ALPHA NAME;
    CTP LLINK, RLINK;
    STP IDTYPE;
    CTP NEXT;
    IDCLASS KLASS;
    union IDVARIANT {
        ID_VALUES VALUES;
        ID_V V;
        ID_F F;
        ID_P P;
        ID_SEGID SEGID;
    } U;
};

#define BLCK 0
#define CREC 1
#define VREC 2
#define REC 3
typedef INTEGER WHERE;

/* expressions */
#define CST 0
#define VARBL 1
#define EXPR 2
typedef INTEGER ATTRKIND;
#define DRCT 0
#define INDRCT 1
#define PACKD 2
#define MULTI 3
#define BYTE 4
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
/* the scanner's: IDSEARCH (the interpreter's, CSP 7, on the P-System) writes
   SY, OP and ID after SYMCURSOR, as the Pascal compiler's globals are */
struct SCANNER {
    CURSRANGE SYMCURSOR;
    SYMBOL SY;
    OPERATOR OP;
    ALPHA ID;
};
extern struct SCANNER SCAN_;
#define SYMCURSOR SCAN_.SYMCURSOR
#define SY SCAN_.SY
#define OP SCAN_.OP
#define ID SCAN_.ID
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

/* EXIT(PASCALCOMPILER): back to the system -- the P-machine's XIT (CSP 4),
   as the Pascal does it: a function's address is its segment + 256 * its
   procedure number; on the host a longjmp to main */
void PASCALCOMPILER(void);
#ifdef __TINYC__
#define EXITCOMPILER() __cspv(4, (int)PASCALCOMPILER & 255, (int)PASCALCOMPILER >> 8)
#else
extern jmp_buf EXITPC;
#define EXITCOMPILER() longjmp(EXITPC, 1)
#endif

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

/* rare.c: what the compiler seldom does */
INTEGER OPTIONS(CHAR STOPPER);
void LONGNUMBER(INTEGER IPART, INTEGER ENDI, INTEGER K, INTEGER ISUM);
void REALNUMBER(INTEGER IPART, INTEGER ENDI, INTEGER FPART, INTEGER ENDF,
                INTEGER EPART, INTEGER ENDE, INTEGER SIGN);
void FINISHCOMPILE(void);
void SCANSTRING(CHAR *FSTRG, INTEGER MAXLENG, CHAR STOPPER);    /* procs.c */

/* the segment procedures */
void COMPINIT(void);                            /* compinit.c */
void DECLARATIONPART(SETOFSYS *FSYS);          /* decpart.c */
void BODYPART(SETOFSYS *FSYS, CTP FPROCP);      /* bodypart.c */
void WRITELINKERINFO(BOOLEAN DECSTUFF);         /* unitpart.c */
void UNITPART(SETOFSYS *FSYS);

/* the interpreter's built-ins (builtin.c) */
#ifdef __TINYC__
#define IDSEARCH() __cspv(7, &SYMCURSOR, SYMBUFP)   /* the identifier at SYMBUFP^[SYMCURSOR] */
#else
void IDSEARCH(void);
#endif
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
/* NEW(P, tag ...): the fixed part and one variant (U, last), as the Pascal
   allocates a record (sizes of types only: an expression would be too much
   for CC on the P-System); NEWID0, NEWTYPE0: a variant without fields;
   NEWIDN(N): the variant's first N fields, a word each, when the tags end
   it early (NEW(LCP, FIELD, FALSE), NEW(LCP, PROC, SPECIAL) ...) */
#define IDHEAD (sizeof(struct IDENTIFIER) - sizeof(union IDVARIANT))
#define NEWID(v) ((CTP)PNEW(IDHEAD + sizeof(ID_##v)))
#define NEWID0() ((CTP)PNEW(IDHEAD))
#define NEWIDN(n) ((CTP)PNEW(IDHEAD + (n) * sizeof(INTEGER)))
#define STHEAD (sizeof(struct STRUCTURE) - sizeof(union STVARIANT))
#define NEWTYPE(v) ((STP)PNEW(STHEAD + sizeof(ST_##v)))
#define NEWTYPE0() ((STP)PNEW(STHEAD))
#define NEWCONST(v) ((CSP)PNEW(sizeof(struct CONSTREC) - sizeof(union CSTVARIANT) + sizeof(CST_##v)))
void *PNEW(int bytes);
void PMARK(void **p);
void PRELEASE(void *p);
INTEGER MEMAVAIL(void);

#endif
