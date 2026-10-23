#!/usr/bin/env python3
"""refcompile.py -- the reference: every Pascal program of the corpus compiled
by BIGGY's own SYSTEM.COMPILER (U132.A_PASCAL_COMPILER_SOURCE_v1.08) on the
P-System, in the emulator.

  refcompile.py [NAME ...]      (default: the whole corpus; see CORPUS)

For each program its source volume is unit #5 (the prefix is set to it), the
Command: prompt's C(ompile is given NAME and the code file's name, and the
code file and the console transcript go to reference/NAME.CODE and
reference/NAME.LOG.  The C compiler's output is compared with these
(tools/compare.py).  Uses UCSD-TinyC's build/run_verify and
UCSD-Pascal-Volumes beside this repository ($TINYC, $VOLUMES).
"""
import os, re, shutil, subprocess, sys, tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
TINYC = os.environ.get('TINYC', os.path.join(os.path.dirname(ROOT), 'ucsd-tinyc'))
VOLUMES = os.environ.get('VOLUMES', os.path.join(os.path.dirname(ROOT), 'UCSD-Pascal-Volumes'))
sys.path.insert(0, os.path.join(TINYC, 'tools'))
import ucsdvol

BLK = os.path.join(VOLUMES, 'BLK_format')
VALIDATION = os.path.join(os.path.dirname(ROOT), 'ucsd-pascal_windows_emulator', 'validation', 'pascal_programs')

# name: (source volume, or None for a volume made of FILES; the text to compile; the code file)
CORPUS = {
    'COMPILER': ('U132.A_PASCAL_COMPILER_SOURCE_v1.08.BLK', 'COMPILER', 'COMPILER'),
    'OS':       ('U134.4_OS_SOURCE_v1.14.BLK', 'SYSTEM', 'OS'),
    'FILER':    ('U134.4_OS_SOURCE_v1.14.BLK', 'II.0.FILER', 'FILER'),
    'L2':       ('U128_L2_YALOE_SOURCE_v1.06.BLK', 'L2', 'L2'),         # (the editor: L2 includes YALOE)
    'LINKER':   ('U132B.2_PASCAL_LINKER_SOURCE.BLK', 'LINK', 'LINKER'),
    'PASCALIO': ('U136.3_LONG_INTEGER_SOURCE.BLK', 'PASCALIO', 'PASCALIO'),
    'BASIC':    ('U130.1_BASIC_SOURCE.BLK', 'BASIC', 'BASIC'),
    'BASICIO':  ('U130.1_BASIC_SOURCE.BLK', 'BASICIO', 'BASICIO'),
}
for _u in ('COPYDUPDIR', 'DISASM.II', 'EDITTORT11', 'LIBMAP', 'MARKDUPDIR', 'RT11TOEDIT', 'SETUP'):
    CORPUS[_u.replace('.', '')] = ('U138.3_UTILITY_SOURCE.BLK', _u, _u.replace('.', '')[:10])


# files a source includes that are not on its volume: put there from another
# (none now: GLOBALS.TEXT, which BASICIO includes, is on U130.1 too)
EXTRA = {}


def small_programs():
    """the small ones, on a volume of their own: BIGGY's MYGOTOXY and SAMPLEGOTO, and the
    emulator's validation programs"""
    files = {}
    big = ucsdvol.Volume(os.path.join(BLK, 'Big_Disk.BLK'))
    for n in ('MYGOTOXY.TEXT', 'SAMPLEGOTO.TEXT'):
        files[n] = big.read(n)[0]
    if os.path.isdir(VALIDATION):
        for f in sorted(os.listdir(VALIDATION)):
            if f.endswith('.pas'):
                name = re.sub(r'[^A-Z0-9]', '', f[:-4].upper())[:10] + '.TEXT'
                files[name] = ucsdvol.text_to_ucsd(open(os.path.join(VALIDATION, f), encoding='latin1').read())
    return files


SMALL = small_programs()
for _n in SMALL:
    CORPUS[_n[:-5]] = (None, _n[:-5], _n[:-5])


def compile_one(name, out_dir):
    vol, src, code = CORPUS[name]
    d = tempfile.mkdtemp(prefix='refc_')
    os.makedirs(os.path.join(d, 'data'))
    shutil.copy(os.path.join(TINYC, 'build', 'data', 'pascal.bin'), os.path.join(d, 'data'))
    shutil.copy(os.path.join(BLK, 'Big_Disk.BLK'), os.path.join(d, 'data', 'Big_Disk.BLK'))
    unit5 = os.path.join(d, 'SRC.BLK')
    if vol:
        shutil.copy(os.path.join(BLK, vol), unit5)
        if name in EXTRA:
            v = ucsdvol.Volume(unit5)
            for n, (ovol, ofile) in EXTRA[name].items():
                v.write(n, *ucsdvol.Volume(os.path.join(BLK, ovol)).read(ofile))
            v.save()
    else:
        v = ucsdvol.Volume.new('SMALL', 2000)
        for n, data in SMALL.items():
            v.write(n, data, 3)
        open(unit5, 'wb').write(v.image())
    ucsdvol.main(['new', os.path.join(d, 'S.BLK'), 'S', '100'])
    script = ('WAIT "Command:"\nTYPE "F"\nWAIT "Filer:"\nTYPE "P"\nWAIT "Prefix"\nTYPE "#5:\\r"\n'
              'WAIT "Filer:"\nTYPE "Q"\nWAIT "Command:"\nTYPE "C"\nWAIT "Compile what text?"\n'
              'TYPE "%s\\r"\nWAIT "To what codefile?"\nTYPE "%s\\r"\nWAIT "Command:"\n' % (src, code))
    open(os.path.join(d, 'c.script'), 'w').write(script)
    r = subprocess.run([os.path.join(TINYC, 'build', 'run_verify'), os.path.join(d, 'data'), unit5,
                        os.path.join(d, 'S.BLK'), os.path.join(d, 'c.script'), 'native', os.path.join(d, 'out'),
                        '', '1800'], capture_output=True, text=True, cwd=d)   # (its printer file goes there)
    tr = ''
    t = os.path.join(d, 'out', 'transcript.txt')
    if os.path.exists(t):
        tr = open(t, encoding='latin1').read()
    ok = 'VERIFY SCRIPT COMPLETED' in r.stdout
    got = None
    try:
        got = ucsdvol.Volume(os.path.join(d, 'out', 'VERIFY_SOURCE.BLK')).read(code + '.CODE')[0]
    except (Exception, SystemExit):
        ok = False
    open(os.path.join(out_dir, name + '.LOG'), 'w', encoding='latin1').write(tr)
    if got is not None:
        open(os.path.join(out_dir, name + '.CODE'), 'wb').write(got)
    shutil.rmtree(d, True)
    return ok, len(got) if got else 0


def main(args):
    out_dir = os.path.join(ROOT, 'reference')
    os.makedirs(out_dir, exist_ok=True)
    names = args or list(CORPUS)
    bad = 0
    for n in names:
        ok, size = compile_one(n, out_dir)
        print('%-4s %-12s %6d bytes' % ('ok' if ok else 'BAD', n, size))
        sys.stdout.flush()
        bad += not ok
    print('%d of %d compiled' % (len(names) - bad, len(names)))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
