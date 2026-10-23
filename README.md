# UCSD-Pascal-compiler-written-in-C

The UCSD Pascal II.0 compiler (`SYSTEM.COMPILER`, U132.A source revision 1.08)
rewritten in C for [Tiny-C](https://github.com/Bio-Printer/UCSD-TinyC), to replace
the Pascal one on the P-System.

**Status: phase 1 done** -- a routine-by-routine translation of the Pascal
compiler that produces the same code files as the original.  All 25 programs
of the corpus (below), the compiler's own source among them, compile to code
files identical to SYSTEM.COMPILER's: on the host, and on the P-System from
the Tiny-C shell (`PC NAME`), where CC also rebuilds PC.CODE from its sources
(`CC @PASCALC:BUILD`).  Next: SYSTEM.COMPILER; then phase 2, which improves
the code it generates (smaller, tighter P-code; the emulator's extra CSPs;
8-byte doubles).

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
* `tools/mkvolume.py` makes the volume PASCALC: (`build/PASCAL-C.BLK`: PC.CODE,
  its sources, BUILD.TEXT, README.TEXT); `tools/psysbuild.py` runs
  `CC @PASCALC:BUILD` on the P-System, which must give the host's PC.CODE.

See [docs/PLAN.md](docs/PLAN.md) for how the Pascal is translated.
