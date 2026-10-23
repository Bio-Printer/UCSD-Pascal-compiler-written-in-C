/* unitpart.c -- UNITPART.TEXT: the segment procedures WRITELINKERINFO (the
   linker's records after a segment: its global names, the references to
   them, the non-resident procedures it calls) and UNITPART (a UNIT, or a
   SEPARATE UNIT: its interface text, then its implementation). */
#include "compglbl.h"
#include "pcio.h"

#pragma segment WRITELIN

/* LITYPES */
enum { EOFMARK, MODDULE, GLOBREF, PUBBLIC, PRIVVATE, CONNSTANT, GLOBDEF,
       PUBLICDEF, CONSTDEF, EXTPROC, EXTFUNC, SSEPPROC, SSEPFUNC,
       SEPPREF, SEPFREF };
/* OPFORMAT */
enum { LWORD, LBYTE, LBIG };

/* LIENTRY: the variant part is three words, as in the Pascal */
static struct {
    ALPHA LINAME;
    INTEGER LITYPE;
    INTEGER W[3];
} LIREC;
#define FORMAT W[0]                     /* MODDULE, PUBBLIC, PRIVVATE, SEPPREF, SEPFREF */
#define LINREFS W[1]
#define NWORDS W[2]
#define LICONST W[0]                    /* CONSTDEF */
#define BASEOFFSET W[0]                 /* PUBLICDEF */
#define PROCNUM W[0]                    /* EXTPROC, EXTFUNC, SSEPPROC, SSEPFUNC */
#define NPARAMS W[1]
#define RANGE W[2]                      /* ^INTEGER: ORD(NIL) is 1 */
#define ORDNIL 1

static INTEGER CURRENTBLOCK;
static ADDRRANGE FIC;

static void GETNEXTBLOCK(void)
{
    CURRENTBLOCK = CURRENTBLOCK + 1;
    if (CURRENTBLOCK > REFBLK)
        CURRENTBLOCK = 0;
    PBLOCKREAD(REFFILE, &(*REFLIST)[1], 1, CURRENTBLOCK);
}

static void GETREFS(INTEGER FID, INTEGER LENGTH)
{
    ADDRRANGE LIC;
    INTEGER J, MAX, BLOCKCOUNT, COUNT;
    if (NREFS == 1 && REFBLK == 0)
        return;                                 /* EXIT(GETREFS) */
    COUNT = 0;
    for (BLOCKCOUNT = 0; BLOCKCOUNT <= REFBLK; BLOCKCOUNT++) {
        if (CURRENTBLOCK < REFBLK)
            MAX = REFSPERBLK;
        else
            MAX = NREFS - 1;
        for (J = 1; J <= MAX; J++)
            if (FID == (*REFLIST)[J].KEY) {
                GENWORD((*REFLIST)[J].OFFSET);
                COUNT = COUNT + 1;
            }
        if (BLOCKCOUNT < REFBLK)
            GETNEXTBLOCK();
    }
    LIC = IC;
    IC = FIC;
    GENWORD(COUNT);
    IC = LIC;
    /* now fill the rest of the 8-word record */
    for (J = 1; J <= MOD(8 - MOD(COUNT, 8), 8); J++)
        GENWORD(0);
}

static void GENNAME(void)
{
    for (LGTH = 1; LGTH <= 8; LGTH++)
        GENBYTE(LIREC.LINAME[LGTH - 1]);
}

static void GLOBALSEARCH(CTP FCP)
{
    BOOLEAN NEEDEDBYLINKER;
    CTP LCP;
    NEEDEDBYLINKER = TRUE;
    switch (FCP->KLASS) {
    case TYPES:
        NEEDEDBYLINKER = FALSE;
        break;
    case KONST:
        if (FCP->IDTYPE->SIZE == 1 && !INMODULE) {
            LIREC.LITYPE = CONSTDEF;
            LIREC.LICONST = FCP->U.VALUES.IVAL;
        } else
            NEEDEDBYLINKER = FALSE;
        break;
    case FORMALVARS:
    case ACTUALVARS:
        if (INMODULE) {
            if (FCP->U.V.PUBLIC) {
                LIREC.LITYPE = PUBBLIC;
                LIREC.NWORDS = 0;
            } else {
                LIREC.LITYPE = PRIVVATE;
                if (FCP->KLASS == FORMALVARS)
                    LIREC.NWORDS = PTRSIZE;
                else
                    LIREC.NWORDS = FCP->IDTYPE->SIZE;
            }
            LIREC.FORMAT = LBIG;
        } else {
            LIREC.LITYPE = PUBLICDEF;
            LIREC.BASEOFFSET = FCP->U.V.VADDR;
        }
        break;
    case FIELD:
        NEEDEDBYLINKER = FALSE;
        break;
    case PROC:
    case FUNC:
        if (FCP->U.P.PFDECKIND == DECLARED) {
            if (FCP->U.P.PFKIND == ACTUAL) {
                if (FCP->KLASS == PROC) {
                    if (FCP->U.P.EXTURNAL) {
                        if (SEPPROC)
                            LIREC.LITYPE = SEPPREF;
                        else
                            LIREC.LITYPE = EXTPROC;
                    } else if (SEPPROC)
                        LIREC.LITYPE = SSEPPROC;
                    else
                        NEEDEDBYLINKER = FALSE;
                } else {                        /* KLASS = FUNC */
                    if (FCP->U.P.EXTURNAL) {
                        if (SEPPROC)
                            LIREC.LITYPE = SEPFREF;
                        else
                            LIREC.LITYPE = EXTFUNC;
                    } else if (SEPPROC)
                        LIREC.LITYPE = SSEPFUNC;
                    else
                        NEEDEDBYLINKER = FALSE;
                }
            } else
                NEEDEDBYLINKER = FALSE;
        } else
            NEEDEDBYLINKER = FALSE;
        if (NEEDEDBYLINKER) {
            LCP = FCP->NEXT;
            LIREC.NPARAMS = 0;
            while (LCP != NULL) {
                if (LCP->KLASS == FORMALVARS)
                    LIREC.NPARAMS = LIREC.NPARAMS + PTRSIZE;
                else if (LCP->KLASS == ACTUALVARS) {
                    if (LCP->IDTYPE->FORM <= POWER)
                        LIREC.NPARAMS = LIREC.NPARAMS + LCP->IDTYPE->SIZE;
                    else
                        LIREC.NPARAMS = LIREC.NPARAMS + PTRSIZE;
                }
                LCP = LCP->NEXT;
            }
            if (LIREC.LITYPE == SEPPREF || LIREC.LITYPE == SEPFREF) {
                LIREC.FORMAT = LBYTE;
                LIREC.NWORDS = LIREC.NPARAMS;
            } else {
                LIREC.PROCNUM = FCP->U.P.PFNAME;
                LIREC.RANGE = ORDNIL;
            }
        }
        break;
    case MODULE:
        if (!INMODULE)
            NEEDEDBYLINKER = FALSE;
        else {
            LIREC.LITYPE = MODDULE;
            LIREC.NWORDS = 0;
            LIREC.FORMAT = LBYTE;
        }
        break;
    }
    if (NEEDEDBYLINKER)
        if (SEGTABLE[SEG].SEGKIND == 2)         /* SEGPROC */
            if (LIREC.LITYPE == CONSTDEF || LIREC.LITYPE == PUBLICDEF)
                NEEDEDBYLINKER = FALSE;
    if (NEEDEDBYLINKER) {
        MOVELEFT(FCP->NAME, LIREC.LINAME, 8);
        GENNAME();
        GENWORD(LIREC.LITYPE);
        switch (LIREC.LITYPE) {
        case MODDULE:
        case PUBBLIC:
        case PRIVVATE:
        case SEPPREF:
        case SEPFREF:
            GENWORD(LIREC.FORMAT);
            FIC = IC;
            GENWORD(0);
            GENWORD(LIREC.NWORDS);
            if (LIREC.LITYPE == MODDULE)
                GETREFS(FCP->U.SEGID, 1);
            else if (LIREC.LITYPE == SEPPREF || LIREC.LITYPE == SEPFREF)
                GETREFS(-FCP->U.P.PFNAME, 1);
            else
                GETREFS(FCP->U.V.VADDR + 32, FCP->IDTYPE->SIZE);
            break;
        case CONSTDEF:
            GENWORD(LIREC.LICONST);
            GENWORD(0);
            GENWORD(0);
            break;
        case PUBLICDEF:
            GENWORD(LIREC.BASEOFFSET);
            GENWORD(0);
            GENWORD(0);
            break;
        case EXTPROC:
        case EXTFUNC:
            GENWORD(LIREC.PROCNUM);
            GENWORD(LIREC.NPARAMS);
            GENWORD(LIREC.RANGE);
            break;
        case SSEPPROC:
        case SSEPFUNC:
            GENWORD(LIREC.PROCNUM);
            GENWORD(LIREC.NPARAMS);
            GENWORD(LIREC.RANGE);
            GENNAME();
            if (LIREC.LITYPE == SSEPPROC)
                GENWORD(SEPPREF);
            else
                GENWORD(SEPFREF);
            GENWORD(LBYTE);
            FIC = IC;
            GENWORD(0);
            GENWORD(LIREC.NPARAMS);
            GETREFS(-LIREC.PROCNUM, 1);
            break;
        }
    }
    if (IC >= 1024) {
        WRITECODE(FALSE);
        IC = 0;
    }
    if (FCP->LLINK != NULL)
        GLOBALSEARCH(FCP->LLINK);
    if (FCP->RLINK != NULL)
        GLOBALSEARCH(FCP->RLINK);
}

void WRITELINKERINFO(BOOLEAN DECSTUFF)
{
    CTP FCP;
    NONRESIDENT I;
    IC = 0;
    if (CODEINSEG)
        ERROR(399);
    if (INMODULE)
        CURRENTBLOCK = REFBLK;
    if (DECSTUFF) {                             /* (skip if no DECLARATIONPART linker info) */
        FCP = DISPLAY[GLEV].FNAME;
        if (FCP != NULL)
            GLOBALSEARCH(FCP);
    }
    /* now the non-resident procedures */
    for (I = SEEK; I <= DECOPS; I++)
        if (PFNUMOF[I] != 0) {
            switch (I) {
            case SEEK:
                MOVELEFT("FSEEK   ", LIREC.LINAME, 8);
                LIREC.NPARAMS = 2;
                break;
            case FREADREAL:
                MOVELEFT("FREADREA", LIREC.LINAME, 8);
                LIREC.NPARAMS = 2;
                break;
            case FWRITEREAL:
                MOVELEFT("FWRITERE", LIREC.LINAME, 8);
                LIREC.NPARAMS = 5;
                break;
            case FREADDEC:
                MOVELEFT("FREADDEC", LIREC.LINAME, 8);
                LIREC.NPARAMS = 3;
                break;
            case FWRITEDEC:
                MOVELEFT("FWRITEDE", LIREC.LINAME, 8);
                LIREC.NPARAMS = 2 + DECSIZE(MAXDEC);
                break;
            case DECOPS:
                MOVELEFT("DECOPS  ", LIREC.LINAME, 8);
                LIREC.NPARAMS = 0;
                break;
            }
            GENNAME();
            if (SEPPROC) {
                GENWORD(SEPPREF);
                GENWORD(LBYTE);
                FIC = IC;
                GENWORD(0);
                GENWORD(LIREC.NPARAMS);
                GETREFS(-PFNUMOF[I], 1);
            } else {
                GENWORD(EXTPROC);
                GENWORD(PFNUMOF[I]);
                GENWORD(LIREC.NPARAMS);
                GENWORD(0);
            }
            PFNUMOF[I] = 0;
        }
    /* now the EOFMARK end record */
    for (LGTH = 1; LGTH <= 8; LGTH++)
        GENBYTE(' ');
    GENWORD(EOFMARK);
    GENWORD(LCMAX);
    GENWORD(0);
    GENWORD(0);
    WRITECODE(TRUE);
    CLINKERINFO = FALSE;
    if (DECSTUFF)
        DLINKERINFO = FALSE;
}

#pragma segment UNITPART

static void OPENREFFILE(void)
{
    REFFILE = POPENNEW((CHAR *)"\017*SYSTEM.INFO[*]", FALSE);
    if (REFFILE == NULL)
        ERROR(402);
}

static struct IDENTIFIER NOTOUTER;

static void UNITDECLARATION(SETOFSYS *FSYS, void **UMARKP)
{
    CTP LCP;
    BOOLEAN FOUND;
    struct LEXSTKREC LLEXSTK;
    if (INMODULE)
        ERROR(182);                             /* nested modules are not allowed */
    if (CODEINSEG) {
        ERROR(399);
        SEGINX = 0;
        CURBYTE = 0;
    }
    /* (BFSY, DMARKP, ISSEGMENT: not set, nor read.  Nor is DFPROCP, but BLOCK
       compares it with OUTERBLOCK -- what was on the stack, never OUTERBLOCK,
       even when that is NIL ($U-): the unit's procedures are not its end) */
    FILLCHAR(&LLEXSTK, sizeof LLEXSTK, 0);
    LLEXSTK.DFPROCP = &NOTOUTER;
    LLEXSTK.DOLDTOP = TOP;
    LLEXSTK.DOLDLEV = LEVEL;
    LLEXSTK.POLDPROC = CURPROC;
    LLEXSTK.SOLDPROC = NEXTPROC;
    LLEXSTK.DOLDSEG = SEG;
    LLEXSTK.DLLC = LC;
    LLEXSTK.PREVLEXSTACKP = TOS;
    SEG = NEXTSEG;
    NEXTSEG = NEXTSEG + 1;
    if (NEXTSEG > MAXSEG)
        ERROR(250);
    NEXTPROC = 1;
    LC = LCAFTERMARKSTACK;
    PUBLICPROCS = FALSE;
    INMODULE = TRUE;
    INSYMBOL();
    LCP = NULL;
    if (SY != IDENT)
        ERROR(2);
    else {
        FOUND = FALSE;
        LCP = MODPTR;
        while (LCP != NULL && !FOUND)
            if (memcmp(LCP->NAME, ID, 8) != 0)
                LCP = LCP->NEXT;
            else {
                FOUND = TRUE;
                ERROR(101);
            }
        if (!FOUND) {
            LCP = NEWID(SEGID);      /* NEW(LCP, MODULE) */
            MOVELEFT(ID, LCP->NAME, 8);
            LCP->IDTYPE = NULL;
            LCP->NEXT = MODPTR;
            LCP->KLASS = MODULE;
            LCP->U.SEGID = SEG;
            MODPTR = LCP;
        }
    }
    MOVELEFT(ID, SEGTABLE[SEG].SEGNAME, 8);
    PMARK(UMARKP);
    REFLIST = PNEW(sizeof(REFARRAY));
    TOS = PNEW(sizeof(struct LEXSTKREC));
    *TOS = LLEXSTK;
    LEVEL = 1;
    if (TOP < DISPLIMIT) {
        TOP = TOP + 1;
        DISPLAY[TOP].FNAME = NULL;
        DISPLAY[TOP].FFILE = NULL;
        DISPLAY[TOP].FLABEL = NULL;
        DISPLAY[TOP].OCCUR = BLCK;
        if (LCP != NULL)
            ENTERID(LCP);
    } else
        ERROR(250);
    INSYMBOL();
    if (SY == SEMICOLON)
        INSYMBOL();
    else
        ERROR(14);
}

void UNITPART(SETOFSYS *FSYS)
{
    void *UMARKP;
    SETS s, u;
    OPENREFFILE();
    do {
        /* (RESET(REFFILE): its blocks are read and written by number) */
        NREFS = 1;
        REFBLK = 0;
        if (SY == SEPARATSY) {
            SEPPROC = TRUE;
            INSYMBOL();
            if (SY != UNITSY)
                ERROR(24);
        } else
            SEPPROC = FALSE;
        UNITDECLARATION(FSYS, &UMARKP);
        if (SEPPROC)
            SEGTABLE[SEG].SEGKIND = 4;
        else
            SEGTABLE[SEG].SEGKIND = 3;
        SEGTABLE[SEG].TEXTADDR = CURBLK;
        WRITETEXT();
        if (SY == INTERSY)
            INSYMBOL();
        else
            ERROR(22);
        ININTERFACE = TRUE;
        DECLARATIONPART(FSYS);
        if (PUBLICPROCS) {
            ININTERFACE = FALSE;
            if (SY != IMPLESY) {
                ERROR(23);
                SKIP(SETMINUS(&s, FSYS, &STATBEGSYS));
            } else
                INSYMBOL();
            BLOCK(SETMINUS(&s, FSYS, SETOF(&u, SEPARATSY, UNITSY, INTERSY, IMPLESY, SETEND)));
            if (REFBLK > 0)
                if (PBLOCKWRITE(REFFILE, &(*REFLIST)[1], 1, REFBLK) != 1)
                    ERROR(402);
            WRITELINKERINFO(TRUE);
        } else {
            DLINKERINFO = FALSE;
            SEGTABLE[SEG].CODELENG = 0;
            SEGTABLE[SEG].DISKADDR = CURBLK;
            SEGTABLE[SEG].SEGKIND = 0;
        }
        SEPPROC = FALSE;                        /* (FALSE whenever not INMODULE) */
        INMODULE = FALSE;
        if (SY == ENDSY)
            INSYMBOL();
        else {
            ERROR(13);
            SKIP(FSYS);
        }
        if (SY != PERIOD) {
            if (SY == SEMICOLON)
                INSYMBOL();
            else
                ERROR(14);
        }
        TOP = TOS->DOLDTOP;
        LEVEL = TOS->DOLDLEV;
        CURPROC = TOS->POLDPROC;
        NEXTPROC = TOS->SOLDPROC;
        SEG = TOS->DOLDSEG;
        LC = TOS->DLLC;
        TOS = TOS->PREVLEXSTACKP;
        PRELEASE(UMARKP);
    } while (SY == UNITSY || SY == SEPARATSY);
    PCLOSE(REFFILE, FALSE);
}
