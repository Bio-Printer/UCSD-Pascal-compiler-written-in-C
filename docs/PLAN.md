# How the Pascal compiler is translated

The source is U132.A revision 1.08 (16 files, about 6,000 lines): COMPGLBLS
(types and globals), COMPINIT, DECPART.A-C (declarations), BODYPART.A-E (code
generation: expressions, calls, statements), UNITPART (units, linker info),
PROCS.A-B (scanner, errors, symbol search, code buffer), BLOCK.  One C file per
Pascal file, the routines in the same order with the same names, so the two can
be read side by side.  It differs from v1.00 (the distribution) only in
BLOCK.TEXT's compile summary.

## Pascal constructs

| Pascal | C |
|---|---|
| records with variants (`STRUCTURE`, `IDENTIFIER`, `ATTR`) | structs with unions; `NEW(P, tags...)` allocates the size Pascal would (only the variant named), so memory use stays comparable |
| `SET OF SYMBOL` (55 members), `SET OF IDCLASS` | a few 16-bit words (`SETOFSYS` 4 words), with `IN`, `+`, `-` as small functions; a set parameter is passed by its address, constant sets are built once (`INITSETS`) |
| nested procedures using their parents' variables (`TYP` and its `SIMPLETYPE`, `FIELDLIST`, ...; `EXPRESSION` / `TERM` / `FACTOR`) | the parent's variables in a struct, its address passed to the nested function -- they recurse (`TYP` -> `FIELDLIST` -> `TYP`), so they cannot simply be globals |
| `EXIT(PROC)` (leave several levels: `EXIT(PASCALCOMPILER)` on an error) | `setjmp` / `longjmp` |
| `MARK` / `RELEASE` (a block's symbols freed at its end) | the same, on the P-System heap (Tiny-C's heap mark); an arena with marks on the host |
| `IDSEARCH`, `TREESEARCH` (the interpreter's built-ins for identifiers and the symbol trees) | C functions doing exactly what the interpreter does (its reserved-word table, its comparison) |
| `REAL` (constants: `NUMBER`, negation) | Tiny-C's float, the P-machine's real; on the host the P-machine's real format and operations |
| `BLOCKREAD` / `BLOCKWRITE` on the work files, `OPENOLD` / `OPENNEW`, `TEXT` output | a small I/O layer: on the P-System the operating system's file procedures, on the host plain files (`.TEXT` as UCSD text pages) |
| segments (`DECLARATIONPART`, `BODYPART`, `UNITPART`, ...) | `#pragma segment`, as the Pascal ones, so that it fits in memory |

## What must be the same

* The code file, byte for byte -- including the bytes the Pascal compiler leaves
  unwritten: the code buffer and the disk buffer are kept and reused exactly as
  it does (alignment bytes keep what an earlier procedure left there).  Only
  what was in them before their first use (heap and stack contents) cannot be
  reproduced; the comparison reports such bytes separately.
* The console output (the dots, `<  N>`, procedure names with `[ N words]` --
  the free memory differs --, errors and the `<sp>(continue)` question), the
  listing (`$L`), the error numbers.
* The interface with the system: USERINFO (the work files, ERRSYM / ERRBLK /
  ERRNUM for the Editor after an error), SYSTEM.LIBRARY for units, include files.

## Steps

1. Reference corpus (done: `tools/refcompile.py`, `reference/`).
2. The translation, host build first: scanner and errors, declarations,
   expressions and statements, units, block -- compared on the corpus as soon as
   whole programs compile.
3. Byte-identical on the whole corpus; the compiler's own source compiled by it.
4. Tiny-C build on the P-System, from the shell (`PC NAME`); memory (segments,
   heap) tuned; compiling the compiler there.
5. The PASCAL-C: volume (sources, the compiler) in UCSD-Pascal-Volumes.
6. Phase 2: better code generation; SYSTEM.COMPILER.
