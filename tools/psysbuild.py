#!/usr/bin/env python3
"""psysbuild.py -- PASCALC:BUILD on the P-System: from the Tiny-C shell,
CC @PASCALC:BUILD compiles the compiler's modules on PASCALC:
(build/PASCAL-C.BLK, tools/mkvolume.py) and links PASCALC:PC2.CODE, which
must be PC.CODE byte for byte (the host's Tiny-C made that one).

  psysbuild.py          (PSYS_MODE as in UCSD-TinyC)

TINY-C: is unit #5 (UCSD-TinyC's build/TINY-C.BLK), PASCALC: unit #13.
"""
import os, re, shutil, subprocess, sys, tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from refcompile import TINYC
sys.path.insert(0, os.path.join(TINYC, 'tools'))
import ucsdvol
from psys import ensure_setup, BUILD, shell_script, shell_transcript


def main():
    ensure_setup()
    d = tempfile.mkdtemp(prefix='pcbuild_')
    data = os.path.join(d, 'data')
    os.makedirs(data)
    for f in os.listdir(os.path.join(BUILD, 'data')):
        shutil.copy(os.path.join(BUILD, 'data', f), data)
    shutil.copy(os.path.join(BUILD, 'BIGGY.BLK'), os.path.join(data, 'Big_Disk.BLK'))
    shutil.copy(os.path.join(BUILD, 'TINY-C.BLK'), d)
    vol = os.path.join(d, 'PASCAL-C.BLK')
    shutil.copy(os.path.join(ROOT, 'build', 'PASCAL-C.BLK'), vol)
    spare = os.path.join(d, 'SPARE.BLK')
    ucsdvol.main(['new', spare, 'SPARE', '100'])
    L = ['WAIT "Command:"', 'TYPE "S"', 'WAIT "shell> "', 'TYPE "cc @pascalc:build\\r"',
         'WAIT "shell> "', 'TYPE "bye\\r"', 'WAIT "Command:"']
    sp = os.path.join(d, 'build.script')
    open(sp, 'w').write(shell_script('\n'.join(L) + '\n'))
    r = subprocess.run([os.path.join(BUILD, 'run_verify'), data, os.path.join(d, 'TINY-C.BLK'), spare, sp,
                        os.environ.get('PSYS_MODE', 'native'), os.path.join(d, 'out'), '', '7200'],
                       capture_output=True, text=True, env=dict(os.environ, VERIFY_UNIT13=vol), cwd=d)
    tp = os.path.join(d, 'out', 'transcript.txt')
    tr = shell_transcript(open(tp, encoding='latin1').read()).replace('\r', '\n') if os.path.exists(tp) else ''
    print(re.sub(r'\n+', '\n', tr)[-2500:])
    v = ucsdvol.Volume(vol)
    ok = False
    if v.find('PC2.CODE'):
        pc2 = v.read('PC2.CODE')[0]
        pc = v.read('PC.CODE')[0]
        ok = pc2 == pc
        print('PC2.CODE %s PC.CODE (%d bytes)' % ('IDENTICAL to' if ok else 'DIFFERS from', len(pc2)))
    else:
        print('no PC2.CODE')
    shutil.rmtree(d, True)
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
