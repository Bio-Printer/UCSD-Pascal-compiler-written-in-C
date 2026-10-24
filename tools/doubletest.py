#!/usr/bin/env python3
"""doubletest.py -- DOUBLE (not the Pascal's: IEEE binary64, the emulator's
CSP 100..137): the programs tests/NAME.PAS, which SYSTEM.COMPILER cannot
compile, so there is no reference code file.  For each:

  * the host's PC (build/pc, make) compiles it; twice, with new memory
    filled differently, for the bytes it never sets (as compare.py);
  * on the P-System, from the Tiny-C shell, PC.CODE (build/psys/PC.CODE,
    make psys) compiles it again: the code file must be the host's;
  * that code file runs (from the shell) and prints tests/NAME.expect
    (what it reads typed as tests/NAME.input says, if there is one).

  doubletest.py [NAME ...]      (default: every tests/*.PAS; PSYS_MODE as in UCSD-TinyC)

The program's volume is unit #5, PC.CODE and the code file unit #13.  The
boot disk is UCSD-TinyC's BIGGY (build/BIGGY.BLK).  In Z80 mode the CSPs
100..137 are the emulator's coprocessor's (engine 1.93 or later).
"""
import os, re, shutil, subprocess, sys, tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from compare import PC, differences, ucsdvol
from refcompile import TINYC
sys.path.insert(0, os.path.join(TINYC, 'tools'))
from psys import ensure_setup, BUILD, shell_script, shell_transcript

TESTS = os.path.join(ROOT, 'tests')
PCCODE = os.environ.get('PCCODE', os.path.join(ROOT, 'build', 'psys', 'PC.CODE'))


def host_compile(name, fill):
    d = tempfile.mkdtemp(prefix='pcd_')
    open(os.path.join(d, name + '.TEXT'), 'wb').write(
        ucsdvol.text_to_ucsd(open(os.path.join(TESTS, name + '.PAS')).read()))
    r = subprocess.run([PC, name + '.TEXT', 'OUT.CODE'], cwd=d, capture_output=True, text=True,
                       encoding='latin1', timeout=600, env=dict(os.environ, PCFILL=str(fill)))
    got = open(os.path.join(d, 'OUT.CODE'), 'rb').read() if os.path.exists(os.path.join(d, 'OUT.CODE')) else None
    shutil.rmtree(d, True)
    return r, got


def keys(name):
    """what is typed while NAME runs: tests/NAME.input, script lines (WAIT "...", TYPE "...")"""
    p = os.path.join(TESTS, name + '.input')
    return open(p).read().splitlines() if os.path.exists(p) else []


def psys_run(name):
    """PC.CODE compiles NAME on the P-System, then NAME runs: (completed, its output, the code file)"""
    d = tempfile.mkdtemp(prefix='pcdbl_')
    data = os.path.join(d, 'data')
    os.makedirs(data)
    for f in os.listdir(os.path.join(BUILD, 'data')):
        shutil.copy(os.path.join(BUILD, 'data', f), data)
    shutil.copy(os.path.join(BUILD, 'BIGGY.BLK'), os.path.join(data, 'Big_Disk.BLK'))
    unit5 = os.path.join(d, 'SRC.BLK')
    v = ucsdvol.Volume.new('DBL', 400)
    v.write(name + '.TEXT', ucsdvol.text_to_ucsd(open(os.path.join(TESTS, name + '.PAS')).read()), 3)
    open(unit5, 'wb').write(v.image())
    spare = os.path.join(d, 'SPARE.BLK')
    ucsdvol.main(['new', spare, 'SPARE', '100'])
    work = os.path.join(d, 'WORK.BLK')
    ucsdvol.main(['new', work, 'WORK', '400'])
    w = ucsdvol.Volume(work)
    w.write('PC.CODE', open(PCCODE, 'rb').read(), 2)
    w.save()
    L = ['WAIT "Command:"', 'TYPE "S"', 'WAIT "shell> "', 'TYPE "cd #5\\r"', 'WAIT "shell> "',
         'TYPE "#13:pc %s #13:%s.CODE\\r"' % (name, name), 'WAIT "shell> "',
         'TYPE "#13:%s\\r"' % name] + keys(name) + ['WAIT "shell> "', 'TYPE "bye\\r"', 'WAIT "Command:"']
    sp = os.path.join(d, 'dbl.script')
    open(sp, 'w').write(shell_script('\n'.join(L) + '\n'))
    r = subprocess.run([os.path.join(BUILD, 'run_verify'), data, unit5, spare, sp,
                        os.environ.get('PSYS_MODE', 'native'), os.path.join(d, 'out'), '', '3600'],
                       capture_output=True, text=True, env=dict(os.environ, VERIFY_UNIT13=work), cwd=d)
    tp = os.path.join(d, 'out', 'transcript.txt')
    tr = shell_transcript(open(tp, encoding='latin1').read()).replace('\r', '\n') if os.path.exists(tp) else ''
    got = None
    try:
        got = ucsdvol.Volume(work).read(name + '.CODE')[0]
    except (Exception, SystemExit):
        pass
    shutil.rmtree(d, True)
    run = 'shell> #13:%s\n' % name
    out = tr[tr.find(run) + len(run):] if run in tr else ''
    out = out[:out.find('shell> ')] if 'shell> ' in out else out
    return 'VERIFY SCRIPT COMPLETED' in r.stdout, out, got, tr


def normalize(out):
    out = re.sub(r'\[exit status \d+\]\n*$', '', out.strip('\n'))   # (what the shell says after it)
    out = out.replace('\n\n', '\n')         # (the console's CR LF)
    return '\n'.join(l.rstrip() for l in out.strip('\n').split('\n')) + '\n'


def main(args):
    ensure_setup()
    names = args or sorted(f[:-4] for f in os.listdir(TESTS) if f.endswith('.PAS'))
    good = 0
    for n in names:
        r, h1 = host_compile(n, 0)
        _, h2 = host_compile(n, 0xEE)
        if h1 is None or r.returncode:
            print('%-10s the host does not compile it:\n%s' % (n, r.stdout[-1500:]))
            continue
        unset = [i for i in range(len(h1)) if h1[i] != h2[i]] if h2 and len(h1) == len(h2) else []
        ok, out, got, tr = psys_run(n)
        same = got is not None and len(got) == len(h1) and not differences(got, h1, unset)
        expect = open(os.path.join(TESTS, n + '.expect')).read()
        right = normalize(out) == expect
        print('%-10s P-System code %s the host\'s (%d bytes); its output %s%s' % (
            n, 'IDENTICAL to' if same else 'DIFFERS from', len(h1), 'as expected' if right else 'DIFFERS',
            '' if ok else ' (the run did not finish)'))
        if not right:
            print(re.sub(r'\n+', '\n', tr)[-2500:])
        good += same and right
    print('%d of %d right' % (good, len(names)))
    return 0 if good == len(names) else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
