"""Factor a detection SOP without expanding products of independent covers.

Only pairwise disjoint input supports are combined arithmetically. Every leaf
is enumerated by SAT + semantic don't-care shrinking and counted by cover BDD.
There is no Shannon counting search and no circuit-to-BDD construction.
"""
import argparse
import functools
import json
import pathlib
import subprocess
import time
from fractions import Fraction

from fast_basis import HERE, FastNet, build_fast, compact, bits, probe
from experiment import shape_key, run_fault


def partition_tree(ex, out):
    @functools.cache
    def support(k):
        if k == 0:
            return 0
        if k <= ex.n:
            return 1 << (k-1)
        acc = 0
        for lit in ex.nodes[k][1]:
            acc |= support(lit//2)
        return acc
    def rec(lit):
        k = lit//2
        if k <= ex.n or ex.nodes[k][0] != 'a':
            return {'kind': 'leaf', 'lit': lit, 'support': support(k)}
        args = ex.nodes[k][1]
        parent = list(range(len(args)))
        def root(i):
            while parent[i] != i:
                parent[i] = parent[parent[i]]
                i = parent[i]
            return i
        owner = {}
        for i, a in enumerate(args):
            for v in bits(support(a//2)):
                if v in owner:
                    parent[root(i)] = root(owner[v])
                else:
                    owner[v] = i
        groups = {}
        for i, a in enumerate(args):
            groups.setdefault(root(i), []).append(a)
        if len(groups) == 1:
            return {'kind': 'leaf', 'lit': lit, 'support': support(k)}
        neg = lit & 1
        children = [rec(ex.and_(*group) ^ neg) for group in groups.values()]
        seen = 0
        for child in children:
            assert not (seen & child['support'])
            seen |= child['support']
        return {'kind': 'or' if neg else 'and', 'children': children, 'support': seen}
    return rec(out)


class Engine:
    def __init__(self, outdir, width=3, factored=True, sharing=True, seconds=2, difference=False, complement=False, lean=False, sweep=False, exact_only=False, union_timeout=60):
        self.outdir = pathlib.Path(outdir)
        self.outdir.mkdir(parents=True, exist_ok=True)
        self.width, self.factored, self.sharing, self.seconds = width, factored, sharing, seconds
        self.difference = difference
        self.complement = complement
        self.lean = lean
        self.sweep = sweep
        self.exact_only = exact_only
        self.union_timeout = union_timeout
        self.resource_timeouts = 0
        self.complement_reused = 0
        self.cache = {}
        self.solved = self.reused = 0
        self.sat_calls = self.regions = 0
        self.enum_seconds = self.union_seconds = 0.0

    def fault(self, net, fault, stuck, index=0):
        start = time.monotonic()
        oldex, oldout, rows, _, npar, _ = build_fast(net, fault, stuck, difference=self.difference)
        ex, out, active = compact(oldex, oldout)
        rows = [rows[v-1] for v in active]
        build_seconds = time.monotonic()-start
        rewrite_seconds = 0.0
        if self.sweep and len(ex.nodes) > 1000:
            from sweep import rewrite
            t = time.monotonic()
            ex, out, kept, _ = rewrite(ex, out, self.outdir/f'fault{index:07d}', seconds=0.1)
            rows = [rows[v-1] for v in kept]
            rewrite_seconds = time.monotonic()-t
        tree = partition_tree(ex, out) if self.factored else {'kind': 'leaf', 'lit': out}
        split_seconds = time.monotonic()-start-build_seconds-rewrite_seconds
        deadline = start + self.seconds
        before = (self.solved, self.reused, self.sat_calls, self.regions)
        def visit(node):
            if node['kind'] != 'leaf':
                children = []
                for child in node['children']:
                    children.append(visit(child))
                ps = [Fraction(c['probability']) for c in children]
                p = Fraction(1)
                for value in ps:
                    p *= value if node['kind'] == 'and' else 1-value
                if node['kind'] == 'or':
                    p = 1-p
                return {'kind': node['kind'], 'children': children, 'probability': str(p),
                        'complete': all(c['complete'] for c in children)}
            leaf, output, local_active = compact(ex, node['lit'])
            key, mapping = shape_key(leaf, output)
            inverted = False
            complement_key, complement_mapping = shape_key(leaf, output ^ 1) if self.complement else (None, None)
            if self.sharing and key in self.cache:
                prior = self.cache[key]
                self.reused += 1
            elif self.sharing and self.complement and complement_key in self.cache:
                # Only completed covers are cached: complementing a partial
                # cover would produce an upper bound, not a sound lower cover.
                prior = self.cache[complement_key]
                mapping = complement_mapping
                inverted = True
                self.reused += 1
                self.complement_reused += 1
            else:
                left = deadline-time.monotonic()
                if left <= 0:
                    return {'kind': 'empty', 'probability': '0', 'complete': False}
                prefix = self.outdir/f'leaf{self.solved:07d}'
                prepared = (leaf, output, [rows[i-1] for i in local_active], [], npar, set())
                self.solved += 1
                try:
                    result = run_fault(net, fault, stuck, prefix, width=self.width, seconds=left,
                                       prepared=prepared, simulate=True,
                                       native_binary=HERE/'build/enumerator' if self.lean else None,
                                       native_options=['--freeze-support-only','1','--off-unit','1'] if self.lean else None,
                                       count_partial=not self.exact_only, union_timeout=self.union_timeout)
                except subprocess.TimeoutExpired:
                    self.resource_timeouts += 1
                    stats_file = prefix.with_suffix('.result.json')
                    if stats_file.exists():
                        stats = json.loads(stats_file.read_text())
                        self.sat_calls += stats['sat_calls']
                        self.regions += stats['cubes']
                        self.enum_seconds += stats['seconds']
                    return {'kind': 'empty', 'probability': '0', 'complete': False,
                            'cause': 'resource_timeout', 'discarded_cover': str(prefix)}
                self.sat_calls += result['sat_calls']
                self.regions += result['cubes']
                self.enum_seconds += result['seconds']
                if result['union'] is None:
                    return {'kind': 'empty', 'probability': '0', 'complete': False,
                            'cause': 'enumeration_incomplete', 'discarded_cover': str(prefix)}
                self.union_seconds += result['union']['seconds']
                prior = {'prefix': str(prefix), 'mapping': mapping, 'probability': result['union']['probability'],
                         'complete': result['complete'], 'regions': result['cubes']}
                if result['complete']:
                    self.cache[key] = prior
            inverse = {label: (var, phase) for var, (label, phase) in mapping.items()}
            remap = {}
            for var, (label, phase) in prior['mapping'].items():
                target, target_phase = inverse[label]
                global_var = local_active[target-1]
                remap[var] = -global_var if phase ^ target_phase else global_var
            probability = 1-Fraction(prior['probability']) if inverted else Fraction(prior['probability'])
            return {'kind': 'cover', 'prefix': prior['prefix'], 'map': remap, 'negated': inverted,
                    'probability': str(probability), 'complete': prior['complete'], 'regions': prior['regions']}
        covered = visit(tree)
        manifest = {'fault': fault, 'stuck': stuck, 'original_npi': len(net.pi), 'ncoords': ex.n,
                    'basis_rows': [hex(r) for r in rows], 'tree': covered}
        prefix = self.outdir/f'fault{index:07d}'
        prefix.with_suffix('.cover.json').write_text(json.dumps(manifest))
        result = {'fault': fault, 'stuck': stuck, 'complete': covered['complete'], 'fdp': covered['probability'],
                  'new_leaves': self.solved-before[0], 'reused_leaves': self.reused-before[1],
                  'sat_calls': self.sat_calls-before[2], 'generated_regions': self.regions-before[3],
                  'active_inputs': ex.n, 'build_seconds': build_seconds, 'split_seconds': split_seconds,
                  'rewrite_seconds': rewrite_seconds,
                  'seconds': time.monotonic()-start, 'manifest': str(prefix.with_suffix('.cover.json'))}
        return result


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--net', required=True)
    p.add_argument('--fault', required=True)
    p.add_argument('--stuck', type=int, default=0)
    p.add_argument('--outdir', required=True)
    p.add_argument('--width', type=int, default=3)
    p.add_argument('--seconds', type=float, default=5)
    p.add_argument('--flat', action='store_true')
    p.add_argument('--difference', action='store_true')
    p.add_argument('--complement', action='store_true')
    p.add_argument('--lean', action='store_true')
    p.add_argument('--sweep', action='store_true')
    p.add_argument('--exact-only', action='store_true')
    p.add_argument('--union-seconds', type=float, default=60)
    a = p.parse_args()
    net = FastNet(a.net)
    engine = Engine(a.outdir, a.width, not a.flat, seconds=a.seconds, difference=a.difference, complement=a.complement, lean=a.lean, sweep=a.sweep, exact_only=a.exact_only, union_timeout=a.union_seconds)
    result = engine.fault(net, a.fault, a.stuck)
    (engine.outdir/'result.json').write_text(json.dumps(result, indent=2))
    print(json.dumps(result))
