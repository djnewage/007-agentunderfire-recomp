#!/usr/bin/env python3
# map_bond.py: bond_eng.xbe's hand-found addresses in bond_fre.xbe and bond_ger.xbe. The builds are one source with their strings
# changed, so their code differs only by shifts: instructions are aligned on runs that read the same with numbers masked, and
# the numbers in aligned instructions give the data's addresses too. Writes, for each language, config/bond_<l>.toml (config/bond.toml's
# entries moved), src/bond/addr_<l>.h (A()'s table) and src/bond_<l>/*.cpp (src/bond's sources with the functions they override or
# call renamed at once: as macros, one language's new name can be another's old). Run after changing src/bond. Stops on anything unmapped.
import os, re, struct, subprocess, sys, tempfile, collections

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def sections(path):
    d = open(path, 'rb').read()
    u = lambda o: struct.unpack_from('<I', d, o)[0]
    bb = u(0x104); so = u(0x120) - bb
    out = []
    for i in range(u(0x11c)):
        o = so + i * 56
        fl, va, vs, ra, rs, no = struct.unpack_from('<IIIIII', d, o)
        out.append((d[no - bb:no - bb + 12].split(b'\0')[0].decode('latin1'), va, vs, d[ra:ra + rs]))
    return out


def disasm(va, code):
    with tempfile.NamedTemporaryFile(delete=False) as f:
        f.write(code)
    t = subprocess.run(['objdump', '-D', '-b', 'binary', '-mi386', '-Mintel', '--no-show-raw-insn', '--adjust-vma=%d' % va, f.name],
                       capture_output=True, text=True).stdout
    os.unlink(f.name)
    ins = []
    for l in t.splitlines():
        m = re.match(r'\s*([0-9a-f]+):\s+(.*)$', l)
        if m:
            raw = re.sub(r'<[^>]*>', '', m.group(2)).strip()
            nums = [int(x, 16) for x in re.findall(r'0x([0-9a-f]+)', raw)]
            ins.append((int(m.group(1), 16), ' '.join(re.sub(r'0x[0-9a-f]{3,}', 'N', raw).split()), nums))
    return ins


class Pair:
    def __init__(self, a, b, moved=False):
        self.moved = moved  # whole blocks stand elsewhere (the USA disc's Driving.xbe): matches in any order, at any distance
        self.sa, self.sb = sections(a), sections(b)
        self.code = {}  # english instruction address -> the other's
        self.vals = collections.defaultdict(collections.Counter)  # english number -> the other's
        for n in ('.text', 'D3D', 'D3DX', 'XGRPH', 'DSOUND', 'XPP', 'DOLBY'):
            x = next((s for s in self.sa if s[0] == n), None); y = next((s for s in self.sb if s[0] == n), None)
            if x and y:
                self.align(disasm(x[1], x[3]), disasm(y[1], y[3]))

    def align(self, A, B, k=8):
        ga = [t for _, t, _ in A]; gb = [t for _, t, _ in B]
        ca = collections.Counter(tuple(ga[i:i + k]) for i in range(len(ga) - k))
        idx = {}
        for i in range(len(gb) - k):
            idx.setdefault(tuple(gb[i:i + k]), []).append(i)
        i = 0; last = -1
        while i < len(ga) - k:
            key = tuple(ga[i:i + k])
            js = idx.get(key, [])
            j = None
            # the builds differ by small shifts: a far match is a coincidence, and would end the walk there
            if len(js) == 1 and ca[key] == 1 and (self.moved or (js[0] > last and abs(js[0] - i) < 4000)):
                j = js[0]
            if j is None:
                i += 1
                continue
            # walk out both ways while the text agrees
            s = 0
            while i - s - 1 >= 0 and j - s - 1 > (-1 if self.moved else last) and ga[i - s - 1] == gb[j - s - 1]:
                s += 1
            e = 0
            while i + e < len(ga) and j + e < len(gb) and ga[i + e] == gb[j + e]:
                e += 1
            for q in range(-s, e):
                ia, ib = A[i + q], B[j + q]
                self.code[ia[0]] = ib[0]
                if len(ia[2]) == len(ib[2]):
                    for x, y in zip(ia[2], ib[2]):
                        if x >= 0x1000:
                            self.vals[x][y] += 1
            i += e; last = j + e - 1
        self.gaps(A, B)

    def gaps(self, A, B):
        # between aligned runs: the instructions' text matched locally; equal counts of unmatched lines pair up in order
        import difflib
        pos_b = {a: n for n, (a, _, _) in enumerate(B)}
        mapped = [(n, pos_b[self.code[a]]) for n, (a, _, _) in enumerate(A) if a in self.code and self.code[a] in pos_b]
        for (i0, j0), (i1, j1) in zip(mapped, mapped[1:]):
            if i1 - i0 < 2 or j1 - j0 < 2 or i1 - i0 > 4000 or j1 - j0 > 4000:
                continue
            xa, xb = A[i0 + 1:i1], B[j0 + 1:j1]
            m = difflib.SequenceMatcher(None, [t for _, t, _ in xa], [t for _, t, _ in xb], autojunk=False)
            for tag, a0, a1, b0, b1 in m.get_opcodes():
                if tag == 'equal' or (tag == 'replace' and a1 - a0 == b1 - b0):
                    for q in range(a1 - a0):
                        ia, ib = xa[a0 + q], xb[b0 + q]
                        self.code[ia[0]] = ib[0]
                        if tag == 'equal' and len(ia[2]) == len(ib[2]):
                            for x, y in zip(ia[2], ib[2]):
                                if x >= 0x1000:
                                    self.vals[x][y] += 1

    def section_of(self, secs, a):
        for s in secs:
            if s[1] <= a < s[1] + s[2]:
                return s
        return None

    def data(self, a):
        # exactly referenced, else from the nearest reference below in its section: a struct's field
        if a in self.vals:
            return self.vals[a].most_common(1)[0][0]
        sec = self.section_of(self.sa, a)
        if not sec:
            return None
        below = [v for v in self.vals if sec[1] <= v <= a and a - v < 0x400]
        if not below:
            return None
        base = max(below)
        return self.vals[base].most_common(1)[0][0] + (a - base)

    def code_addr(self, a):
        if a in self.code:
            return self.code[a]
        # no aligned run near (a short function among like ones): its first bytes, nearest at about the shift around it
        sa = self.section_of(self.sa, a); sb = next((s for s in self.sb if sa and s[0] == sa[0]), None)
        if sa and sb:
            pat = sa[3][a - sa[1]:a - sa[1] + 12]
            hits = [m.start() + sb[1] for m in re.finditer(re.escape(pat), sb[3])]
            around = [self.code[c] - c for c in range(a - 0x800, a + 0x800) if c in self.code]
            if hits and around:
                want = a + sorted(around)[len(around) // 2]
                best = min(hits, key=lambda h: abs(h - want))
                if abs(best - want) < 0x200:
                    return best
        # just before an aligned run (a function's first lines, where the masked text was not unique enough): the run's shift
        near = [c for c in range(a + 1, a + 0x100) if c in self.code]
        return self.code[near[0]] - (near[0] - a) if near else None

    def any(self, a):
        s = self.section_of(self.sa, a)
        if s and s[0] in ('.text', 'D3D', 'D3DX', 'XGRPH', 'DSOUND', 'XPP', 'DOLBY'):
            if a in self.code:
                return self.code[a]
            if a in self.vals:  # the library's own variables stand in its code's sections
                return self.data(a)
            return self.code_addr(a) or self.data(a)  # by a number nearby only last: that can land inside an instruction
        return self.data(a)


def rewrite(p, src, lang, addr_h, cfg_in, cfg_out, swaps):
    # src's sources at p's addresses in src_<lang>, addr_h A()'s table, cfg_out the config moved; returns the problems
    bad = 0
    srcs = sorted(f for f in os.listdir('src/' + src) if f.endswith('.cpp') and not f.startswith('addr_'))
    text = {f: open('src/%s/%s' % (src, f)).read() for f in srcs}
    funcs = sorted({int(x, 16) for t in text.values() for x in re.findall(r'\bsub_([0-9A-F]{8})\b', t)})
    addrs = sorted({int(x, 16) for t in text.values() for x in re.findall(r'\bA\(0x([0-9A-Fa-f]+)\)', t)})
    print(src, lang, 'aligned', len(p.code), 'instructions')
    names = {}
    for a in funcs:
        b = p.code_addr(a)
        if b is None:
            print('  function %X: unmapped' % a); bad += 1
        else:
            names['%08X' % a] = '%08X' % b
    if len(set(names.values())) != len(names):
        print('  two functions on one address'); bad += 1
    os.makedirs('src/%s_%s' % (src, lang), exist_ok=True)
    for f in srcs:
        body = re.sub(r'sub_([0-9A-F]{8})\b', lambda m: 'sub_' + names.get(m.group(1), m.group(1)), text[f])
        with open('src/%s_%s/%s' % (src, lang, f), 'w') as o:
            o.write('// generated by tools/map_bond.py from src/%s/%s\n#define AUF_LANG_%s\n%s' % (src, f, lang.upper(), body))
    table = []
    for a in addrs:
        b = p.any(a)
        if b is None:
            print('  address %X: unmapped' % a); bad += 1
        else:
            table.append('        case 0x%X: return 0x%X;\n' % (a, b))
    if addr_h:
        with open(addr_h, 'w') as f:
            f.write('// generated by tools/map_bond.py\n' +
                    '#define AUF_ADDR_TABLE \\\n' + ''.join(l.rstrip('\n') + ' \\\n' for l in table) + '\n')
    out = open(cfg_in).read()
    for x, y in swaps:
        out = out.replace(x, y)
    def move(m):
        nonlocal bad
        a = int(m.group(2), 16)
        b = p.any(a)
        if b is None:
            print('  config %s: unmapped' % m.group(0).strip()); bad += 1
            return m.group(0)
        return m.group(1) + '0x%X' % b
    out = re.sub(r'((?:^|\n)[^#\n]*?=\s*)0x([0-9A-Fa-f]{4,8})\b', move, out)
    open(cfg_out, 'w').write('# generated by tools/map_bond.py from %s\n' % os.path.basename(cfg_in) + out)
    return bad


def main():
    # the USA disc is not in assets beside PAL's: map_bond.py usa <its folder>
    os.chdir(ROOT)
    bad = 0
    if sys.argv[1:2] == ['usa']:
        x = sys.argv[2]
        bad += rewrite(Pair('assets/bond_eng.xbe', x + '/bond.xbe'), 'bond', 'usa', 'src/bond/addr_usa.h', 'config/bond.toml', 'config/bond_usa.toml',
                       [('"bond_game.toml"', '"bond_usa_game.toml"'), ('bond_eng.xbe', 'bond.xbe'), ('generated/bond', 'generated/bond_usa')])
        bad += rewrite(Pair('assets/DRIVING.XBE', x + '/Driving.xbe', True), 'driving', 'usa', 'src/driving/addr_usa.h', 'config/driving.toml', 'config/driving_usa.toml',
                       [('"driving_game.toml"', '"driving_usa_game.toml"'), ('DRIVING.XBE', 'Driving.xbe'), ('generated/driving', 'generated/driving_usa'),
                        # not in this build: no caller of SetGammaRamp; the pause's lit box takes its place from the tab's widget, no list of places on the stack
                        ('D3DDevice_SetGammaRamp                               = 0x114980\n', ''), ('[[hook]]\nat = 0xBBFAB\ncall = "TabPlaces"\n', '')])
        bad += rewrite(Pair('assets/default.xbe', x + '/default.xbe', True), 'game', 'usa', None, 'config/auf.toml', 'config/auf_usa.toml',
                       [('"game.toml"', '"game_usa_game.toml"'), ('generated/game', 'generated/game_usa')])
        return 1 if bad else 0
    for lang in sys.argv[1:] or ['fre', 'ger']:
        bad += rewrite(Pair('assets/bond_eng.xbe', 'assets/bond_%s.xbe' % lang), 'bond', lang, 'src/bond/addr_%s.h' % lang, 'config/bond.toml', 'config/bond_%s.toml' % lang,
                       [('includes = ["bond_game.toml"]', 'includes = ["bond_%s_game.toml"]' % lang), ('bond_eng.xbe', 'bond_%s.xbe' % lang), ('generated/bond', 'generated/bond_%s' % lang),
                        ('AgentUnderFireBond.dll', 'AgentUnderFireBond%s.dll' % lang.capitalize())])
    return 1 if bad else 0


sys.exit(main())
