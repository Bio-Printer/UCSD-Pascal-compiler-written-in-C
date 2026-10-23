#!/usr/bin/env python3
"""compare.py -- the C compiler (build/pc) against the reference: each program
of the corpus (refcompile.py's CORPUS) compiled on the host, its code file
compared with reference/NAME.CODE.

What a code file holds is compared byte for byte (regions()): the segment
dictionary, each segment's code (CODELENG bytes), its linker records, and a
unit's interface text.  The rest of a block after a segment is whatever was in
the compiler's disk buffer -- in the reference, memory left by the program run
before it -- and so is the comment's text after its end.  Within those, the
bytes the compiler never sets (the byte GENWORD skips to reach a word: what
was in the code buffer, which NEW does not clear; a procedure number of a
segment that has no body for it, from the global array PROCTABLE, never set)
are found by compiling twice,
new memory filled with 0 and then with 0xEE ($PCFILL): the bytes that change
are not compared, and are counted.

  compare.py [-v] [NAME ...]      (default: the whole corpus)

The files of the program's volume are put in a directory, as they are on the
volume (whole blocks), with BIGGY's SYSTEM.LIBRARY, and pc is run there.
-v shows the compiler's console output and the first differences.
"""
import os, shutil, struct, subprocess, sys, tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import refcompile
from refcompile import CORPUS, EXTRA, SMALL, BLK, ucsdvol

PC = os.path.join(ROOT, 'build', 'pc')


LINKREFS = (1, 3, 4, 13, 14)           # MODDULE, PUBBLIC, PRIVVATE, SEPPREF, SEPFREF: references follow


def regions(code):
    """the byte ranges of CODE that its segment dictionary gives a meaning to"""
    w = lambda o: struct.unpack_from('<H', code, o)[0]
    r = [(0, 256), (336, 512)]          # (the comment, 256..335, is compared to its end)
    for i in range(16):
        addr, leng, kind, text = w(4 * i), w(4 * i + 2), w(192 + 2 * i), w(224 + 2 * i)
        if text:
            r.append((text * 512, addr * 512))          # the interface text
        if leng:
            r.append((addr * 512, addr * 512 + leng))
        if kind in (1, 2, 3, 4) and leng:               # linker records after the code
            o = start = (addr + (leng + 511) // 512) * 512
            while o + 16 <= len(code):
                t = w(o + 8)
                o += 16
                if t in LINKREFS:
                    o += 2 * (w(o - 4) + (-w(o - 4)) % 8)
                if t == 0:
                    break
            r.append((start, o))
    return r


def differences(got, ref, unset=()):
    """the offsets where GOT differs from REF in what REF's code file defines,
    but for UNSET"""
    diffs = []
    if len(got) < 512:
        return [0]
    for a, b in regions(ref):
        diffs += [i for i in range(a, b) if i >= len(got) or i >= len(ref) or got[i] != ref[i]]
    end = got.find(0, 256, 336)
    diffs += [i for i in range(256, end if end >= 0 else 336) if got[i] != ref[i]]
    return sorted(set(diffs) - set(unset))


def put(d, name, raw):
    raw = raw + bytes(-len(raw) % 512)
    open(os.path.join(d, name.upper()), 'wb').write(raw)


def compile_one(name, verbose, fill=0):
    vol, src, code = CORPUS[name]
    d = tempfile.mkdtemp(prefix='pcc_')
    if vol:
        v = ucsdvol.Volume(os.path.join(BLK, vol))
        for e in v.entries:
            put(d, e[3], v.read(e[3])[0])
        for n, (ovol, ofile) in EXTRA.get(name, {}).items():
            put(d, n, ucsdvol.Volume(os.path.join(BLK, ovol)).read(ofile)[0])
    else:
        for n, data in SMALL.items():
            put(d, n, data)
    if not os.path.exists(os.path.join(d, 'SYSTEM.LIBRARY')):
        put(d, 'SYSTEM.LIBRARY', ucsdvol.Volume(os.path.join(BLK, 'Big_Disk.BLK')).read('SYSTEM.LIBRARY')[0])
    r = subprocess.run([PC, src + '.TEXT', 'OUT.CODE'], cwd=d, capture_output=True, text=True,
                       encoding='latin1', timeout=600, env=dict(os.environ, PCFILL=str(fill)))
    got = None
    if os.path.exists(os.path.join(d, 'OUT.CODE')):
        got = open(os.path.join(d, 'OUT.CODE'), 'rb').read()
    shutil.rmtree(d, True)
    return r, got


def main(args):
    verbose = '-v' in args
    names = [a for a in args if a != '-v'] or list(CORPUS)
    same = 0
    for n in names:
        ref = open(os.path.join(ROOT, 'reference', n + '.CODE'), 'rb').read()
        r, got = compile_one(n, verbose)
        unset = []
        if got is not None:
            r2, got2 = compile_one(n, verbose, 0xEE)
            if got2 is None or len(got2) != len(got):
                got2 = b''
            inside = set(i for a, b in regions(ref) for i in range(a, b)) | set(range(256, 336))
            unset = [i for i in range(len(got2)) if got[i] != got2[i] and i in inside]
        if got is None:
            print('%-12s no code file (status %d)' % (n, r.returncode))
            if r.stderr:
                print(r.stderr.strip()[-2000:])
        elif not differences(got, ref, unset) and len(got) == len(ref):
            print('%-12s identical (%d bytes; %d never set)' % (n, len(got), len(unset)))
            same += 1
        else:
            diffs = differences(got, ref, unset)
            first = diffs[0] if diffs else -1
            print('%-12s DIFFERENT: %d bytes (reference %d), %d differ, the first at %d (block %d +%d)'
                  % (n, len(got), len(ref), len(diffs), first, first // 512, first % 512))
            if verbose:
                for i in diffs[:16]:
                    print('   %6d: %02x, reference %02x' % (i, got[i], ref[i]))
        if verbose:
            print(r.stdout[-3000:])
        sys.stdout.flush()
    print('%d of %d identical' % (same, len(names)))
    return 0 if same == len(names) else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
