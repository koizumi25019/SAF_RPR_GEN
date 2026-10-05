#!/usr/bin/env python3
"""Compare XID and paper CORE serially, without diagnostic timing overhead."""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import resource
import statistics
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repeats', type=int, default=7)
    parser.add_argument('circuits', nargs='*', default=['c17a', 's27_C', 's208_C', 's298_C'])
    args = parser.parse_args()
    if args.repeats < 1:
        parser.error('--repeats must be positive')
    binary = ROOT / 'build/main_release'
    out = ROOT / 'output/paper_core_benchmark' / time.strftime('%Y%m%dT%H%M%SZ', time.gmtime())
    out.mkdir(parents=True)
    cpu = min(os.sched_getaffinity(0))
    os.sched_setaffinity(0, {cpu})  # Single CPU for this script and its children.
    hooks = set()
    for source in (ROOT / 'src').rglob('*.c'):
        hooks.update(re.findall(r'getenv\("([A-Z0-9_]+)"\)', source.read_text(errors='replace')))
    env_base = {k: v for k, v in os.environ.items() if k not in hooks}
    references = {}
    measurements = []

    def run(circuit, mode, label, verify=False):
        stem = out / f'{circuit}_{mode}_{label}'
        set_path = stem.with_suffix('.set')
        set_path.write_text(f'-net {ROOT / "input/circuit" / (circuit + ".v")}\n'
                            f'-fdp {stem.with_suffix(".csv")}\n-log {stem.with_suffix(".log")}\n'
                            f'-dc_method {mode}\n-dom_reuse off\n'
                            f'-core_verify {"on" if verify and mode == "core" else "off"}\n')
        env = dict(env_base)
        if verify:
            env['GT_BDD'] = '1'
        before = resource.getrusage(resource.RUSAGE_CHILDREN)
        start = time.perf_counter()
        with stem.with_suffix('.stderr').open('w') as stderr:
            subprocess.run([str(binary), '-set', str(set_path)], cwd=ROOT / 'build',
                           env=env, stdout=subprocess.DEVNULL, stderr=stderr, check=True, timeout=120)
        wall = time.perf_counter() - start
        after = resource.getrusage(resource.RUSAGE_CHILDREN)
        child_cpu = after.ru_utime + after.ru_stime - before.ru_utime - before.ru_stime
        with stem.with_suffix('.csv').open() as f:
            rows = list(csv.DictReader(f))
        assert all(r['complete'] in ('1', '') for r in rows), stem
        fdp = {(r['net_name'], r['f_type']): r['fdp'] for r in rows}
        if circuit not in references:
            references[circuit] = fdp
        assert fdp == references[circuit], f'{stem}: FDP mismatch'
        stderr = stem.with_suffix('.stderr').read_text()
        if verify:
            assert '[GT] summary:' in stderr and 'ALL VERIFIED' in stderr, stem
        else:
            assert '[GT]' not in stderr, stem
        log = stem.with_suffix('.log').read_text()
        def seconds(field):
            match = re.search(r'//\s+' + re.escape(field) + r'\s+: ([0-9.]+) sec', log)
            assert match, (field, stem)
            return float(match[1])
        return dict(circuit=circuit, mode=mode, run=label,
                    faults=sum(r['complete'] == '1' for r in rows),
                    cubes=sum(int(r['cube_cnt']) for r in rows if r['cube_cnt']),
                    cpu_s=child_cpu, wall_s=wall, reported_cpu_s=seconds('CPU Time'),
                    dc_s=seconds("CPU Time (Don't care)"),
                    sat_s=seconds('CPU Time (CaDiCaL)'), bdd_s=seconds('CPU Time (BDD)'))

    for circuit in args.circuits:
        # Warm each mode, then alternate order across repetitions.
        for mode in ('xid', 'core'):
            run(circuit, mode, 'warmup')
        for trial in range(args.repeats):
            for mode in (('xid', 'core') if trial % 2 == 0 else ('core', 'xid')):
                measurements.append(run(circuit, mode, str(trial + 1)))
        # Verify correctness separately; these runs are excluded from timing.
        for mode in ('xid', 'core'):
            run(circuit, mode, 'verify', verify=True)
        print(f'{circuit}: {args.repeats} runs/mode, FDP equal, BDD ALL VERIFIED', flush=True)

    with (out / 'measurements.csv').open('w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=list(measurements[0]))
        writer.writeheader()
        writer.writerows(measurements)
    summary = []
    for circuit in args.circuits:
        for mode in ('xid', 'core'):
            rows = [r for r in measurements if r['circuit'] == circuit and r['mode'] == mode]
            assert len({r['cubes'] for r in rows}) == 1, (circuit, mode, 'cube count varied')
            result = {k: rows[0][k] for k in ('circuit', 'mode', 'faults', 'cubes')}
            for field in ('cpu_s', 'wall_s', 'dc_s', 'sat_s', 'bdd_s'):
                result[field] = statistics.median(r[field] for r in rows)
            result['cpu_min_s'] = min(r['cpu_s'] for r in rows)
            result['cpu_max_s'] = max(r['cpu_s'] for r in rows)
            summary.append(result)
    with (out / 'summary.csv').open('w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=list(summary[0]))
        writer.writeheader()
        writer.writerows(summary)
    (out / 'metadata.json').write_text(json.dumps(dict(
        binary=str(binary), sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
        repeats=args.repeats, cpu=cpu, fault_model='SAF', limit='unlimited',
        dominance_reuse=False, performance_checks=False, verification_separate=True,
        warmups=1, order='alternating', cpu_metric='waited child user+system CPU via getrusage',
        set_controls={'dc_method': 'xid|core', 'dom_reuse': 'off', 'core_verify': 'off (on for CORE validation)'},
        env_controls={key: value for key, value in env_base.items() if key in hooks}), indent=2))
    for row in summary:
        print(f'{row["circuit"]:8} {row["mode"]:4} CPU={row["cpu_s"]:.6f}s '
              f'wall={row["wall_s"]:.6f}s DC={row["dc_s"]:.3f}s cubes={row["cubes"]}')
    print(f'Results: {out}', flush=True)


if __name__ == '__main__':
    main()
