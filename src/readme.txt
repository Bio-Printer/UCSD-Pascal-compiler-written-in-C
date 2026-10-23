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

USE

From the shell (S at the Command: prompt):

  PC NAME              compile NAME.TEXT to NAME.CODE
  PC NAME CODE         ... to CODE.CODE (CODE: any name; .CODE is added)

as the system's C(ompile does with "Compile what text?" NAME and "To what
codefile?" CODE.  Include files ($I) and the library ($U, USES) are found
as the Pascal compiler finds them: by their names, on the prefix volume
when they have no volume.  The work files are not used.  PC is not yet the
system's SYSTEM.COMPILER: C(ompile at the Command: prompt runs the Pascal
one.

THE FILES

  PC.CODE       the compiler
  BUILD.TEXT    CC @PASCALC:BUILD makes PC.CODE again, the same
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
                statements
  UNITPART.C    WRITELINKERINFO and UNITPART (segments): units
  BLOCK.C       BLOCK, and the compiler's main body
  RARE.C        what the compiler seldom does, a segment of its own: ERROR,
                the options of a comment, long and REAL constants, the end
                of a compile
  PCIO_PSYS.C   the system's files: FINIT, FOPEN, FBLOCKIO, FCLOSE, the
                listing by FWRITECHAR; the console
  PC_PSYS.C     PC NAME [CODE]: the command line
  FILES.TEXT    the list of the files

BUILDING IT

  CC @PASCALC:BUILD   (from the shell; TINY-C: on line)

compiles the modules (their .OBJ files go onto PASCALC:) and links
PASCALC:PC.CODE, the same as the one it replaces, byte for byte.

MEMORY

PC runs in the memory a program has from the shell.  Its segments are
those of the Pascal compiler: COMPINIT, DECLARATIONPART and BODYPART are in
memory only while they run, and a $S+ or a small memory makes them take
turns (swapping); NOSWAP keeps both in when 20000 words are free.  The
compiler's own source, the largest program here, compiles from the shell
with 3777 words to spare.

The C sources, the host build (the same compiler for Linux, with the
P-machine's REAL arithmetic emulated) and the tests are in the repository
UCSD-Pascal-compiler-written-in-C.
