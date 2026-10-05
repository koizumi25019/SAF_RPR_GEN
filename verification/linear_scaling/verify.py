"""Independent raw-gate SAT certificates, exhaustive small-circuit checks.

BDD is used only by the runner to count generated covers. This verifier proves
soundness/completeness with an independent gate CNF, including transformed cubes.
"""
import argparse
import csv
import itertools
import json
import pathlib
import random
import sys
import time
from fractions import Fraction

from experiment import HERE, CachedNet, build_cached, probe, run_fault, shape_key, state_encoding, write_cnf
from expand_connections import expand
# Avoid importing this file again under the old verifier's name.
import importlib.util
spec = importlib.util.spec_from_file_location('raw_gate_verifier', HERE.parent/'linear_coordinates/verify.py')
raw = importlib.util.module_from_spec(spec)
spec.loader.exec_module(raw)


def certify(net, fault, stuck, rows, cubes, prefix, groups=None, regions=None, complete=True):
    clauses, nv, out = raw.raw_cnf(net, fault, stuck, rows)
    if groups is not None:
        extra, nv, states = state_encoding(groups, nv)
        clauses += extra
        cubes = [[-q for qs, mask in zip(states, masks) for v, q in enumerate(qs) if not (mask >> v & 1)]
                 for masks in regions]
    prefix = pathlib.Path(prefix)
    prefix.parent.mkdir(parents=True, exist_ok=True)
    path = prefix.with_suffix('.raw.cnf')
    write_cnf(path, clauses, nv)
    off, det = probe.SAT(path, nv), probe.SAT(path, nv)
    start = time.monotonic()
    try:
        for i, cube in enumerate(cubes):
            rc, _ = off.solve([-out] + cube)
            assert rc == 20, ('unsound', fault, stuck, i)
            det.add([-l for l in cube])
        rc, _ = det.solve([out])
        assert not complete or rc == 20, ('incomplete', fault, stuck)
        result = {'fault': fault, 'stuck': stuck, 'sound_regions': len(cubes), 'coverage_unsat': rc == 20,
                  'seconds': time.monotonic() - start}
        prefix.with_suffix('.verified.json').write_text(json.dumps(result, indent=2))
        return result
    finally:
        off.close()
        det.close()


def saved_cover(netpath, prefix):
    prefix = pathlib.Path(prefix)
    meta = json.loads(prefix.with_suffix('.metadata.json').read_text())
    result = json.loads(prefix.with_suffix('.result.json').read_text())
    regions = None
    if meta['groups'] is not None:
        regions = json.loads(prefix.with_suffix('.regions.json').read_text())['regions']
    return certify(probe.Netlist(netpath), meta['fault'], meta['stuck'],
                   [int(r, 16) for r in meta['basis_rows']],
                   json.loads(prefix.with_suffix('.cubes.json').read_text()), prefix,
                   meta['groups'], regions, result['complete'])


def reused_covers(outdir, sample=24):
    outdir = pathlib.Path(outdir)
    net = CachedNet(outdir/'expanded.v')
    results = list(map(json.loads, (outdir/'faults.jsonl').read_text().splitlines()))
    candidates = [r for r in results if r['reused'] and r['complete']]
    chosen = random.Random(20260911).sample(candidates, min(sample, len(candidates)))
    verified = []
    for i, row in enumerate(chosen):
        prior = results[row['representative']]
        prefix = pathlib.Path(prior['prefix'])
        meta = json.loads(prefix.with_suffix('.metadata.json').read_text())
        assert meta['width'] == 1, 'Reuse verifier currently supports binary cubes'
        ex, out, rows, *_ = build_cached(net, row['fault'], row['stuck'], meta['variant'])
        key, mapping = shape_key(ex, out)
        old_ex, old_out, *_ = build_cached(net, prior['fault'], prior['stuck'], meta['variant'])
        old_key, old_map = shape_key(old_ex, old_out)
        assert key == old_key
        inverse = {label: (var, phase) for var, (label, phase) in mapping.items()}
        transformed = []
        for cube in json.loads(prefix.with_suffix('.cubes.json').read_text()):
            new = []
            for lit in cube:
                label, phase = old_map[abs(lit)]
                var, target_phase = inverse[label]
                new.append(-var if (lit < 0) ^ phase ^ target_phase else var)
            transformed.append(new)
        verified.append(certify(net, row['fault'], row['stuck'], rows, transformed, outdir/f'verify_reuse_{i:03d}'))
    report = {'reused_faults_checked': len(verified), 'all_sound_complete': True, 'details': verified}
    (outdir/'reuse_verified.json').write_text(json.dumps(report, indent=2))
    return report


def c17_regression(outdir):
    outdir = pathlib.Path(outdir)
    net = CachedNet(expand(probe.ROOT/'input/circuit/c17a.v', outdir/'expanded.v'))
    with open(probe.ROOT/'expected/c17a_result.csv') as fp:
        golden = list(csv.DictReader(fp))
    reps = {(r['net_name'], int(r['f_type'][-1])) for r in golden if r['complete'].strip()}
    expected = {(r['net_name'], int(r['f_type'][-1])): Fraction(r['fdp']) for r in golden}
    checked = []
    for i, (fault, stuck) in enumerate(sorted(reps)):
        for width in (1, 3, 6):
            prefix = outdir/f'f{i:03d}_w{width}'
            prepared = build_cached(net, fault, stuck, 'long')
            ex, out, rows, *_ = prepared
            detected = 0
            for bits in itertools.product((0, 1), repeat=len(net.pi)):
                x = sum(b << j for j, b in enumerate(bits))
                y = [(x & r).bit_count() & 1 for r in rows]
                value = net.simulate(bits, fault, stuck)
                assert ex.evaluate(out, y) == value
                detected += value
            count = Fraction(detected, 1 << len(net.pi))
            assert count == expected[(fault, stuck)]
            result = run_fault(net, fault, stuck, prefix, width=width, prepared=prepared, simulate=True)
            assert result['complete'] and Fraction(result['union']['probability']) == count
            checked.append(saved_cover(outdir/'expanded.v', prefix))
    report = {'representative_faults': len(reps), 'widths': [1, 3, 6], 'runs': len(checked),
              'assignments_per_fault': 1 << len(net.pi), 'golden_match': True, 'all_sound_complete': True}
    (outdir/'verification.json').write_text(json.dumps(report, indent=2))
    return report


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--net', default='input/circuit/s5378_C.v')
    p.add_argument('--prefix')
    p.add_argument('--reused')
    p.add_argument('--sample', type=int, default=24)
    p.add_argument('--c17')
    a = p.parse_args()
    if a.c17: result = c17_regression(a.c17)
    elif a.reused: result = reused_covers(a.reused, a.sample)
    elif a.prefix: result = saved_cover(a.net, a.prefix)
    else: p.error('Specify --prefix, --reused or --c17')
    print(json.dumps(result))
