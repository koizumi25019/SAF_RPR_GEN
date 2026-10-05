"""Prove a factored generated cover against independent raw-gate fault CNF."""
import argparse
import collections
import importlib.util
import json
import pathlib
import time
from fractions import Fraction

from fast_basis import HERE, probe
from experiment import write_cnf

spec = importlib.util.spec_from_file_location('independent_raw', HERE.parent/'linear_coordinates/verify.py')
raw = importlib.util.module_from_spec(spec)
spec.loader.exec_module(raw)


def verify(netpath, manifest_path):
    manifest_path = pathlib.Path(manifest_path)
    data = json.loads(manifest_path.read_text())
    net = probe.Netlist(netpath)
    observed = len(net.po)
    if len(net.g) > 10000:
        # Unaffected outputs contribute exactly zero to the detection OR.
        # Slice by graph reachability, without affine or Boolean rewrites.
        fanout = collections.defaultdict(list)
        for output, (_, inputs) in net.g.items():
            for source in inputs:
                fanout[source].append(output)
        affected = {data['fault']}
        stack = [data['fault']]
        while stack:
            for output in fanout[stack.pop()]:
                if output not in affected:
                    affected.add(output)
                    stack.append(output)
        net.po = [o for o in net.po if o in affected]
    rows = [int(r, 16) for r in data['basis_rows']]
    # A full invertible matrix is unnecessary: independent rows give uniform
    # independent projected coordinates, with equally sized original preimages.
    pivots = {}
    for row in rows:
        while row:
            k = row.bit_length()-1
            if k not in pivots:
                pivots[k] = row
                break
            row ^= pivots[k]
        assert row, 'Dependent coordinate rows'
    clauses, nv, detection = raw.raw_cnf(net, data['fault'], data['stuck'], rows)
    nv += 1
    one = nv
    clauses.append([one])
    cache = {}
    def conjunction(lits):
        nonlocal nv
        lits = set(lits)
        if -one in lits or any(-l in lits for l in lits):
            return -one
        lits.discard(one)
        if not lits:
            return one
        if len(lits) == 1:
            return next(iter(lits))
        key = tuple(sorted(lits))
        if key not in cache:
            nv += 1
            z = nv
            clauses.extend([[-z, l] for l in key])
            clauses.append([z]+[-l for l in key])
            cache[key] = z
        return cache[key]
    def disjunction(lits):
        return -conjunction([-l for l in lits])
    def cover(node):
        if node['kind'] == 'empty':
            return -one, set(), Fraction(0)
        if node['kind'] in ('and', 'or'):
            results = [cover(c) for c in node['children']]
            seen = set()
            p = Fraction(1)
            for lit, support, value in results:
                assert not (seen & support), 'Factors are not independent'
                seen.update(support)
                p *= value if node['kind'] == 'and' else 1-value
            if node['kind'] == 'or':
                p = 1-p
            assert p == Fraction(node['probability'])
            combine = conjunction if node['kind'] == 'and' else disjunction
            return combine([r[0] for r in results]), seen, p
        prefix = pathlib.Path(node['prefix'])
        mapping = {int(k): v for k, v in node['map'].items()}
        support = set(map(abs, mapping.values()))
        assert len(support) == len(mapping)
        assert all(1 <= v <= data['ncoords'] for v in support)
        def remap(lit):
            return mapping[abs(lit)] * (1 if lit > 0 else -1)
        meta = json.loads(prefix.with_suffix('.metadata.json').read_text())
        if meta['groups'] is None:
            cubes = json.loads(prefix.with_suffix('.cubes.json').read_text())
            result = disjunction([conjunction([remap(l) for l in c]) for c in cubes])
        else:
            blocks = json.loads(prefix.with_suffix('.regions.json').read_text())
            regions = []
            for masks in blocks['regions']:
                conditions = []
                for group, mask in zip(blocks['groups'], masks):
                    if mask == (1 << (1 << len(group)))-1:
                        continue
                    states = []
                    for v in range(1 << len(group)):
                        if mask >> v & 1:
                            states.append(conjunction([remap(x if v >> j & 1 else -x) for j, x in enumerate(group)]))
                    conditions.append(disjunction(states))
                regions.append(conjunction(conditions))
            result = disjunction(regions)
        union = json.loads(prefix.with_suffix('.union.json').read_text())
        p = Fraction(union['probability'])
        if node.get('negated'):
            assert node['complete'], 'Cannot complement a partial cover'
            result = -result
            p = 1-p
        assert p == Fraction(node['probability'])
        return result, support, p
    covered, _, probability = cover(data['tree'])
    path = manifest_path.with_suffix('.raw.cnf')
    write_cnf(path, clauses, nv)
    solver = probe.SAT(path, 0)
    start = time.monotonic()
    try:
        rc, _ = solver.solve([-detection, covered])
        assert rc == 20, 'Unsound factored cover'
        rc, _ = solver.solve([detection, -covered])
        assert not data['tree']['complete'] or rc == 20, 'Incomplete claimed-complete cover'
    finally:
        solver.close()
    result = {'fault': data['fault'], 'stuck': data['stuck'], 'sound': True, 'exact': rc == 20,
              'probability': str(probability), 'coordinate_rank': len(rows),
              'original_outputs': observed, 'affected_outputs_verified': len(net.po),
              'disjoint_supports_verified': True, 'seconds': time.monotonic()-start}
    manifest_path.with_suffix('.verified.json').write_text(json.dumps(result, indent=2))
    return result


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--net', required=True)
    p.add_argument('--manifest', required=True)
    a = p.parse_args()
    print(json.dumps(verify(a.net, a.manifest)))
