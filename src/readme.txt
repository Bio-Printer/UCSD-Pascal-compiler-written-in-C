PASCALC:  THE PASCAL COMPILER IN C

(The disk image is PASCAL-C.BLK; a volume's name has at most 7 characters.)

PC is the UCSD Pascal II.0 compiler written in C (Tiny-C): a translation,
routine by routine, of U132.A_PASCAL_COMPILER_SOURCE_v1.08, the source of
BIGGY's SYSTEM.COMPILER.  Its code files are the same as the Pascal
compiler's: compiling the compiler's own source (COMPILER.TEXT on U132.A)
gives SYSTEM.COMPILER's code file, and so do the operating system, the
Filer, the editor L2, the linker, BASIC, the utilities of U138.3 and the
emulator's test programs.  What it shows on the console and writes in a
listing is what the Pascal compiler does.

WHAT IS WHERE (FILES.TEXT lists every file)

  PC.CODE       the compiler
  README.TEXT   this
  SRC/          its sources (C, Tiny-C), and BUILD.TEXT:
                CC @PASCALC:/SRC/BUILD makes PC.CODE again, the same
  OBJ/          empty: the objects BUILD makes (-O PASCALC:/OBJ)
  DEMOS/        programs to try: PI (pi by Archimedes' method, DOUBLEs)
  TESTS/        the tests of DOUBLE: DOUBLES, READTEST

SRC, OBJ, DEMOS and TESTS are sub-volumes, as on TINY-C: (the shell's
MKDIR, CD, DIR: DIR PASCALC:/DEMOS, CD PASCALC:/TESTS); the Filer sees
each as one data file.

USE

From the shell (S at the Command: prompt):

  PC NAME              compile NAME.TEXT to NAME.CODE
  PC NAME CODE         ... to CODE.CODE (CODE: any name; .CODE is added)

as the system's C(ompile does with "Compile what text?" NAME and "To what
codefile?" CODE.  NAME may be a path into sub-volumes, as in every Tiny-C
program: PC PASCALC:/DEMOS/PI compiles DEMOS/PI.TEXT to DEMOS/PI.CODE, and
PASCALC:/DEMOS/PI runs it where it is; after CD PASCALC:/DEMOS, PC PI and
PI do the same (a name without a volume is on the shell's path).  Include
files ($I) and the library ($U, USES) are found the same way, and when
they are not there as the Pascal compiler finds them: by their names, on
the prefix volume when they have no volume.  (A program PC compiles opens
its own files as any Pascal program does, through the system: on the
prefix volume, which a CD sets to the path's volume.)  The work files are
not used.  PC is not yet the system's SYSTEM.COMPILER: C(ompile at the
Command: prompt runs the Pascal one.

THE SOURCES (SRC)

  BUILD.TEXT    CC @PASCALC:/SRC/BUILD makes PC.CODE again, the same
  COMPGLBL.H    COMPGLBLS: the constants, types and global variables
  PCIO.H        what the compiler reads and writes (the system's files)
  GLOBALS.C     the global variables
  PROCS.C       PROCS.A, PROCS.B: the scanner, the symbol table, the code
                buffer
  BUILTIN.C     the interpreter's TREESEARCH and SCAN (IDSEARCH is the
                interpreter's own)
  SETS.C        Pascal's sets (SET OF SYMBOL, SET OF IDCLASS)
  HEAP.C        NEW, MARK, RELEASE, MEMAVAIL (the P-machine's)
  COMPINIT.C    COMPINIT (a segment): the standard names
  DECPART.C     DECPART.A-C, DECLARATIONPART (a segment)
  BODYPART.C    BODYPART.A-E (a segment): the code generator, expressions,
                statements; DOUBLE's code (in the segment DIRS)
  UNITPART.C    WRITELINKERINFO and UNITPART (segments): units
  BLOCK.C       BLOCK, and the compiler's main body
  RARE.C        what the compiler seldom does, a segment of its own: ERROR,
                the options of a comment, long and REAL constants, the end
                of a compile
  PCIO_PSYS.C   the system's files: FINIT, FOPEN, FBLOCKIO, FCLOSE, the
                listing by FWRITECHAR; files in sub-volumes; the console
  PC_PSYS.C     PC NAME [CODE]: the command line

DOUBLE

PC has a type the Pascal compiler does not: DOUBLE, 8 bytes (IEEE
binary64: 16 significant digits, exponents to 308), done by the emulator
(its CSPs 100..137).  1.5D0 is a DOUBLE constant (D for E); a REAL
constant (0.1) is made a DOUBLE from its digits where it meets one.
INTEGER and REAL become DOUBLE by themselves; SNGL(D) is D as a REAL,
TRUNC and ROUND give an INTEGER.  + - * /, the comparisons, ABS SQR SQRT
SIN COS ATAN LN EXP LOG, WRITE(D:W:F) (F decimals), WRITE(D) (16 digits,
3.141592653589793e+00) and READ(D) work as for a REAL; nothing is needed
from SYSTEM.LIBRARY.  Try:  PC PASCALC:/DEMOS/PI  then  PASCALC:/DEMOS/PI.

BUILDING IT

  CC @PASCALC:/SRC/BUILD   (from the shell; TINY-C: on line)

compiles the modules in SRC (their .OBJ files go into OBJ) and links
PASCALC:PC.CODE, the same as the one it replaces, byte for byte.
CC @TINY-C:ALL does it too, after Tiny-C, its tools and SYS-C:SYSTEM.C.

MEMORY

PC runs in the memory a program has from the shell.  Its segments are
those of the Pascal compiler: COMPINIT, DECLARATIONPART and BODYPART are in
memory only while they run, and a $S+ or a small memory makes them take
turns (swapping); NOSWAP keeps both in when 20000 words are free.  The
compiler's own source, the largest program here, compiles from the shell
with 3146 words to spare.

The C sources, the host build (the same compiler for Linux, with the
P-machine's REAL arithmetic emulated) and the tests are in the repository
UCSD-Pascal-compiler-written-in-C.
