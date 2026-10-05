"""Reproducible synthetic, c17 golden, and selected real-circuit FFR tests."""
import csv
import itertools
import json
from fractions import Fraction
from ffr import HERE, ROOT, Netlist, run, prepare


def synthetic(pairs=12, correlated=False):
    path = HERE/'cases'/f'pairs{pairs}_{"shared" if correlated else "independent"}.v'
    path.parent.mkdir(parents=True, exist_ok=True)
    inputs = ['x'] + (['a'] if correlated else [])
    inputs += [name for i in range(pairs) for name in
               ([f'b{i}'] if correlated else [f'a{i}', f'b{i}'])]
    gates = []
    for i in range(pairs):
        a = 'a' if correlated else f'a{i}'
        gates.append(f'OR2 o{i} (.A({a}), .B(b{i}), .Z(s{i}));')
        previous = 'x' if i == 0 else f't{i-1}'
        gates.append(f'AND2 g{i} (.A({previous}), .B(s{i}), .Z(t{i}));')
    path.write_text('module toy;\ninput '+', '.join(inputs)+';\noutput t'+str(pairs-1)+';\n'
                    +'\n'.join(gates)+'\nendmodule\n')
    return path


def main():
    results = []
    for shared in [False, True]:
        net = synthetic(12, shared)
        expected = Fraction(1, 2)*(Fraction(1, 2)+Fraction(1, 2**13)) if shared else Fraction(1, 2)*Fraction(3, 4)**12
        for flat in [False, True]:
            result = run(net, 'x', 0, HERE/'runs'/f'{net.stem}_{"flat" if flat else "ffr"}', 10, flat=flat)
            assert result['complete'] and Fraction(result['union']['probability']) == expected
            results.append(result)
            print(json.dumps(result), flush=True)

    netpath = ROOT/'input/circuit/c17a.v'
    net = Netlist(netpath)
    with (ROOT/'expected/c17a_result.csv').open() as file:
        golden = {(r['net_name'], int(r['f_type'][-1])): r['fdp'] for r in csv.DictReader(file)}
    regression = []
    for fault in net.pi+list(net.g):
        for stuck in (0, 1):
            result = run(netpath, fault, stuck, HERE/'regression'/f'{fault}_{stuck}', 3)
            assert result['complete']
            p = Fraction(result['union']['probability'])
            expected = Fraction(sum(net.simulate(bits, fault, stuck) for bits in
                                   itertools.product([0, 1], repeat=len(net.pi))), 1 << len(net.pi))
            assert p == expected
            assert f'{float(p):.10e}' == golden[(fault, stuck)]
            regression.append(result)
    (HERE/'regression/summary.json').write_text(json.dumps(regression, indent=2))
    print(json.dumps({'c17_stem_faults': len(regression), 'golden_and_exhaustive_and_raw_cnf': True}), flush=True)
    (HERE/'results').mkdir(exist_ok=True)
    (HERE/'results/synthetic.json').write_text(json.dumps(results, indent=2))


if __name__ == '__main__':
    main()
