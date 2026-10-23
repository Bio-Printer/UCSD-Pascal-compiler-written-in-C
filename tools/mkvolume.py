#!/usr/bin/env python3
"""mkvolume.py -- the volume PASCALC: (build/PASCAL-C.BLK; a volume's name is
at most 7 characters): the compiler
(build/psys/PC.CODE, make psys), its P-System sources as text files, the
batch file that builds it with CC (BUILD.TEXT), a README.TEXT and a
FILES.TEXT.  Uses UCSD-TinyC beside this repository ($TINYC)."""
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from refcompile import TINYC
sys.path.insert(0, os.path.join(TINYC, 'tools'))
import ucsdvol

VOLUME = os.path.join(ROOT, 'build', 'PASCAL-C.BLK')

# the modules of the P-System build, in the Makefile's order (PSRC)
MODULES = ['globals', 'builtin', 'sets', 'heap', 'procs', 'compinit', 'decpart', 'bodypart',
           'unitpart', 'block', 'rare', 'pcio_psys', 'pc_psys']

BUILD = """; BUILD -- the Pascal compiler in C: CC @PASCALC:BUILD from the shell (or
; X(ecute TINY-C:CC and answer @PASCALC:BUILD), TINY-C: on line.  Compiles
; every module (-O: each NAME.OBJ onto PASCALC:), then links them into
; PASCALC:PC2.CODE, the same as PC.CODE.
-O PASCALC:
""" + ''.join('-C %s\n' % m.upper() for m in MODULES) + \
    '-L PASCALC:PC2=%s\n' % ','.join(m.upper() for m in MODULES)

WHAT = {
    'globals': 'the global variables',
    'builtin': 'the interpreter\'s TREESEARCH and SCAN',
    'sets': 'Pascal\'s sets',
    'heap': 'NEW, MARK, RELEASE, MEMAVAIL',
    'procs': 'PROCS.A, PROCS.B: the scanner, the symbol table, the code buffer',
    'compinit': 'COMPINIT: the standard names',
    'decpart': 'DECPART.A-C: DECLARATIONPART',
    'bodypart': 'BODYPART.A-E: code, expressions, statements',
    'unitpart': 'UNITPART: WRITELINKERINFO, UNITPART',
    'block': 'BLOCK, and the compiler\'s main body',
    'rare': 'ERROR, the options, long and REAL constants, the end',
    'pcio_psys': 'the system\'s files, the console',
    'pc_psys': 'PC NAME [CODE]',
}


def main():
    pc = os.path.join(ROOT, 'build', 'psys', 'PC.CODE')
    if not os.path.exists(pc):
        raise SystemExit('no build/psys/PC.CODE: make psys')
    if os.path.exists(VOLUME):
        os.remove(VOLUME)
    ucsdvol.main(['new', VOLUME, 'PASCALC', '1000'])
    v = ucsdvol.Volume(VOLUME)
    files = [('README.TEXT', ucsdvol.text_to_ucsd(open(os.path.join(ROOT, 'src', 'readme.txt')).read()), 3,
              'what is on PASCALC:, how to use and build it'),
             ('PC.CODE', open(pc, 'rb').read(), 2, 'the compiler: PC NAME [CODE] from the shell'),
             ('BUILD.TEXT', ucsdvol.text_to_ucsd(BUILD), 3, 'CC @PASCALC:BUILD makes PC2.CODE (= PC.CODE)'),
             ('COMPGLBL.H', ucsdvol.text_to_ucsd(open(os.path.join(ROOT, 'src', 'compglbl.h')).read()), 3,
              'COMPGLBLS: constants, types, global variables'),
             ('PCIO.H', ucsdvol.text_to_ucsd(open(os.path.join(ROOT, 'src', 'pcio.h')).read()), 3,
              'what the compiler reads and writes')]
    for m in MODULES:
        files.append((m.upper() + '.C', ucsdvol.text_to_ucsd(open(os.path.join(ROOT, 'src', m + '.c')).read()), 3,
                      WHAT[m]))
    listing = ['PASCALC:  the UCSD Pascal compiler written in C (PASCAL-C.BLK)', '',
               '  %-15s %6s  %s' % ('FILE', 'BLOCKS', 'WHAT'), '']
    for name, raw, kind, what in files:
        listing.append('  %-15s %6d  %s' % (name, (len(raw) + 511) // 512, what))
    listing.append('  %-15s %6s  %s' % ('FILES.TEXT', '', 'this list'))
    v.write('FILES.TEXT', ucsdvol.text_to_ucsd('\n'.join(listing) + '\n'), 3)
    for name, raw, kind, what in files:
        v.write(name, raw, kind)
    v.save()
    print('%s: %d files' % (os.path.relpath(VOLUME, ROOT), len(files) + 1))


if __name__ == '__main__':
    sys.exit(main())
