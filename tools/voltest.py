#!/usr/bin/env python3
"""voltest.py -- PASCALC: (build/PASCAL-C.BLK, tools/mkvolume.py) used as it is
meant to be, from the Tiny-C shell, with its files in their sub-volumes:

  pc pascalc:/demos/pi        a path: PI.TEXT read in DEMOS, PI.CODE made there
  pascalc:/demos/pi           ... and run where it is: the output of tests/PI.expect
  cd pascalc:/tests           the shell's path (CD): names without a volume are there
  pc readtest                 READTEST.TEXT and READTEST.CODE in TESTS
  readtest                    tests/READTEST.input typed: tests/READTEST.expect
                              (its own DBL.TEXT: on the prefix volume, which the CD made
                              PASCALC: -- a Pascal program's files are the OS's, no sub-volumes)

and on the host: PI.CODE and READTEST.CODE in their sub-volumes, each the same as
the host's PC makes (the bytes it never sets aside), nothing else new but DBL.TEXT.

  voltest.py        (PSYS_MODE as in UCSD-TinyC)

PASCALC: is unit #13, the boot disk UCSD-TinyC's BIGGY (build/BIGGY.BLK).
"""
import os, re, shutil, subprocess, sys, tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from compare import differences, ucsdvol
from doubletest import TESTS, host_compile, keys, normalize
from refcompile import TINYC
sys.path.insert(0, os.path.join(TINYC, 'tools'))
from psys import ensure_setup, BUILD, shell_script, shell_transcript


def output(tr, cmd):
    """what the command printed (after its line, to the next prompt)"""
    i = tr.find('shell> ' + cmd + '\n')
    if i < 0:
        return ''
    out = tr[i + len('shell> ' + cmd + '\n'):]
    return out[:out.find('shell> ')] if 'shell> ' in out else out


def main():
    ensure_setup()
    d = tempfile.mkdtemp(prefix='pcvol_')
    data = os.path.join(d, 'data')
    os.makedirs(data)
    for f in os.listdir(os.path.join(BUILD, 'data')):
        shutil.copy(os.path.join(BUILD, 'data', f), data)
    shutil.copy(os.path.join(BUILD, 'BIGGY.BLK'), os.path.join(data, 'Big_Disk.BLK'))
    vol = os.path.join(d, 'PASCAL-C.BLK')
    shutil.copy(os.path.join(ROOT, 'build', 'PASCAL-C.BLK'), vol)
    before = {e[3] for e in ucsdvol.Volume(vol).entries}
    work = os.path.join(d, 'WORK.BLK')
    ucsdvol.main(['new', work, 'WORK', '400'])
    spare = os.path.join(d, 'SPARE.BLK')
    ucsdvol.main(['new', spare, 'SPARE', '100'])
    cmds = ['cd #5', 'pc pascalc:/demos/pi', 'pascalc:/demos/pi', 'cd pascalc:/tests', 'pc readtest', 'readtest']
    L = ['WAIT "Command:"', 'TYPE "S"']
    for c in cmds:
        L += ['WAIT "shell> "', 'TYPE "%s\\r"' % c]
        if c == 'readtest':
            L += keys('READTEST')
    L += ['WAIT "shell> "', 'TYPE "bye\\r"', 'WAIT "Command:"']
    sp = os.path.join(d, 'vol.script')
    open(sp, 'w').write(shell_script('\n'.join(L) + '\n'))
    r = subprocess.run([os.path.join(BUILD, 'run_verify'), data, work, spare, sp,
                        os.environ.get('PSYS_MODE', 'native'), os.path.join(d, 'out'), '', '3600'],
                       capture_output=True, text=True, env=dict(os.environ, VERIFY_UNIT13=vol), cwd=d)
    tp = os.path.join(d, 'out', 'transcript.txt')
    tr = shell_transcript(open(tp, encoding='latin1').read()).replace('\r', '\n') if os.path.exists(tp) else ''
    tr = re.sub(r'shell> ', 'shell> ', re.sub(r'[A-Z0-9-]*:[/A-Z0-9]*> ', 'shell> ', tr))  # (the prompt shows the path)
    v = ucsdvol.Volume(vol)
    checks = [('the script completed', 'VERIFY SCRIPT COMPLETED' in r.stdout)]
    for sub, name in (('DEMOS', 'PI'), ('TESTS', 'READTEST')):
        h1 = host_compile(name, 0)[1]
        h2 = host_compile(name, 0xEE)[1]
        unset = [i for i in range(len(h1)) if h1[i] != h2[i]]
        try:
            got = v.sub(sub).read(name + '.CODE')[0]
        except (Exception, SystemExit):
            got = None
        checks.append(('%s/%s.CODE made there, the same as the host\'s' % (sub, name),
                       got is not None and len(got) == len(h1) and not differences(got, h1, unset)))
        run = 'pascalc:/demos/pi' if name == 'PI' else 'readtest'
        checks.append(('%s printed tests/%s.expect' % (run, name),
                       normalize(output(tr, run)) == open(os.path.join(TESTS, name + '.expect')).read()))
    checks.append(('at the top of PASCALC: nothing new but READTEST\'s DBL.TEXT',
                   {e[3] for e in v.entries} == before | {'DBL.TEXT'}))
    bad = [n for n, good in checks if not good]
    for n, good in checks:
        print('%-5s %s' % ('ok' if good else 'FAIL', n))
    if bad:
        print(re.sub(r'\n+', '\n', tr)[-3000:])
    print('PASCALC: volume test (%s): %s   (work: %s)' % (os.environ.get('PSYS_MODE', 'native'),
                                                         'FAILED' if bad else 'PASSED', d))
    if not bad:
        shutil.rmtree(d, True)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
