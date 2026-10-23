/* builtin.c -- the interpreter's built-in procedures the Pascal compiler calls:
   IDSEARCH (CSP 7: the identifier at the cursor, and the reserved words;
   on the P-System the interpreter's own),
   TREESEARCH (CSP 8: a name in an identifier tree) and SCAN (CSP 11).
   They do what SYSTEM.MICRO does (the emulator's NativeCsp.inc follows it
   instruction for instruction; its reserved-word table is the interpreter's
   RESTBL at 1766H). */
#include "compglbl.h"

#ifndef __TINYC__                       /* (the interpreter's on the P-System, CSP 7) */
/* RESTBL: for each letter A..Z+1 the index of its first record; a record is
   the word's 8 characters, its SYMBOL and its OPERATOR */
static const unsigned char RESIDX[27] = {
    0, 2, 3, 5, 8, 11, 15, 16, 16, 20, 20, 20, 21, 22, 23, 25, 28, 28, 30, 33, 36, 39, 40, 42, 42, 42, 42 };
/* the records in three tables: the words (8 characters each), their SYMBOLs and OPERATORs */
static const char RESNAME[42][9] = {
    "AND     ", "ARRAY   ", "BEGIN   ", "CASE    ", "CONST   ", "DIV     ",
    "DO      ", "DOWNTO  ", "ELSE    ", "END     ", "EXTERNAL", "FOR     ",
    "FILE    ", "FORWARD ", "FUNCTION", "GOTO    ", "IF      ", "IMPLEMEN",
    "IN      ", "INTERFAC", "LABEL   ", "MOD     ", "NOT     ", "OF      ",
    "OR      ", "PACKED  ", "PROCEDUR", "PROGRAM ", "RECORD  ", "REPEAT  ",
    "SET     ", "SEGMENT ", "SEPARATE", "THEN    ", "TO      ", "TYPE    ",
    "UNIT    ", "UNTIL   ", "USES    ", "VAR     ", "WHILE   ", "WITH    " };
static const unsigned char RESSY[42] = {
    MULOP, ARRAYSY, BEGINSY, CASESY, CONSTSY, MULOP, DOSY, DOWNTOSY,
    ELSESY, ENDSY, EXTERNLSY, FORSY, FILESY, FORWARDSY, FUNCSY, GOTOSY,
    IFSY, IMPLESY, RELOP, INTERSY, LABELSY, MULOP, NOTSY, OFSY,
    ADDOP, PACKEDSY, PROCSY, PROGSY, RECORDSY, REPEATSY, SETSY, PROGSY,
    SEPARATSY, THENSY, TOSY, TYPESY, UNITSY, UNTILSY, USESSY, VARSY,
    WHILESY, WITHSY };
static const unsigned char RESOP[42] = {
    ANDOP, NOOP, NOOP, NOOP, NOOP, IDIV, NOOP, NOOP,
    NOOP, NOOP, NOOP, NOOP, NOOP, NOOP, NOOP, NOOP,
    NOOP, NOOP, INOP, NOOP, NOOP, IMOD, MUL, NOOP,
    OROP, NOOP, NOOP, NOOP, NOOP, NOOP, NOOP, NOOP,
    NOOP, NOOP, NOOP, NOOP, NOOP, NOOP, NOOP, NOOP,
    NOOP, NOOP };

static CHAR upper(CHAR c)
{
    c = c & 127;
    if (c >= 96)
        c = c - 32;
    return c;
}

/* IDSEARCH(SYMCURSOR, SYMBUFP^): the identifier starting at the cursor, in capitals,
   '_' left out, its first 8 characters kept; the cursor on its last character.
   A reserved word sets SY and OP (ID is left as it was); another identifier
   SY := IDENT and ID := its 8 characters. */
void IDSEARCH(void)
{
    CHAR tok[8];
    int i, n, k, rec, end;
    CHAR a;
    for (i = 1; i < 8; i++)
        tok[i] = ' ';
    i = SYMCURSOR;
    tok[0] = upper(SYMBUFP[i]);
    i++;
    n = 0;
    for (;;) {
        a = SYMBUFP[i] & 127;
        if (a == '_') {
            i++;
            continue;
        }
        a = upper(a);
        if (!((a >= 'A' && a <= 'Z') || (a >= '0' && a <= '9')))
            break;
        n++;
        if (n <= 7)
            tok[n] = a;
        i++;
    }
    SYMCURSOR = i - 1;
    k = tok[0] - 'A';
    rec = RESIDX[k];
    end = RESIDX[k + 1];
    for (; rec < end; rec++) {
        for (i = 1; i < 8; i++)
            if ((CHAR)RESNAME[rec][i] != tok[i])
                break;
        if (i == 8) {
            SY = RESSY[rec];
            OP = RESOP[rec];
            return;
        }
    }
    SY = IDENT;                         /* (OP as it was: INSYMBOL set it to NOOP) */
    for (i = 0; i < 8; i++)
        ID[i] = tok[i];
}
#endif

/* TREESEARCH(ROOT, FOUND, TARGET): 0 when a node's NAME is TARGET (FOUND that node);
   else 1 or -1, FOUND the node whose right or left link (NIL) the name would hang on.
   The names are compared byte by byte; a node's name below the target goes right. */
INTEGER TREESEARCH(CTP ROOT, CTP *FOUND, CHAR *TARGET)
{
    CTP node, link;
    int i;
    node = ROOT;
    for (;;) {
        for (i = 0; i < 8; i++)
            if (node->NAME[i] != TARGET[i])
                break;
        if (i == 8) {
            *FOUND = node;
            return 0;
        }
        if (node->NAME[i] < TARGET[i]) {
            link = node->RLINK;
            if (!link) {
                *FOUND = node;
                return 1;
            }
        } else {
            link = node->LLINK;
            if (!link) {
                *FOUND = node;
                return -1;
            }
        }
        node = link;
    }
}

/* SCAN(LIMIT, =CH or <>CH, P^): how far from P the first character that is (or is not:
   NE) CH lies, looking LIMIT characters forward (or -LIMIT back); LIMIT if none */
INTEGER SCAN(INTEGER LIMIT, BOOLEAN NE, CHAR CH, CHAR *P)
{
    INTEGER i;
    if (LIMIT >= 0) {
        for (i = 0; i < LIMIT; i++)
            if ((P[i] == CH) != NE)
                return i;
        return LIMIT;
    }
    for (i = 0; i > LIMIT; i--)
        if ((P[i] == CH) != NE)
            return i;
    return LIMIT;
}
