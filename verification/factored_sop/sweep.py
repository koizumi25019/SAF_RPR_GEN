"""Simulation-filtered, SAT-proved equivalences before SOP enumeration."""
import json
import pathlib
import random
import subprocess
import time
from fast_basis import HERE, probe, compact
from experiment import write_cnf


def rewrite(ex, out, prefix, seconds=0.1, max_pairs=1000):
    prefix = pathlib.Path(prefix)
    start = time.monotonic()
    rng = random.Random(20260911)
    mask = (1 << 192)-1
    values = {0: 0, **{i: rng.getrandbits(192) for i in range(1, ex.n+1)}}
    buckets = {0: [0]}
    for i in range(1, ex.n+1):
        sig = values[i]
        canonical = min(sig, sig ^ mask)
        buckets.setdefault(canonical, []).append(2*i + int(sig != canonical))
    pairs = []
    for i in sorted(ex.reachable(out)):
        if i <= ex.n:
            continue
        kind, args = ex.nodes[i]
        sig = mask if kind == 'a' else 0
        for lit in args:
            value = values[lit//2] ^ (mask if lit & 1 else 0)
            sig = sig & value if kind == 'a' else sig ^ value
        values[i] = sig
        canonical = min(sig, sig ^ mask)
        phase = int(sig != canonical)
        bucket = buckets.setdefault(canonical, [])
        for prior in bucket[:3]:
            if len(pairs) < max_pairs:
                pairs.append((i, prior ^ phase))
        if len(bucket) < 3:
            bucket.append(2*i + phase)
    if not pairs:
        return ex, out, list(range(1, ex.n+1)), {'pairs': 0, 'proven': [], 'seconds': time.monotonic()-start}
    clauses, nv, _ = ex.cnf(out)
    nv += 1
    zero = nv
    clauses.append([-zero])
    cnf = prefix.with_suffix('.sweep.cnf')
    write_cnf(cnf, clauses, nv)
    def sat_lit(lit):
        var = lit//2 or zero
        return -var if lit & 1 else var
    path = prefix.with_suffix('.pairs')
    path.write_text(''.join(f'{i} {replacement} {i} {sat_lit(replacement)}\n' for i, replacement in pairs))
    run = subprocess.run([str(HERE/'build/sweep'), str(cnf), str(path), str(seconds)],
                         text=True, capture_output=True, check=True, timeout=seconds+5)
    result = json.loads(run.stdout)
    replacements = {}
    for node, lit in result['proven']:
        replacements.setdefault(node, lit)
    dst = probe.Expr(ex.n)
    memo = {0: 0, **{i: 2*i for i in range(1, ex.n+1)}}
    def translate(lit):
        k = lit//2
        if k not in memo:
            if k in replacements:
                memo[k] = translate(replacements[k])
            else:
                kind, children = ex.nodes[k]
                args = [translate(a) for a in children]
                memo[k] = dst.and_(*args) if kind == 'a' else dst.xor(*args)
        return memo[k] ^ (lit & 1)
    output = translate(out)
    dst, output, active = compact(dst, output)
    result.update({'nodes_before': len(ex.nodes)-1, 'nodes_after': len(dst.nodes)-1,
                   'inputs_before': ex.n, 'inputs_after': dst.n, 'end_to_end_seconds': time.monotonic()-start})
    prefix.with_suffix('.sweep.json').write_text(json.dumps(result, indent=2))
    return dst, output, active, result
