#!/usr/bin/env python3
"""被覆生成手法の事前検証用プローブ（本体とは独立した使い捨ての参照実装）。

SAFのステム故障1件について、検出関数 D の完全被覆を得るまでの「生成回数」を方式別に数える。
  plain   : 純粋な素項キューブ（検出側のみ）
  dual    : 検出側・非検出側の純粋な素項キューブを交互に生成（どちらかが尽きれば完了）
  ordered : 順序付き被覆。純粋な素項を作った後、先行キューブを前提にさらにリテラルを削る
  affine  : 擬似キューブ。素項の固定条件どうしをパリティ制約へ併合する
  window  : 窓関係。窓内は全数シミュレーション、窓外の自由度はSATの反例で絞る（CEGAR）
FDPは計算しない。完了した被覆の健全性は乱数シミュレーションで点検するだけで、証明ではない。
依存: python-sat（pip install python-sat）。
"""
import re, sys, time, random
from pysat.solvers import Cadical153

def parse(path):
    txt = re.sub(r'//.*', '', open(path).read())
    ins, outs, gates = [], [], []
    for stmt in txt.split(';'):
        s = ' '.join(stmt.split())
        if not s: continue
        m = re.match(r'(input|output|wire)\s+(.*)', s)
        if m:
            names = [x.strip() for x in m.group(2).split(',') if x.strip()]
            if m.group(1) == 'input': ins += names
            elif m.group(1) == 'output': outs += names
            continue
        m = re.match(r'([A-Za-z]+?)(\d*)\s+\S+\s*\((.*)\)\s*$', s)
        if not m or m.group(1) in ('module', 'endmodule'): continue
        pins = dict(re.findall(r'\.(\w+)\s*\(\s*([^\s)]+)\s*\)', m.group(3)))
        out = pins.pop('Z')
        gates.append((m.group(1), out, [pins[k] for k in sorted(pins)]))
    return ins, outs, gates

class Ckt:
    def __init__(s, path):
        s.ins, s.outs, gl = parse(path)
        s.gate = {o: (t, i) for t, o, i in gl}
        # topological order
        order, seen = [], set(s.ins)
        def visit(n):
            st = [(n, 0)]
            while st:
                x, k = st.pop()
                if x in seen: continue
                fi = s.gate[x][1]
                if k < len(fi):
                    st.append((x, k + 1))
                    if fi[k] not in seen: st.append((fi[k], 0))
                else:
                    seen.add(x); order.append(x)
        for o in s.gate: visit(o)
        s.order = order
        s.fo = {}
        for o, (t, fi) in s.gate.items():
            for i in fi: s.fo.setdefault(i, []).append(o)
    def tfo(s, n):
        r, st = set(), [n]
        while st:
            x = st.pop()
            if x in r: continue
            r.add(x); st += s.fo.get(x, [])
        return r
    def tfi(s, roots):
        r, st = set(), list(roots)
        while st:
            x = st.pop()
            if x in r: continue
            r.add(x)
            if x in s.gate: st += s.gate[x][1]
        return r

def gate_cnf(t, z, xs):
    inv = t in ('NAND', 'NOR', 'XNOR', 'INV')
    if t in ('BUF', 'INV'):
        a = xs[0]; zz = -z if inv else z
        return [[-a, zz], [a, -zz]]
    zz = -z if inv else z
    if t in ('AND', 'NAND'):
        return [[-zz, x] for x in xs] + [[zz] + [-x for x in xs]]
    if t in ('OR', 'NOR'):
        return [[zz, -x] for x in xs] + [[-zz] + xs]
    if t in ('XOR', 'XNOR'):
        a, b = xs
        return [[-a, -b, -zz], [a, b, -zz], [a, -b, zz], [-a, b, zz]]
    raise ValueError(t)

def ev(t, vals, mask):
    if t == 'BUF': return vals[0]
    if t == 'INV': return ~vals[0] & mask
    if t in ('AND', 'NAND'):
        r = mask
        for v in vals: r &= v
    elif t in ('OR', 'NOR'):
        r = 0
        for v in vals: r |= v
    else:
        r = vals[0] ^ vals[1]
    return (~r & mask) if t in ('NAND', 'NOR', 'XNOR') else r

class Fault:
    def __init__(s, c, net, stuck):
        s.c, s.net, s.stuck = c, net, stuck
        s.cone = c.tfo(net)
        s.pos = [o for o in c.outs if o in s.cone]
        s.scope = c.tfi(s.pos)
        s.pis = [p for p in c.ins if p in s.scope]
        s.sorder = [n for n in c.order if n in s.scope]
        nv = [0]
        def new():
            nv[0] += 1; return nv[0]
        s.g = {n: new() for n in s.pis}
        for n in s.sorder: s.g[n] = new()
        s.f = dict(s.g)
        for n in s.cone:
            if n in s.scope: s.f[n] = new()
        cl = []
        for n in s.sorder:
            t, fi = c.gate[n]
            cl += gate_cnf(t, s.g[n], [s.g[i] for i in fi])
            if n in s.cone and n != net:
                cl += gate_cnf(t, s.f[n], [s.f[i] for i in fi])
        cl.append([s.f[net]] if stuck else [-s.f[net]])
        diffs = []
        for o in s.pos:
            x = new(); a, b = s.g[o], s.f[o]
            cl += [[-a, -b, -x], [a, b, -x], [a, -b, x], [-a, b, x]]
            diffs.append(x)
        s.d = new()
        cl += [[-x, s.d] for x in diffs] + [[-s.d] + diffs]
        s.clauses, s.nvars = cl, nv[0]
        s.pivar = [s.g[p] for p in s.pis]
    def solver(s, detect):
        sv = Cadical153(bootstrap_with=s.clauses)
        sv.add_clause([s.d] if detect else [-s.d])
        return sv
    def sim(s, pat, nbits):
        """pat: dict pi->int bit-vector. returns detect bit-vector."""
        mask = (1 << nbits) - 1
        g = dict(pat); f = {}
        for n in s.sorder:
            t, fi = s.c.gate[n]
            g[n] = ev(t, [g[i] for i in fi], mask)
        f[s.net] = mask if s.stuck else 0
        det = 0
        for n in s.sorder:
            if n in s.cone and n != s.net:
                t, fi = s.c.gate[n]
                f[n] = ev(t, [f.get(i, g[i]) for i in fi], mask)
        for o in s.pos: det |= g[o] ^ f.get(o, g[o])
        return det

def shrink(oracle, lits, minimize):
    """lits: assumptions under which oracle is UNSAT. returns a subset core."""
    assert not oracle.solve(assumptions=lits), 'seed not separated by oracle'
    core = oracle.get_core() or []
    calls = 1
    if minimize:
        core = list(core); i = 0
        while i < len(core):
            trial = core[:i] + core[i + 1:]
            calls += 1
            if not oracle.solve(assumptions=trial):
                c2 = set(oracle.get_core() or [])
                core = [l for l in trial if l in c2]
            else:
                i += 1
    return core, calls

def run_basic(fa, method, limit, tlimit, minimize=True, ratio=1):
    """returns dict(complete, rules, n1, n0, time, calls, rules list)."""
    t0 = time.time()
    P = fa.solver(True); N = fa.solver(False)
    if method == 'dl':
        Po, No = P, N                      # oracles see every blocking clause
    else:
        Po, No = fa.solver(True), fa.solver(False)   # clean oracles
    rules, calls, turn = [], 0, 0
    piset = fa.pivar
    done = None
    while True:
        if len(rules) >= limit or time.time() - t0 > tlimit: break
        side = 1
        if method != 'plain' and turn % (ratio + 1) == ratio: side = 0
        turn += 1
        gen, oracle = (P, No) if side else (N, Po)
        calls += 1
        if not gen.solve():
            done = 'P' if side else 'N'; break
        model = gen.get_model()
        seed = [model[v - 1] for v in piset]
        core, c = shrink(oracle, seed, minimize); calls += c
        rules.append((side, core))
        block = [-l for l in core]
        if method == 'dl':
            P.add_clause(block); N.add_clause(block)
        elif side: P.add_clause(block)
        else: N.add_clause(block)
    for s_ in {id(x): x for x in (P, N, Po, No)}.values(): s_.delete()
    return dict(complete=done is not None, end=done, rules=rules, n=len(rules),
                n1=sum(1 for s_, _ in rules if s_), n0=sum(1 for s_, _ in rules if not s_),
                time=time.time() - t0, calls=calls)

def check(fa, res, method, nbits=4096, seed=1):
    """random-simulation consistency check of a completed result."""
    rnd = random.Random(seed); mask = (1 << nbits) - 1
    pat = {p: rnd.getrandbits(nbits) for p in fa.pis}
    det = fa.sim(pat, nbits)
    v2p = {fa.g[p]: p for p in fa.pis}
    def cube(core):
        r = mask
        for l in core:
            b = pat[v2p[abs(l)]]
            r &= b if l > 0 else (~b & mask)
        return r
    if method == 'dl':
        und, pred = mask, 0
        for side, core in res['rules']:
            m = cube(core) & und
            if side: pred |= m
            und &= ~m
        if res['end'] == 'N': pred |= und
    else:
        u = v = 0
        for side, core in res['rules']:
            if side: u |= cube(core)
            else: v |= cube(core)
        pred = u if res['end'] == 'P' else (~v & mask)
    return pred == det

def tt_var(i, w):
    """truth-table column of variable i among w vars (2^w bits)."""
    n = 1 << w; blk = 1 << i
    pat = ((1 << blk) - 1) << blk
    r = 0
    for k in range(0, n, 2 * blk): r |= pat << k
    return r

def isop(L, U, w, v=None):
    """Minato-Morreale ISOP on full-width truth tables (ints of 2^w bits).
    returns (list of cubes {var: bit}, cover table)."""
    FULL = (1 << (1 << w)) - 1
    if v is None: v = w
    if L == 0: return [], 0
    if U == FULL: return [dict()], FULL
    x = v - 1; col = tt_var(x, w); blk = 1 << x; ncol = ~col & FULL
    def c1(T): t = T & col; return t | (t >> blk)
    def c0(T): t = T & ncol; return t | (t << blk)
    L0, L1, U0, U1 = c0(L), c1(L), c0(U), c1(U)
    A0, f0 = isop(L0 & ~U1 & FULL, U0, w, x)
    A1, f1 = isop(L1 & ~U0 & FULL, U1, w, x)
    Ls = (L0 & ~f0 & FULL) | (L1 & ~f1 & FULL)
    As, fs = isop(Ls, U0 & U1, w, x)
    cubes = [{**c, x: 0} for c in A0] + [{**c, x: 1} for c in A1] + As
    return cubes, (f0 & ncol) | (f1 & col) | fs

def cof_table(fa, W, fixed, ww):
    nb = 1 << ww; M = (1 << nb) - 1
    pat = {}
    for p in fa.pis:
        if p in W: pat[p] = tt_var(W[p], ww)
        else: pat[p] = M if fixed[p] else 0
    return fa.sim(pat, nb)

def run_window(fa, limit, tlimit, w=10, policy='freq', seed=0):
    t0 = time.time(); rnd = random.Random(seed)
    P = fa.solver(True); No = fa.solver(False)
    v2p = {fa.g[p]: p for p in fa.pis}
    freq = {p: 0 for p in fa.pis}
    nv = fa.nvars
    seeds = cubes_out = cegar = 0; gains = []; done = False; orc = 0
    while seeds < limit and time.time() - t0 < tlimit:
        if not P.solve(): done = True; break
        model = P.get_model(); seeds += 1
        seedl = [model[v - 1] for v in fa.pivar]
        mval = {v2p[abs(l)]: (1 if l > 0 else 0) for l in seedl}
        core, c = shrink(No, seedl, True); orc += c
        K = {v2p[abs(l)]: (1 if l > 0 else 0) for l in core}
        for p in K: freq[p] += 1
        if policy == 'freq':
            cand = sorted(K, key=lambda p: -freq[p])
            extra = sorted((p for p in fa.pis if p not in K), key=lambda p: -freq[p])
        else:
            cand = list(K); rnd.shuffle(cand)
            extra = [p for p in fa.pis if p not in K]; rnd.shuffle(extra)
        Wl = cand[:w]
        if len(Wl) < w: Wl += extra[:w - len(Wl)]
        ww = len(Wl); W = {p: i for i, p in enumerate(Wl)}
        nb = 1 << ww; M = (1 << nb) - 1
        T = cof_table(fa, W, mval, ww)
        out_lits = [(fa.g[p] if K[p] else -fa.g[p]) for p in K if p not in W]
        nv += 1; act = nv
        added = 0   # complement cubes already encoded
        prevN = 0
        while True:
            # encode x_W in T  (clauses forbidding each cube of ISOP(~T)), only the newly forbidden part
            newN = (~T & M) & ~prevN
            if newN:
                cl, _ = isop(newN, ~T & M, ww)
                for cb in cl:
                    No.add_clause([-act] + [(-fa.g[Wl[i]] if b else fa.g[Wl[i]]) for i, b in cb.items()])
                prevN = ~T & M
            orc += 1
            if not No.solve(assumptions=out_lits + [act]): break
            cegar += 1
            mm = No.get_model()
            fixed = {p: (1 if mm[fa.g[p] - 1] > 0 else 0) for p in fa.pis}
            T2 = cof_table(fa, W, fixed, ww)
            assert T & ~T2, 'counterexample did not refine'
            T &= T2
        No.add_clause([-act])
        base = M
        for p, i in W.items():
            if p in K:
                col = tt_var(i, ww); base &= col if K[p] else (~col & M)
        assert T & base == base, 'prime cube lost'
        gains.append(bin(T).count('1') / bin(base).count('1'))
        cl, cov = isop(T, T, ww)
        neg_out = [-l for l in out_lits]
        for cb in cl:
            P.add_clause(neg_out + [(-fa.g[Wl[i]] if b else fa.g[Wl[i]]) for i, b in cb.items()])
        cubes_out += len(cl)
    P.delete(); No.delete()
    gains.sort()
    return dict(complete=done, seeds=seeds, cubes=cubes_out, time=time.time() - t0,
                med=gains[len(gains) // 2] if gains else 0, mx=max(gains) if gains else 0,
                cegar=cegar / max(1, seeds), orc=orc)

def run_ordered(fa, limit, tlimit, ratio=1):
    t0 = time.time()
    P = fa.solver(True); N = fa.solver(False); Pc = fa.solver(True); Nc = fa.solver(False)
    rules, turn, done, ext = [], 0, None, 0
    while len(rules) < limit and time.time() - t0 < tlimit:
        side = 0 if turn % (ratio + 1) == ratio else 1
        turn += 1
        gen, clean, rel = (P, Nc, N) if side else (N, Pc, P)
        if not gen.solve(): done = 'P' if side else 'N'; break
        model = gen.get_model()
        seed = [model[v - 1] for v in fa.pivar]
        core, _ = shrink(clean, seed, True)          # pure prime
        core = list(core); i = 0; n0 = len(core)
        while i < len(core):                          # relative extension
            trial = core[:i] + core[i + 1:]
            if not rel.solve(assumptions=trial):
                c2 = set(rel.get_core() or []); core = [l for l in trial if l in c2]
            else: i += 1
        ext += n0 - len(core)
        rules.append((side, core))
        P.add_clause([-l for l in core]); N.add_clause([-l for l in core])
    for s in (P, N, Pc, Nc): s.delete()
    return dict(complete=done is not None, end=done, rules=rules, n=len(rules), ext=ext, time=time.time() - t0)

class Xor:
    """parity variables shared by two solvers"""
    def __init__(s, fa, solvers):
        s.fa, s.solvers, s.nv, s.memo = fa, solvers, fa.nvars, {}
    def var(s, S):
        S = tuple(sorted(S))
        if len(S) == 1: return S[0]
        if S in s.memo: return s.memo[S]
        a = s.var(S[:-1]); b = S[-1]
        s.nv += 1; z = s.nv
        for sv in s.solvers:
            for cl in ([-a, -b, -z], [a, b, -z], [a, -b, z], [-a, b, z]): sv.add_clause(cl)
        s.memo[S] = z
        return z
    def lit(s, S, par):
        v = s.var(S); return v if par else -v

def run_affine(fa, limit, tlimit):
    t0 = time.time()
    P = fa.solver(True); No = fa.solver(False)
    X = Xor(fa, [P, No])
    v2p = {fa.g[p]: p for p in fa.pis}
    regions, merges, done = 0, 0, False
    logvol = []
    while regions < limit and time.time() - t0 < tlimit:
        if not P.solve(): done = True; break
        model = P.get_model()
        seedl = [model[v - 1] for v in fa.pivar]
        mval = {v2p[abs(l)]: (1 if l > 0 else 0) for l in seedl}
        core, _ = shrink(No, seedl, True)
        cons = [(frozenset([abs(l)]), 1 if l > 0 else 0) for l in core]
        changed = True
        while changed and len(cons) > 1:
            changed = False
            k = len(cons)
            pairs = [(i, j) for i in range(k) for j in range(i + 1, k)]
            # simulation filter: flip one variable of cons[i] and one of cons[j] from the seed
            nb = len(pairs); M = (1 << nb) - 1
            pat = {p: (M if mval[p] else 0) for p in fa.pis}
            for b, (i, j) in enumerate(pairs):
                for c in (i, j):
                    p = v2p[min(cons[c][0])]
                    pat[p] ^= 1 << b
            det = fa.sim(pat, nb)
            for b, (i, j) in enumerate(pairs):
                if not (det >> b) & 1: continue
                if cons[i] is None or cons[j] is None: continue
                S = cons[i][0] ^ cons[j][0]
                if not S: continue
                new = (S, cons[i][1] ^ cons[j][1])
                trial = [c for t, c in enumerate(cons) if t not in (i, j) and c is not None] + [new]
                if not No.solve(assumptions=[X.lit(c[0], c[1]) for c in trial]):
                    cons[i] = None; cons[j] = new; merges += 1; changed = True
            cons = [c for c in cons if c is not None]
        P.add_clause([-X.lit(c[0], c[1]) for c in cons])
        regions += 1; logvol.append(len(core) - len(cons))
    P.delete(); No.delete()
    logvol.sort()
    return dict(complete=done, n=regions, merges=merges, time=time.time() - t0,
                med=logvol[len(logvol) // 2] if logvol else 0, mx=max(logvol) if logvol else 0)

if __name__ == '__main__':
    import argparse
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('netlist'); ap.add_argument('faults', nargs='+', help='net/sa0 or net/sa1 (stem faults)')
    ap.add_argument('--limit', type=int, default=3000, help='max generations per method')
    ap.add_argument('--seconds', type=float, default=30.0, help='time limit per method')
    ap.add_argument('--window', type=int, default=10, help='window width (PIs)')
    ap.add_argument('--methods', default='plain,dual,ordered,affine,window')
    a = ap.parse_args()
    c = Ckt(a.netlist)
    for spec in a.faults:
        net, st = spec.split('/'); fa = Fault(c, net, int(st[-1]))
        out = [f'{spec} pi={len(fa.pis)}']
        for m in a.methods.split(','):
            if m in ('plain', 'dual'):
                r = run_basic(fa, m, a.limit, a.seconds); ok = check(fa, r, m) if r['complete'] else None
                extra = f"ok={ok}"
            elif m == 'ordered':
                r = run_ordered(fa, a.limit, a.seconds); ok = check(fa, r, 'dl') if r['complete'] else None
                extra = f"ok={ok} relative_deletions={r['ext']}"
            elif m == 'affine':
                r = run_affine(fa, a.limit, a.seconds)
                extra = f"merges/region med={r['med']} max={r['mx']}"
            elif m == 'window':
                r = run_window(fa, a.limit, a.seconds, a.window); r['n'] = r['seeds']
                extra = f"cubes={r['cubes']} gain med={r['med']:.0f} max={r['mx']:.0f} cegar/seed={r['cegar']:.1f}"
            else: raise SystemExit(f'unknown method {m}')
            out.append(f"{m}: {'complete' if r['complete'] else 'incomplete'} n={r['n']} {r['time']:.1f}s {extra}")
        print(' | '.join(out), flush=True)
