"""FFR path factorization; each factor is solved by SAT + semantic DC.

The counting process reads only generated PI cube covers. Overlapping supports
are retained explicitly, so multiplication of marginal probabilities is avoided.
SAF stem faults only. This is not an implementation of the paper's H-ASS.
"""
import argparse
import collections
import functools
import importlib.util
import json
import pathlib
import subprocess
import sys
import time

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent.parent
sys.path.insert(0, str(HERE.parent / 'linear_coordinates'))
from probe import Netlist, Expr, SAT

spec = importlib.util.spec_from_file_location('ffr_raw', HERE.parent/'linear_coordinates/verify.py')
raw = importlib.util.module_from_spec(spec)
spec.loader.exec_module(raw)


def write_cnf(path, clauses, nv):
    path.write_text(f'p cnf {nv} {len(clauses)}\n' + ''.join(
        ' '.join(map(str, clause))+' 0\n' for clause in clauses))


def path_to_root(net, fault):
    fanout = collections.defaultdict(list)
    for output, (_, inputs) in net.g.items():
        for pin, signal in enumerate(inputs):
            fanout[signal].append((output, pin))
    observed = set(net.po)
    path = []
    current = fault
    while current not in observed and len(fanout[current]) == 1:
        output, pin = fanout[current][0]
        path.append((output, pin))
        current = output
    return current, path


def prepare(net, fault, stuck):
    if fault not in net.pidx and fault not in net.g:
        raise ValueError('Only named stem faults are supported')
    root, path = path_to_root(net, fault)
    ex = Expr(len(net.pi))

    @functools.cache
    def good(signal):
        if signal in net.pidx:
            return 2*(net.pidx[signal]+1)
        kind, inputs = net.g[signal]
        return ex.gate(kind, [good(s) for s in inputs])

    @functools.cache
    def forced(signal, location, value):
        if signal == location:
            return value
        if signal in net.pidx:
            return good(signal)
        kind, inputs = net.g[signal]
        return ex.gate(kind, [forced(s, location, value) for s in inputs])

    local_factors = [('excitation', ex.xor(good(fault), stuck))]
    for output, pin in path:
        kind, inputs = net.g[output]
        if kind in ('AND', 'NAND', 'OR', 'NOR'):
            noncontrol = 1 if kind in ('AND', 'NAND') else 0
            for index, signal in enumerate(inputs):
                if index != pin:
                    local_factors.append((f'{output}:{signal}={noncontrol}',
                                          good(signal) ^ (1-noncontrol)))
        elif kind not in ('BUF', 'INV', 'XOR', 'XNOR', 'EXOR', 'EXNOR'):
            raise ValueError(kind)
    observable = ex.or_(*(ex.xor(forced(o, root, 0), forced(o, root, 1)) for o in net.po))
    # Remove true and duplicate local factors but retain false as an explicit
    # leaf.  Keep observability separate so it can be enumerated under care.
    seen = {1}
    unique_local = []
    for name, lit in local_factors:
        if lit not in seen:
            seen.add(lit)
            unique_local.append((name, lit))
    care = ex.and_(*(lit for _, lit in unique_local))
    factors = list(unique_local)
    if observable not in seen:
        factors.append(('root_observability', observable))
    detection = ex.or_(*(ex.xor(good(o), forced(o, fault, stuck)) for o in net.po))
    combined = ex.and_(care, observable)
    return ex, detection, combined, factors, unique_local, care, observable, root, path


def enumerate_factor(ex, output, prefix, seconds, limit):
    clauses, nv, ol = ex.cnf(output)
    cnf = prefix.with_suffix('.cnf')
    write_cnf(cnf, clauses, nv)
    support = sorted(i for i in ex.reachable(output) if i <= ex.n)
    if not support:
        assert output in (0, 1)
        cubes = [[]] if output else []
        return {'complete': True, 'cubes': len(cubes), 'sat_calls': 0, 'seconds': 0}, cubes
    command = [str(HERE.parent/'linear_scaling/native/build/enumerator'),
               '--cnf', str(cnf), '--npi', str(ex.n), '--output', str(ol),
               '--support', ','.join(map(str, support)), '--method', 'greedy',
               '--seconds', str(seconds), '--limit', str(limit), '--prefix', str(prefix)]
    process = subprocess.run(command, text=True, capture_output=True, check=True,
                             timeout=seconds+10)
    return json.loads(process.stdout), json.loads(prefix.with_suffix('.cubes.json').read_text())


def enumerate_conditioned(ex, output, care, prefix, seconds, limit):
    """Enumerate U with H & U == H & output, where H is ``care``.

    The generated cubes may cover arbitrary assignments outside H.  They are
    therefore valid only when intersected with the separately generated H
    factors and are never counted on their own.
    """
    if care == 0 or output == 1:
        cubes = [[]]
        return {'complete': True, 'cubes': 1, 'sat_calls': 0, 'seconds': 0,
                'care_literal': care, 'conditioned': True}, cubes
    if output == 0:
        return {'complete': True, 'cubes': 0, 'sat_calls': 0, 'seconds': 0,
                'care_literal': care, 'conditioned': True}, []
    if care == 1:
        stats, cubes = enumerate_factor(ex, output, prefix, seconds, limit)
        stats['conditioned'] = True
        return stats, cubes

    joint = ex.and_(care, output)
    # Expr.and_ flattens positive AND nodes.  Build an otherwise redundant raw
    # XOR anchor so the original care and output nodes remain reachable and can
    # safely be passed as assumptions to the native solvers.
    anchor = ex.node('x', [joint, care, care, output, output])
    clauses, nv, _ = ex.cnf(anchor)
    cnf = prefix.with_suffix('.cnf')
    write_cnf(cnf, clauses, nv)

    def cnf_lit(lit):
        assert lit not in (0, 1)
        return -(lit//2) if lit & 1 else lit//2

    support = sorted(i for i in ex.reachable(joint) if i <= ex.n)
    command = [str(HERE.parent/'linear_scaling/native/build/enumerator'),
               '--cnf', str(cnf), '--npi', str(ex.n),
               '--output', str(cnf_lit(output)), '--care', str(cnf_lit(care)),
               '--support', ','.join(map(str, support)), '--method', 'greedy',
               '--seconds', str(seconds), '--limit', str(limit), '--prefix', str(prefix)]
    process = subprocess.run(command, text=True, capture_output=True, check=True,
                             timeout=seconds+10)
    stats = json.loads(process.stdout)
    stats['conditioned'] = True
    return stats, json.loads(prefix.with_suffix('.cubes.json').read_text())


def verify(net, fault, stuck, covers, prefix):
    """Verify the resulting AND-of-SOP with independent raw gate truth tables."""
    clauses, nv, detected = raw.raw_cnf(net, fault, stuck, [1 << i for i in range(len(net.pi))])

    def and_(lits):
        nonlocal nv
        nv += 1
        z = nv
        clauses.extend([[-z, a] for a in lits])
        clauses.append([z]+[-a for a in lits])
        return z

    def or_(lits):
        return -and_([-a for a in lits])

    covered = and_([or_([and_(cube) for cube in cubes]) for cubes in covers])
    cnf = prefix.with_suffix('.raw.cnf')
    write_cnf(cnf, clauses, nv)
    solver = SAT(cnf, 0)
    try:
        sound = solver.solve([covered, -detected])[0] == 20
        exact = solver.solve([-covered, detected])[0] == 20
        assert sound and exact, (fault, stuck, sound, exact)
    finally:
        solver.close()
    return {'sound': sound, 'exact': exact}


def run(netpath, fault, stuck, prefix, seconds=3, limit=100000, flat=False,
        care_observation=False):
    if flat and care_observation:
        raise ValueError('flat and care_observation are mutually exclusive')
    prefix = pathlib.Path(prefix)
    prefix.parent.mkdir(parents=True, exist_ok=True)
    start = time.monotonic()
    net = Netlist(netpath)
    ex, detection, combined, factors, local_factors, care, observable, root, path = prepare(
        net, fault, stuck)
    # A SAT miter also proves the structural identity before enumeration.
    clauses, nv, mismatch = ex.cnf(ex.xor(detection, combined))
    identity = prefix.with_suffix('.identity.cnf')
    write_cnf(identity, clauses, nv)
    solver = SAT(identity, 0)
    try:
        assert solver.solve([mismatch])[0] == 20, 'FFR identity failed'
    finally:
        solver.close()
    if flat:
        factors = [('flat_detection', detection)]
    elif care_observation:
        factors = list(local_factors) + [('root_observability_under_care', observable)]
    results, covers = [], []
    for index, (name, lit) in enumerate(factors):
        p = prefix.parent/(prefix.name+f'_factor{index:03d}')
        if care_observation and index == len(factors)-1:
            stats, cubes = enumerate_conditioned(ex, lit, care, p, seconds, limit)
        else:
            stats, cubes = enumerate_factor(ex, lit, p, seconds, limit)
        results.append({'name': name, **stats})
        covers.append(cubes)
        if not stats['complete']:
            break
    complete = len(results) == len(factors) and all(r['complete'] for r in results)
    counter = None
    if complete:
        data = f'{ex.n} {len(covers)}\n'
        for cubes in covers:
            data += str(len(cubes))+'\n'+''.join(' '.join(map(str,c))+' 0\n' for c in cubes)
        counter = json.loads(subprocess.run([str(HERE/'build/factor_union')], input=data,
                             text=True, capture_output=True, check=True, timeout=30).stdout)
    elapsed = time.monotonic()-start
    proof = verify(net, fault, stuck, covers, prefix) if complete else None
    result = {'net': str(netpath), 'fault': fault, 'stuck': stuck, 'root': root,
              'path_gates': len(path), 'flat': flat,
              'care_observation': care_observation, 'factors': len(factors),
              'complete': complete, 'generated_cubes': sum(r['cubes'] for r in results),
              'factor_results': results, 'union': counter, 'proof': proof,
              'total_seconds_excluding_raw_proof': elapsed}
    prefix.with_suffix('.covers.json').write_text(json.dumps(covers))
    prefix.with_suffix('.result.json').write_text(json.dumps(result, indent=2))
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--net', required=True)
    parser.add_argument('--fault', required=True)
    parser.add_argument('--stuck', type=int, default=0)
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--seconds', type=float, default=3)
    parser.add_argument('--limit', type=int, default=100000)
    parser.add_argument('--flat', action='store_true')
    parser.add_argument('--care-observation', action='store_true')
    args = parser.parse_args()
    if args.flat and args.care_observation:
        parser.error('--flat and --care-observation are mutually exclusive')
    print(json.dumps(run(args.net, args.fault, args.stuck, args.prefix,
                         args.seconds, args.limit, args.flat, args.care_observation)))
