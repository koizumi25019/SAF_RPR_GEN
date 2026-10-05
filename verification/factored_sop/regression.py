"""Exhaustive c17 regression for compact coordinates and factored covers."""
import csv
import itertools
import json
import random
from fractions import Fraction
from factor import Engine, FastNet
from fast_basis import HERE, build_fast
from verify import verify
from expand_connections import expand


def sweep_regression():
    from fast_basis import probe, compact
    from sweep import rewrite
    output = HERE/'runs/sweep_regression'
    output.mkdir(exist_ok=True)
    checks = []
    for seed in range(12):
        rng = random.Random(seed)
        ex = probe.Expr(6)
        pool = [2*i for i in range(1, 7)]
        for _ in range(100):
            args = [rng.choice(pool) ^ rng.randrange(2) for _ in range(rng.randrange(2, 5))]
            pool.append(ex.and_(*args) if rng.randrange(2) else ex.xor(*args))
        ex, out, _ = compact(ex, pool[-1])
        new, newout, kept, stats = rewrite(ex, out, output/f'random{seed}', seconds=.2)
        for values in itertools.product((0, 1), repeat=ex.n):
            assert ex.evaluate(out, values) == new.evaluate(newout, [values[i-1] for i in kept])
        checks.append({'seed': seed, 'variables': ex.n, 'proven': len(stats['proven'])})
    ex = probe.Expr(2)
    a, b = 2, 4
    out = ex.and_(ex.or_(a, b), ex.or_(a, b ^ 1))
    for phase in (0, 1):
        new, newout, kept, _ = rewrite(ex, out ^ phase, output/f'absorption{phase}', seconds=.2)
        assert kept == [1] and new.n == 1
        assert [new.evaluate(newout, [v]) for v in (0, 1)] == [phase, 1 ^ phase]
    ex = probe.Expr(16)
    out = ex.and_(*(2*i for i in range(1, 17)))
    new, newout, _, _ = rewrite(ex, out, output/'rare', seconds=.2)
    assert new.n == 16 and new.evaluate(newout, [1]*16) == 1
    result = {'exhaustive_random_expressions': len(checks), 'absorption_and_phase_verified': True,
              'rare_function_not_merged_to_false': True, 'details': checks}
    (output/'summary.json').write_text(json.dumps(result, indent=2))
    return result


def main():
    out = HERE/'runs/c17'
    out.mkdir(parents=True, exist_ok=True)
    netpath = expand(HERE.parents[1]/'input/circuit/c17a.v', out/'expanded.v')
    net = FastNet(netpath)
    with open(HERE.parents[1]/'expected/c17a_result.csv') as fp:
        gold = list(csv.DictReader(fp))
    faults = sorted({(r['net_name'], int(r['f_type'][-1])) for r in gold if r['complete'].strip()})
    expected = {(r['net_name'], int(r['f_type'][-1])): Fraction(r['fdp']) for r in gold}
    checks = []
    forms = [('miter', False, False, False), ('difference', True, False, False),
             ('complement', True, True, False), ('lean', True, True, True)]
    for name, difference, complement, lean in forms:
        engine = Engine(out/name, width=3, difference=difference, complement=complement, lean=lean)
        for i, (fault, stuck) in enumerate(faults):
            ex, output, rows, *_ = build_fast(net, fault, stuck, difference=difference)
            for values in itertools.product((0, 1), repeat=len(net.pi)):
                x = sum(value << j for j, value in enumerate(values))
                y = [(x & row).bit_count() & 1 for row in rows]
                assert ex.evaluate(output, y) == net.simulate(values, fault, stuck)
            result = engine.fault(net, fault, stuck, i)
            assert result['complete'] and Fraction(result['fdp']) == expected[(fault, stuck)]
            checks.append(verify(netpath, result['manifest']))
    result = {'faults': len(faults), 'forms': [r[0] for r in forms], 'verified_runs': len(checks),
              'assignments_per_fault': 32, 'golden_match': True, 'all_sound_exact': True}
    (out/'summary.json').write_text(json.dumps(result, indent=2))
    sweep_regression()
    print(json.dumps(result))


if __name__ == '__main__':
    main()
