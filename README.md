# UCSD-Pascal-compiler-written-in-C

The UCSD Pascal II.0 compiler (`SYSTEM.COMPILER`, U132.A source revision 1.08)
rewritten in C for [Tiny-C](https://github.com/Bio-Printer/UCSD-TinyC), to replace
the Pascal one on the P-System.

**Status: phase 1 started** -- a routine-by-routine translation of the Pascal
compiler, which must produce the same code files as the original, byte for
byte.  Phase 2 then improves the code it generates (smaller, tighter P-code; the
emulator's extra CSPs; 8-byte doubles).

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
  programs -- 24 code files with their console transcripts.
* The C compiler builds on the host too (gcc), so it can be run on the whole
  corpus in seconds and compared byte for byte; then the same C, compiled by
  Tiny-C, on the P-System.

See [docs/PLAN.md](docs/PLAN.md) for how the Pascal is translated.
