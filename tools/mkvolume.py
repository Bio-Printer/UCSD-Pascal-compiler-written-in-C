#!/usr/bin/env python3
"""mkvolume.py -- the volume PASCALC: (build/PASCAL-C.BLK; a volume's name is
at most 7 characters), laid out as TINY-C: is, in sub-volumes:

  PC.CODE, README.TEXT, FILES.TEXT   the compiler (build/psys/PC.CODE, make psys)
  SRC/     its P-System sources as text files, and BUILD.TEXT (CC @PASCALC:/SRC/BUILD)
  OBJ/     empty: the objects BUILD makes (-O PASCALC:/OBJ)
  DEMOS/   programs to try (PC PASCALC:/DEMOS/PI, then PASCALC:/DEMOS/PI)
  TESTS/   the tests of DOUBLE (tools/doubletest.py)

Made with UCSD-TinyC's mkvolume (its Dir: the sub-volumes made on the host, as
the shell's MKDIR makes them); uses UCSD-TinyC beside this repository ($TINYC)."""
import importlib.util, os, subprocess, sys, tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from refcompile import TINYC
sys.path.insert(0, os.path.join(TINYC, 'tools'))
import ucsdvol
_spec = importlib.util.spec_from_file_location('tcmkvolume', os.path.join(TINYC, 'tools', 'mkvolume.py'))
tcmkvolume = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(tcmkvolume)
Dir, ROOM = tcmkvolume.Dir, tcmkvolume.ROOM

VOLUME = os.path.join(ROOT, 'build', 'PASCAL-C.BLK')

# the modules of the P-System build, in the Makefile's order (PSRC)
MODULES = ['globals', 'builtin', 'sets', 'heap', 'procs', 'compinit', 'decpart', 'bodypart', 'double',
           'unitpart', 'block', 'rare', 'pcio_psys', 'pc_psys']

BUILD = """; BUILD -- the Pascal compiler in C: CC @PASCALC:/SRC/BUILD from the shell (or
; X(ecute TINY-C:CC and answer @PASCALC:/SRC/BUILD), TINY-C: on line.  Compiles
; every module (the names beside this file; -O: each NAME.OBJ into PASCALC:/OBJ),
; then links them into PASCALC:PC.CODE, the same as the one it replaces.
-O PASCALC:/OBJ
""" + ''.join('-C %s\n' % m.upper() for m in MODULES) + \
    '-L PASCALC:PC=%s\n' % ','.join(m.upper() for m in MODULES)

WHAT = {
    'globals': 'the global variables',
    'builtin': 'the interpreter\'s TREESEARCH and SCAN',
    'sets': 'Pascal\'s sets',
    'heap': 'NEW, MARK, RELEASE, MEMAVAIL',
    'procs': 'PROCS.A, PROCS.B: the scanner, the symbol table, the code buffer',
    'compinit': 'COMPINIT: the standard names',
    'decpart': 'DECPART.A-C: DECLARATIONPART',
    'bodypart': 'BODYPART.A-E: code, expressions, statements',
    'double': 'DOUBLE\'s code (BODYPART calls it)',
    'unitpart': 'UNITPART: WRITELINKERINFO, UNITPART',
    'block': 'BLOCK, and the compiler\'s main body',
    'rare': 'ERROR, the options, long and REAL constants, the end',
    'pcio_psys': 'the system\'s files, the console',
    'pc_psys': 'PC NAME [CODE]',
}


# programs to try, and the tests of DOUBLE (tests/, tools/doubletest.py): PC NAME, then NAME
DEMOS = [('PI', 'pi by Archimedes\' method, in DOUBLEs (Tiny-C\'s PI.C)')]
TESTS = [('DOUBLES', 'DOUBLE: arithmetic, functions, WRITE, parameters'),
         ('READTEST', 'DOUBLE: READ from the console and a file')]


def objblocks():
    """the blocks the objects take (the host's CC makes the same ones)"""
    d = tempfile.mkdtemp(prefix='pcobj_')
    n = 0
    for m in MODULES:
        o = os.path.join(d, m + '.obj')
        subprocess.run([os.path.join(TINYC, 'build', 'tc'), '-c', '-I', os.path.join(TINYC, 'tinyc', 'include'),
                        os.path.join(ROOT, 'src', m + '.c'), '-o', o], capture_output=True, check=True)
        n += (os.path.getsize(o) + 511) // 512
    return n


def main():
    pc = os.path.join(ROOT, 'build', 'psys', 'PC.CODE')
    if not os.path.exists(pc):
        raise SystemExit('no build/psys/PC.CODE: make psys')
    v = Dir('PASCALC')
    v.binary('PC.CODE', open(pc, 'rb').read(), 'the compiler: PC NAME [CODE] from the shell')
    v.textfile('README.TEXT', os.path.join(ROOT, 'src', 'readme.txt'), 'what is on PASCALC:, how to use and build it')
    src = v.sub('SRC', 'the compiler\'s sources and BUILD.TEXT')
    src.text('BUILD.TEXT', BUILD, 'CC @PASCALC:/SRC/BUILD makes PASCALC:PC.CODE again')
    src.textfile('COMPGLBL.H', os.path.join(ROOT, 'src', 'compglbl.h'), 'COMPGLBLS: constants, types, global variables')
    src.textfile('PCIO.H', os.path.join(ROOT, 'src', 'pcio.h'), 'what the compiler reads and writes')
    src.textfile('GEN.H', os.path.join(ROOT, 'src', 'gen.h'), 'what BODYPART.C and DOUBLE.C share')
    for m in MODULES:
        src.textfile(m.upper() + '.C', os.path.join(ROOT, 'src', m + '.c'), WHAT[m])
    v.sub('OBJ', 'empty: the objects BUILD makes (-O PASCALC:/OBJ)', 6 + objblocks() + ROOM)
    for name, progs in (('DEMOS', DEMOS), ('TESTS', TESTS)):
        d = v.sub(name, 'programs to try: PC PASCALC:/DEMOS/NAME, then PASCALC:/DEMOS/NAME' if name == 'DEMOS'
                  else 'the tests of DOUBLE: PC PASCALC:/TESTS/NAME, then PASCALC:/TESTS/NAME')
        for t, what in progs:
            d.textfile(t + '.TEXT', os.path.join(ROOT, 'tests', t + '.PAS'), what)
    for d in [r for n, r, k, w in v.items if isinstance(r, Dir)]:
        d.made()
    v.items.append(['FILES.TEXT', b'', 3, 'this list'])
    # room for a second PC.CODE (BUILD links the new one while the old one is there) and more
    v.blocks = 6 + sum(max(1, (len(r.made().image() if isinstance(r, Dir) else r) + 511) // 512)
                       for n, r, k, w in v.items) + 2 * ((len(open(pc, 'rb').read()) + 511) // 512) + ROOM
    for _ in range(2):                          # (once to learn its own size)
        listing = '\n'.join(['PASCALC:  the UCSD Pascal compiler written in C (PASCAL-C.BLK): every file,',
                             '          the sub-volumes\' too (A/NAME: NAME in the sub-volume A)', '',
                             '  %-24s %6s  %s' % ('FILE', 'BLOCKS', 'WHAT'), ''] + v.listing()) + '\n'
        v.items[-1][1] = ucsdvol.text_to_ucsd(listing)
    open(VOLUME, 'wb').write(v.made().image())
    print('%s: %d files' % (os.path.relpath(VOLUME, ROOT), len(v.listing())))


if __name__ == '__main__':
    sys.exit(main())
