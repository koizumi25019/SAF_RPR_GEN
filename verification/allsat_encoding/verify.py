"""Independent checks of projection polarity, exact covers and c17 golden FDP."""
import csv
import itertools
import json
import random
from fractions import Fraction
from types import SimpleNamespace

from run import HERE, ROOT, Encoded, FastNet, certificate, prepare, probe, run, write_cnf
from expand_connections import expand


def projection():
    rng = random.Random(517)
    results = []
    for trial in range(12):
        ex = probe.Expr(6)
        pool = list(range(12))  # Includes constants and both polarities.
        for _ in range(16):
            args = rng.choices(pool, k=rng.randrange(2, 5))
            pool.append((ex.and_(*args) if rng.randrange(2) else ex.xor(*args)) ^ rng.randrange(2))
        out = pool[-1] if trial > 1 else trial
        clauses, nv, ol = Encoded(ex, 'nnf_pg').cnf(out)
        path = HERE/'runs/verify'/f'projection{trial}.cnf'
        path.parent.mkdir(parents=True, exist_ok=True)
        write_cnf(path, clauses, nv)
        sat = probe.SAT(path, 0)
        try:
            for bits in itertools.product([0, 1], repeat=6):
                value = ex.evaluate(out, bits)
                assumptions = [i+1 if b else -i-1 for i, b in enumerate(bits)]
                rc, _ = sat.solve(assumptions+[ol if value else -ol])
                assert rc == 10, (trial, bits, 'missing')
                rc, _ = sat.solve(assumptions+[-ol if value else ol])
                assert rc == 20, (trial, bits, 'spurious')
        finally:
            sat.close()
        results.append({'trial':trial, 'assignments':64, 'both_polarities_verified':True})
    return results


def c17():
    path = expand(ROOT/'input/circuit/c17a.v', HERE/'runs/c17/expanded.v')
    net = FastNet(path)
    results = []
    for row in csv.DictReader((ROOT/'expected/c17a_result.csv').open()):
        fault, stuck = row['net_name'], int(row['f_type'][-1])
        prepared = prepare(net, fault, stuck)
        for encoding in ['tseitin', 'nnf_pg']:
            for method, seed, width in [('greedy',False,1), ('greedy',True,1),
                                        ('witness',True,1), ('greedy',False,3)]:
                prefix = HERE/'runs/c17'/f'{fault}_{stuck}_{encoding}_{method}_{seed}_w{width}'
                r = run(net, fault, stuck, prefix, prepared, encoding, method, seed, width)
                assert r['complete']
                assert f"{float(Fraction(r['union']['probability'])):.10e}" == row['fdp']
                proof = certificate(path, prefix)
                assert proof['sound'] and proof['exact']
                results.append(dict(fault=fault, stuck=stuck, encoding=encoding,
                                    method=method, seed=seed, width=width, **{'proof':proof}))
    return results


def families():
    # Exact exhaustive bitsets use the original PI coordinates; no circuit BDD.
    results = []
    for kind in ['or_pairs', 'and_pairs']:
        n = 16
        ex = probe.Expr(n)
        pairs = [(2*i+2, 2*i+4) for i in range(0, n, 2)]
        out = ex.or_(*(ex.and_(a,b) for a,b in pairs)) if kind=='or_pairs' else ex.and_(*(ex.or_(a,b) for a,b in pairs))
        expected = sum(1 << bits for bits in range(1 << n)
                       if (any((bits>>i&3)==3 for i in range(0,n,2)) if kind=='or_pairs'
                           else all((bits>>i&3)!=0 for i in range(0,n,2))))
        full = (1 << (1 << n))-1
        literal = {i+1:sum(1 << bits for bits in range(1 << n) if bits>>i&1) for i in range(n)}
        literal.update({-i:full ^ b for i,b in list(literal.items())})
        dummy = SimpleNamespace(pi=[f'x{i}' for i in range(n)])
        prepared = ex, out, [1<<i for i in range(n)], [], 0, set()
        for encoding in ['tseitin', 'nnf_pg']:
            for method, seed, width in [('greedy',False,1), ('greedy',True,1),
                                       ('witness',True,1), ('greedy',False,2)]:
                prefix = HERE/'runs/families'/f'{kind}_{encoding}_{method}_{seed}_w{width}'
                r = run(dummy, 'z', 0, prefix, prepared, encoding, method, seed, width, seconds=3)
                covered = 0
                if width==1:
                    cubes = json.loads(prefix.with_suffix('.cubes.json').read_text())
                    for c in cubes:
                        bits = full
                        for l in c:
                            bits &= literal[l]
                        covered |= bits
                else:
                    data = json.loads(prefix.with_suffix('.regions.json').read_text())
                    for region in data['regions']:
                        bits = full
                        for group, mask in zip(data['groups'],region):
                            states = 0
                            for s in range(1 << len(group)):
                                if mask >> s & 1:
                                    state = full
                                    for j,v in enumerate(group):
                                        state &= literal[v if s>>j&1 else -v]
                                    states |= state
                            bits &= states
                        covered |= bits
                assert covered & ~expected == 0
                if r['complete']:
                    assert covered == expected
                    assert Fraction(r['union']['probability'])==Fraction(expected.bit_count(), 1<<n)
                r.update(family=kind, exhaustive_sound=True, exhaustive_exact=covered==expected,
                         expected_probability=str(Fraction(expected.bit_count(),1<<n)))
                results.append(r)
                print(json.dumps({k:r[k] for k in ['family','encoding','method','seed','width','cubes','complete','seconds']}), flush=True)
    return results


if __name__ == '__main__':
    (HERE/'results').mkdir(exist_ok=True)
    for name, function in [('projection',projection), ('c17',c17), ('families',families)]:
        result = function()
        (HERE/'results'/f'{name}_verification.json').write_text(json.dumps(result, indent=2))
        print(json.dumps({'check':name, 'cases':len(result), 'verified':True}), flush=True)
