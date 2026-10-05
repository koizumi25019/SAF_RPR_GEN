"""Compare ordinary and care-conditioned FFR observation covers."""
import argparse
import csv
import itertools
import json
import random
import statistics
from fractions import Fraction
from pathlib import Path

from ffr import HERE, ROOT, Netlist, prepare, run


def run_pair(netpath, fault, stuck, seconds, iteration=0, reverse=False,
             directory='care_runs'):
    rows = []
    modes = [('care', True), ('ordinary', False)]
    if reverse:
        modes.reverse()
    for label, conditioned in modes:
        prefix = HERE/directory/f'{pathlib_name(netpath)}_{fault}_{stuck}_{iteration}_{label}'
        result = run(netpath, fault, stuck, prefix, seconds=seconds,
                     care_observation=conditioned)
        rows.append(result)
        print(json.dumps({k: v for k, v in result.items()
                          if k not in ('factor_results',)}), flush=True)
    return rows


def pathlib_name(path):
    return Path(path).stem


def care_synthetic(pairs=12):
    """Reconvergence where H implies a large unconditional observation DNF."""
    path = HERE/'care_cases'/f'implied_observation_{pairs}.v'
    path.parent.mkdir(parents=True, exist_ok=True)
    inputs = ['x', 'z'] + [v for i in range(pairs) for v in (f'a{i}', f'b{i}')]
    gates = []
    for i in range(pairs):
        gates.append(f'OR2 os{i} (.A(a{i}), .B(b{i}), .Z(s{i}));')
        previous = 'x' if i == 0 else f't{i-1}'
        gates.append(f'AND2 pt{i} (.A({previous}), .B(s{i}), .Z(t{i}));')
        if i == 0:
            gates.append('BUF hb0 (.A(s0), .Z(h0));')
        else:
            gates.append(f'AND2 hc{i} (.A(h{i-1}), .B(s{i}), .Z(h{i}));')
    root = f't{pairs-1}'
    h = f'h{pairs-1}' if pairs > 1 else 'h0'
    gates += [f'OR2 oq (.A({h}), .B(z), .Z(q));',
              'INV iq (.A(q), .Z(nq));',
              f'OR2 oy1 (.A({root}), .B(nq), .Z(y1));',
              f'AND2 oy2 (.A({root}), .B(q), .Z(y2));']
    path.write_text('module care_toy;\ninput '+', '.join(inputs)+
                    ';\noutput y1, y2;\n'+'\n'.join(gates)+'\nendmodule\n')
    return path


def c17_regression():
    netpath = ROOT/'input/circuit/c17a.v'
    net = Netlist(netpath)
    with (ROOT/'expected/c17a_result.csv').open() as file:
        golden = {(r['net_name'], int(r['f_type'][-1])): r['fdp']
                  for r in csv.DictReader(file)}
    rows = []
    for fault in net.pi+list(net.g):
        for stuck in (0, 1):
            prefix = HERE/'care_regression'/f'{fault}_{stuck}'
            result = run(netpath, fault, stuck, prefix, seconds=3,
                         care_observation=True)
            assert result['complete'] and result['proof'] == {'sound': True, 'exact': True}
            probability = Fraction(result['union']['probability'])
            exhaustive = Fraction(sum(net.simulate(bits, fault, stuck) for bits in
                                      itertools.product([0, 1], repeat=len(net.pi))),
                                  1 << len(net.pi))
            assert probability == exhaustive
            assert f'{float(probability):.10e}' == golden[(fault, stuck)]
            rows.append(result)
    summary = {'faults': len(rows), 'all_complete': True, 'raw_cnf_exact': True,
               'exhaustive_exact': True, 'golden_exact': True}
    (HERE/'results/care_c17.json').write_text(json.dumps(rows, indent=2))
    print(json.dumps(summary), flush=True)


def screen_candidates(seconds):
    """Rank a fixed s5378 sample by H/O PI-support overlap, then measure top 8."""
    netpath = ROOT/'input/circuit/s5378_C.v'
    net = Netlist(netpath)
    signals = net.pi+list(net.g)
    sample = random.Random(19).sample(signals, 200)
    ranked = []
    for fault in sample:
        ex, _, _, _, _, care, observable, root, path = prepare(net, fault, 0)
        hs = {i for i in ex.reachable(care) if i <= ex.n}
        os = {i for i in ex.reachable(observable) if i <= ex.n}
        ranked.append({'fault': fault, 'root': root, 'path_gates': len(path),
                       'care_support': len(hs), 'observation_support': len(os),
                       'support_overlap': len(hs & os)})
    ranked.sort(key=lambda row: (row['support_overlap'], row['care_support'],
                                 row['observation_support'], row['path_gates'],
                                 row['fault']), reverse=True)
    selected = ranked[:8]
    (HERE/'results/care_screen_supports.json').write_text(json.dumps(ranked, indent=2))
    rows = []
    for candidate in selected:
        for stuck in (0, 1):
            rows.extend(run_pair(netpath, candidate['fault'], stuck, seconds,
                                 directory='care_screen_runs'))
    (HERE/'results/care_screen.json').write_text(json.dumps(rows, indent=2))
    print(json.dumps({'sample': len(sample), 'selected': selected}), flush=True)


def large_support_sample():
    netpath = ROOT/'input/circuit/s38584_C.v'
    net = Netlist(netpath)
    sample = random.Random(23).sample(net.pi+list(net.g), 30)
    rows = []
    for fault in sample:
        ex, _, _, _, _, care, observable, root, path = prepare(net, fault, 0)
        hs = {i for i in ex.reachable(care) if i <= ex.n}
        os = {i for i in ex.reachable(observable) if i <= ex.n}
        rows.append({'fault': fault, 'root': root, 'path_gates': len(path),
                     'care_support': len(hs), 'observation_support': len(os),
                     'support_overlap': len(hs & os)})
    rows.sort(key=lambda row: (row['support_overlap'], row['care_support'],
                               row['observation_support'], row['path_gates'],
                               row['fault']), reverse=True)
    result = {'sample_seed': 23, 'sample_size': len(sample), 'rows': rows}
    (HERE/'results/care_s38584_support_sample.json').write_text(json.dumps(result, indent=2))
    print(json.dumps({'sample': len(sample), 'top': rows[:3]}), flush=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--c17', action='store_true')
    parser.add_argument('--candidates', action='store_true')
    parser.add_argument('--synthetic', action='store_true')
    parser.add_argument('--screen', action='store_true')
    parser.add_argument('--large', action='store_true')
    parser.add_argument('--large-screen', action='store_true')
    parser.add_argument('--repeat', type=int, default=1)
    parser.add_argument('--seconds', type=float, default=2)
    args = parser.parse_args()
    (HERE/'results').mkdir(exist_ok=True)
    if args.c17:
        c17_regression()
        return
    if args.synthetic:
        netpath = care_synthetic()
        rows = run_pair(netpath, 'x', 0, args.seconds,
                        directory='care_synthetic_runs')
        (HERE/'results/care_synthetic.json').write_text(json.dumps(rows, indent=2))
        return
    if args.screen:
        screen_candidates(args.seconds)
        return
    if args.large:
        rows = []
        netpath = ROOT/'input/circuit/s38584_C.v'
        for iteration in range(args.repeat):
            rows.extend(run_pair(netpath, 'II16102', 1, args.seconds, iteration,
                                 reverse=bool(iteration & 1),
                                 directory='care_s38584_repeats'))
        (HERE/'results/care_s38584_repeats.json').write_text(json.dumps(rows, indent=2))
        return
    if args.large_screen:
        large_support_sample()
        return
    netpath = ROOT/'input/circuit/s5378_C.v'
    cases = ([('II4216', 1), ('n79gat', 1), ('n995gat', 0), ('n518gat', 1)]
             if args.candidates else
             [('n1592gat', 0), ('n2428gat', 0), ('n2432gat', 0),
              ('n673gat', 0), ('n291gat', 1)])
    rows = []
    for iteration in range(args.repeat):
        for fault, stuck in cases:
            rows.extend(run_pair(netpath, fault, stuck, args.seconds, iteration,
                                 reverse=bool(iteration & 1),
                                 directory=('care_candidate_repeats' if args.candidates
                                            else 'care_initial_runs')))
    result_name = 'care_candidates.json' if args.candidates else 'care_real.json'
    (HERE/'results'/result_name).write_text(json.dumps(rows, indent=2))
    for fault, _ in cases:
        selected = [r for r in rows if r['fault'] == fault]
        report = {}
        for conditioned in (False, True):
            mode = [r for r in selected if r['care_observation'] == conditioned]
            report['care' if conditioned else 'ordinary'] = {
                'complete': sum(r['complete'] for r in mode),
                'median_cubes': statistics.median(r['generated_cubes'] for r in mode),
                'median_seconds': statistics.median(
                    r['total_seconds_excluding_raw_proof'] for r in mode),
            }
        print(json.dumps({'fault': fault, **report}), flush=True)


if __name__ == '__main__':
    main()
