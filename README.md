# UCSD-Pascal-compiler-written-in-C

The UCSD Pascal II.0 compiler (`SYSTEM.COMPILER`, U132.A source revision 1.08)
rewritten in C for [Tiny-C](https://github.com/Bio-Printer/UCSD-TinyC), to replace
the Pascal one on the P-System.

**Status: phase 1 done** -- a routine-by-routine translation of the Pascal
compiler that produces the same code files as the original.  All 25 programs
of the corpus (below), the compiler's own source among them, compile to code
files identical to SYSTEM.COMPILER's: on the host, and on the P-System from
the Tiny-C shell (`PC NAME`), where CC also rebuilds PC.CODE from its sources
(`CC @PASCALC:/SRC/BUILD`).  It also has a type DOUBLE (below), the emulator's
8-byte floating point.  Next: SYSTEM.COMPILER; then phase 2, which improves
the code it generates (smaller, tighter P-code; the emulator's extra CSPs).

## Goals

1. **Identical first.**  Translated from the Pascal source (UCSD-Pascal-Volumes'
   `U132.A_PASCAL_COMPILER_SOURCE_v1.08`, BIGGY's SYSTEM.COMPILER), it must give
   the same code file for every program -- the compiler's own source above all --
   and the same console output and listings.
2. **Runs where the old one runs.**  From the shell (`PC NAME`) while it is
   developed; at the end it becomes SYSTEM.COMPILER (the Command: prompt's
   C(ompile and R(un).
3. **Then better code.**  Once it is identical and stable, optimised code
   generation; then byte equality is replaced by behaviour (the same programs
   give the same results), and the compiler still compiles itself.

## How it is checked

* `tools/refcompile.py` compiles a corpus of Pascal programs with the real
  SYSTEM.COMPILER on the P-System (the emulator) into `reference/`: the
  compiler itself, the operating system and the Filer (OS source v1.14), the
  editor (L2), the linker, BASIC, the long-integer unit (PASCALIO), the U138
  utilities, BIGGY's MYGOTOXY and SAMPLEGOTO, and the emulator's validation
  programs -- 25 code files with their console transcripts.
* `make` builds the compiler on the host (`build/pc`, gcc) and
  `tools/compare.py` compiles the corpus with it and compares each code file
  with the reference: everything the code file defines (the segment
  dictionary, the code, the linker records, a unit's interface text), byte
  for byte.  Bytes the compiler never sets -- what was in memory, which the
  Pascal's NEW does not clear -- are found by compiling twice with new memory
  filled differently, and left out.
* `make psys` builds it with Tiny-C for the P-System (`build/psys/PC.CODE`);
  `tools/psysrun.py` compiles the corpus with it from the shell in the
  emulator and compares the same way.
* `tools/mkvolume.py` makes the volume PASCALC: (`build/PASCAL-C.BLK`), laid out
  as UCSD-TinyC's TINY-C: is, in sub-volumes: PC.CODE, README.TEXT and FILES.TEXT
  at the top, `SRC/` the sources with BUILD.TEXT, `OBJ/` (empty) for the objects
  BUILD makes, `DEMOS/` (PI) and `TESTS/` (DOUBLES, READTEST).
  `tools/psysbuild.py` runs `CC @PASCALC:/SRC/BUILD` on the P-System, which must
  give the host's PC.CODE (the objects in OBJ, none in SRC); `tools/voltest.py`
  uses the volume from the shell: `PC PASCALC:/DEMOS/PI` and the program run where
  it is, then `CD PASCALC:/TESTS`, `PC READTEST` and READTEST.
* On the P-System PC takes paths into sub-volumes as every Tiny-C program does
  (`PC PASCALC:/DEMOS/PI`, or a name on the shell's path after a CD), through the
  library's own (dirs.c: `__svplace`, `__svopenat`, `__svcloseat`); its include files
  and units too.

* `tools/doubletest.py` checks DOUBLE (below): the programs `tests/*.PAS`, compiled by
  the host and on the P-System (the same code file), run and print `tests/*.expect`.

See [docs/PLAN.md](docs/PLAN.md) for how the Pascal is translated.

## DOUBLE (not the Pascal's)

`DOUBLE` is a standard type: IEEE binary64, 8 bytes (16 significant digits,
exponents to ±308), done by the emulator's CSPs 100..137 (UCSD-TinyC's
`docs/DOUBLES.md`; in Z80 mode the emulator's coprocessor, engine 1.93 or later).
Programs without DOUBLE compile as before, byte for byte.

* **Constants**: `1.5D0`, `2D-3`, `3.14159265358979D0` are DOUBLEs (a `D` for the
  `E`).  A REAL literal or constant (`0.1`, `CONST TENTH = 0.1`) is a DOUBLE where
  it meets one -- `D := 0.1`, `TENTH * D`, a DOUBLE parameter -- made from its
  digits, so exact, not a widened REAL.
* **Conversions**: INTEGER and REAL become DOUBLE automatically, also on the left
  of an operator; nothing becomes smaller by itself: `SNGL(D)` (new) is D as a
  REAL, `TRUNC(D)` and `ROUND(D)` an INTEGER (as for a REAL; a DOUBLE outside
  -32768..32767 gives the nearest end).
* **Operators**: `+ - * /`, the sign, `= <> < <= > >=` (a DOUBLE with an INTEGER,
  a REAL or a DOUBLE).  Not `DIV`, `MOD`, `SUCC`, `PRED`, `ORD`, FOR, CASE,
  subranges or sets, as for a REAL.
* **Functions**: `ABS SQR SQRT SIN COS ATAN LN EXP LOG` (LOG is base 10) of a
  DOUBLE give a DOUBLE; with an INTEGER or a REAL they are the REAL ones.
* **Everywhere a REAL can be**: variables, VAR and value parameters, function
  results (4 words: the caller makes room), arrays and records.
* **WRITE**: `WRITE(D:W:F)` F decimals, as C's `%W.Ff`; `WRITE(D)` and
  `WRITE(D:W)` 16 significant digits, `%W.15e` (`3.141592653589793e+00`).  A
  width less than the text does not cut it.  (C's form: no space before a
  number, as REAL's WRITE puts.)
* **READ**: as READ of a REAL reads one -- blanks, then the number (digits,
  `+ - . E e`), its next character left in F^ -- by C's `strtod` (CSP 135).
* **Code**: WRITE and READ of a DOUBLE are made in place (CSP 133 into a string,
  then the system's string write; READ about 110 bytes), nothing from
  SYSTEM.LIBRARY: a program needs no L(ink for them.  A procedure's code is at most
  1299 bytes, so one with many READs and WRITEs of DOUBLEs may need splitting
  (error 253).
* **The compiler**: the code for DOUBLE (`src/double.c`, a module of its own so
  that CC compiles BODYPART.C in less memory) is in a segment of its own (1.3 KB,
  in the library's DIRS: a code file has at most 10 segments),
  in memory only while a DOUBLE is compiled.  The compiler's own source compiles
  from the shell with 3146 words to spare (3434 before sub-volume paths, which bring
  the C library's string functions into memory; 3777 before DOUBLE).

`tests/PI.PAS` is Tiny-C's `tests/pi.c` in Pascal and prints its `pi.expect`, byte
for byte; `tests/DOUBLES.PAS` and `tests/READTEST.PAS` try the rest.
