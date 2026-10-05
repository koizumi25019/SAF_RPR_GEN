"""Sparse basis completion and bounded preprocessing for large circuits."""
import collections
import functools
import pathlib
import sys

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent/'linear_scaling'))
from experiment import CachedNet, probe
# Imported legacy helpers prepend their own directories. Restore this package's
# precedence so batch.py and verify.py cannot resolve to older experiments.
sys.path.insert(0, str(HERE))


def bits(mask):
    while mask:
        low = mask & -mask
        yield low.bit_length()-1
        mask ^= low


class FastNet(CachedNet):
    def __init__(self, path, tt_limit=6, anf_limit=128):
        self.small_support = functools.cache(self._small_support)
        super().__init__(path, tt_limit, anf_limit)
        # Avoid retaining one potentially large transitive cone per fault.
        self.descendants = functools.lru_cache(maxsize=64)(self._descendants)

    def _small_support(self, k):
        if k in self.pidx:
            return frozenset([k])
        acc = set()
        for child in self.g[k][1]:
            support = self.small_support(child)
            if support is None:
                return None
            acc.update(support)
            if len(acc) > self.tt_limit:
                return None
        return frozenset(acc)

    def _affine_full(self, k):
        poly = self.poly(k)
        if poly is not None:
            if all(m.bit_count() <= 1 for m in poly):
                return functools.reduce(int.__or__, poly, 0), int(0 in poly)
            # An exact ANF containing a degree >= 2 monomial is not affine.
            return None
        support = self.small_support(k)
        if support is None:
            return None
        ss = sorted(support)
        vals = {n: sum(((a >> i) & 1) << a for a in range(1 << len(ss))) for i, n in enumerate(ss)}
        mask = (1 << (1 << len(ss)))-1
        def ev(n):
            if n not in vals:
                kind, inputs = self.g[n]
                vals[n] = probe.eval_gate(kind, [ev(i) for i in inputs], mask)
            return vals[n]
        tt = ev(k)
        const, amask = tt & 1, 0
        for i, n in enumerate(ss):
            if ((tt >> (1 << i)) & 1) ^ const:
                amask |= 1 << self.pidx[n]
        predicted = mask if const else 0
        for n in ss:
            if amask >> self.pidx[n] & 1:
                predicted ^= vals[n]
        return (amask, const) if predicted == tt else None


@functools.lru_cache(maxsize=8)
def complete_basis(n, candidates):
    rows, echelon = [], {}
    def insert(row):
        v, combo = row, 1 << len(rows)
        while v:
            pivot = v.bit_length()-1
            prior = echelon.get(pivot)
            if prior is None:
                echelon[pivot] = (v, combo)
                rows.append(row)
                return True
            v ^= prior[0]
            combo ^= prior[1]
        return False
    for row in candidates:
        insert(row)
    npar = len(rows)
    for i in range(n):
        insert(1 << i)
    inverse = []
    for i in range(n):
        v, combo = 1 << i, 0
        while v:
            row, transform = echelon[v.bit_length()-1]
            v ^= row
            combo ^= transform
        inverse.append(combo)
    assert len(rows) == n
    return rows, inverse, npar


def build_fast(net, fault, stuck, variant='long', difference=False, observability=False):
    if fault not in net.pidx and fault not in net.g:
        raise ValueError(fault)
    downstream = net.descendants(fault)
    ends = [o for o in net.po if o in downstream]
    relevant, stack = set(), ends[:]
    while stack:
        k = stack.pop()
        if k in relevant:
            continue
        relevant.add(k)
        if k in net.g:
            stack.extend(net.g[k][1])
    counts = collections.Counter()
    for k in sorted(relevant):
        ac = net.affine(k)
        if ac and ac[0].bit_count() > 1:
            counts[ac[0]] += 1
    if variant == 'binary':
        candidates = []
    elif variant == 'long':
        candidates = [r for r, _ in sorted(counts.items(), key=lambda x: (-x[0].bit_count(), -x[1], x[0]))]
    else:
        raise ValueError('Supported variants: binary, long')
    rows, inverse, npar = complete_basis(len(net.pi), tuple(candidates))
    ex = probe.Expr(len(net.pi))
    @functools.cache
    def affine(mask, const):
        tr = 0
        for i in bits(mask):
            tr ^= inverse[i]
        return ex.xor(*(2*(j+1) for j in bits(tr)), const)
    @functools.cache
    def rec(k, forced):
        bad = forced is not None
        if bad and k == fault:
            return forced
        if k in net.pidx:
            return affine(1 << net.pidx[k], 0)
        if bad and k not in downstream:
            return rec(k, None)
        ac = net.affine(k)
        if ac is not None and (not bad or fault in net.pidx):
            mask, const = ac
            if bad and fault in net.pidx and mask >> net.pidx[fault] & 1:
                mask ^= 1 << net.pidx[fault]
                const ^= forced
            return affine(mask, const)
        kind, inputs = net.g[k]
        return ex.gate(kind, [rec(i, forced) for i in inputs])
    if difference or observability:
        activation = ex.xor(rec(fault, None), stuck)
        observable = ex.or_(*(ex.xor(rec(o, 0), rec(o, 1)) for o in ends))
        out = observable if observability else ex.and_(activation, observable)
    else:
        out = ex.or_(*(ex.xor(rec(o, None), rec(o, stuck)) for o in ends))
    return ex, out, rows, inverse, npar, relevant


def compact(ex, out):
    active = sorted(k for k in ex.reachable(out) if k <= ex.n)
    dst = probe.Expr(len(active))
    memo = {0: 0, **{old: 2*(i+1) for i, old in enumerate(active)}}
    def visit(lit):
        k = lit // 2
        if k not in memo:
            kind, children = ex.nodes[k]
            args = [visit(a) for a in children]
            memo[k] = dst.and_(*args) if kind == 'a' else dst.xor(*args)
        return memo[k] ^ (lit & 1)
    return dst, visit(out), active
