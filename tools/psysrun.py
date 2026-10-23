#!/usr/bin/env python3
"""psysrun.py -- the C compiler on the P-System: build/psys/PC.CODE (make psys)
compiles programs of the corpus from the Tiny-C shell, in the emulator, and
each code file is compared with the reference as compare.py does.

  psysrun.py [-v] [NAME ...]     (default: the whole corpus; -v: the transcripts)

The program's volume is unit #5 (the shell's cd #5), PC.CODE and the code
file are on a work volume, unit #13: "#13:pc NAME #13:NAME.CODE".  The boot
disk is UCSD-TinyC's BIGGY (build/BIGGY.BLK).  PSYS_MODE as in UCSD-TinyC.
The bytes the compiler never sets are those the host's two compiles find.
"""
import os, re, shutil, subprocess, sys, tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import compare
from compare import CORPUS, EXTRA, SMALL, BLK, ucsdvol
from refcompile import TINYC
sys.path.insert(0, os.path.join(TINYC, 'tools'))
from psys import ensure_setup, BUILD, shell_script, shell_transcript

PCCODE = os.environ.get('PCCODE', os.path.join(ROOT, 'build', 'psys', 'PC.CODE'))


def run_one(name):
    vol, src, code = CORPUS[name]
    d = tempfile.mkdtemp(prefix='pcpsys_')
    data = os.path.join(d, 'data')
    os.makedirs(data)
    for f in os.listdir(os.path.join(BUILD, 'data')):
        shutil.copy(os.path.join(BUILD, 'data', f), data)
    shutil.copy(os.path.join(BUILD, 'BIGGY.BLK'), os.path.join(data, 'Big_Disk.BLK'))
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
        for n, data_ in SMALL.items():
            v.write(n, data_, 3)
        open(unit5, 'wb').write(v.image())
    spare = os.path.join(d, 'SPARE.BLK')
    ucsdvol.main(['new', spare, 'SPARE', '100'])
    work = os.path.join(d, 'WORK.BLK')
    ucsdvol.main(['new', work, 'WORK', '1200'])
    w = ucsdvol.Volume(work)
    w.write('PC.CODE', open(PCCODE, 'rb').read(), 2)
    w.save()
    L = ['WAIT "Command:"', 'TYPE "S"', 'WAIT "shell> "', 'TYPE "cd #5\\r"', 'WAIT "shell> "',
         'TYPE "#13:pc %s #13:%s.CODE\\r"' % (src, name), 'WAIT "shell> "', 'TYPE "bye\\r"', 'WAIT "Command:"']
    sp = os.path.join(d, 'pc.script')
    open(sp, 'w').write(shell_script('\n'.join(L) + '\n'))
    env = dict(os.environ, VERIFY_UNIT13=work)
    r = subprocess.run([os.path.join(BUILD, 'run_verify'), data, unit5, spare, sp,
                        os.environ.get('PSYS_MODE', 'native'), os.path.join(d, 'out'), '', '7200'],
                       capture_output=True, text=True, env=env, cwd=d)
    tp = os.path.join(d, 'out', 'transcript.txt')
    tr = shell_transcript(open(tp, encoding='latin1').read()).replace('\r', '\n') if os.path.exists(tp) else ''
    got = None
    for p in (work, os.path.join(d, 'out', 'VERIFY_UNIT13.BLK')):
        try:
            got = ucsdvol.Volume(p).read(name + '.CODE')[0]
            break
        except (Exception, SystemExit):
            pass
    ok = 'VERIFY SCRIPT COMPLETED' in r.stdout
    shutil.rmtree(d, True)
    return ok, tr, got


def main(args):
    ensure_setup()
    verbose = '-v' in args
    names = [a for a in args if a != '-v'] or list(CORPUS)
    same = 0
    for n in names:
        ref = open(os.path.join(ROOT, 'reference', n + '.CODE'), 'rb').read()
        ok, tr, got = run_one(n)
        unset = []
        _, h1 = compare.compile_one(n, False)
        _, h2 = compare.compile_one(n, False, 0xEE)
        if h1 and h2 and len(h1) == len(h2):
            unset = [i for i in range(len(h1)) if h1[i] != h2[i]]
        if got is None:
            print('%-12s no code file%s' % (n, '' if ok else ' (the run did not finish)'))
            print(re.sub(r'\n+', '\n', tr)[-1500:])
        else:
            diffs = compare.differences(got, ref, unset)
            if not diffs and len(got) == len(ref):
                print('%-12s identical on the P-System (%d bytes)' % (n, len(got)))
                same += 1
                if verbose:
                    print(re.sub(r'\n+', '\n', tr)[-3000:])
            else:
                print('%-12s DIFFERENT: %d bytes (reference %d), %d differ, the first at %s'
                      % (n, len(got), len(ref), len(diffs), diffs[:1]))
                print(re.sub(r'\n+', '\n', tr)[-1500:])
        sys.stdout.flush()
    print('%d of %d identical' % (same, len(names)))
    return 0 if same == len(names) else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
