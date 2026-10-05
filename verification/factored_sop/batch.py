"""Fixed-target large-circuit comparisons; all artifacts remain in verification."""
import argparse
import json
import pathlib
import resource
import time

from factor import Engine, FastNet
from fast_basis import HERE
from expand_connections import expand


def batch(netpath, fault_file, outdir, mode='factored', width=3, seconds=2, branches=False, exact_only=False, union_timeout=60):
    outdir = pathlib.Path(outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    start = time.monotonic()
    if branches:
        netpath = expand(netpath, outdir/'expanded.v')
    net = FastNet(netpath)
    preprocessing = time.monotonic()-start
    faults = [line.split() for line in pathlib.Path(fault_file).read_text().splitlines() if line.strip() and not line.startswith('#')]
    engine = Engine(outdir, width, mode != 'flat', seconds=seconds,
                    difference=mode in ('difference', 'complement', 'lean', 'sweep'),
                    complement=mode in ('complement', 'lean', 'sweep'), lean=mode == 'lean', sweep=mode == 'sweep',
                    exact_only=exact_only, union_timeout=union_timeout)
    results = []
    with open(outdir/'faults.jsonl', 'w') as fp:
        for i, (fault, kind) in enumerate(faults):
            assert kind in ('sa0', 'sa1')
            result = engine.fault(net, fault, int(kind[-1]), i)
            results.append(result)
            fp.write(json.dumps(result)+'\n')
            fp.flush()
            if (i+1) % 20 == 0:
                print(json.dumps({'done': i+1, 'complete': sum(r['complete'] for r in results),
                                  'seconds': time.monotonic()-start, 'new_leaves': engine.solved,
                                  'reused_leaves': engine.reused}), flush=True)
    summary = {'faults': len(results), 'complete': sum(r['complete'] for r in results),
               'incomplete': sum(not r['complete'] for r in results), 'wall_seconds': time.monotonic()-start,
               'preprocessing_seconds': preprocessing, 'build_seconds': sum(r['build_seconds'] for r in results),
               'rewrite_seconds': sum(r['rewrite_seconds'] for r in results),
               'new_leaves': engine.solved, 'reused_leaves': engine.reused,
               'complement_reused': engine.complement_reused,
               'resource_timeouts': engine.resource_timeouts, 'exact_only': exact_only, 'union_timeout': union_timeout,
               'sat_calls': engine.sat_calls, 'generated_regions': engine.regions,
               'enum_seconds': engine.enum_seconds, 'union_seconds': engine.union_seconds,
               'maxrss_kb': resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,
               'mode': mode, 'width': width, 'fault_seconds_budget': seconds,
               'net': str(netpath), 'fault_list': str(fault_file), 'branches': branches}
    (outdir/'summary.json').write_text(json.dumps(summary, indent=2))
    print(json.dumps(summary), flush=True)
    return summary


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--net', required=True)
    p.add_argument('--faults', required=True)
    p.add_argument('--outdir', required=True)
    p.add_argument('--mode', choices=['flat', 'factored', 'difference', 'complement', 'lean', 'sweep'], default='factored')
    p.add_argument('--width', type=int, default=3)
    p.add_argument('--seconds', type=float, default=2)
    p.add_argument('--expand-branches', action='store_true')
    p.add_argument('--exact-only', action='store_true')
    p.add_argument('--union-seconds', type=float, default=60)
    a = p.parse_args()
    batch(a.net, a.faults, a.outdir, a.mode, a.width, a.seconds, a.expand_branches, a.exact_only, a.union_seconds)
