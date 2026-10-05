"""Retry incomplete binary covers with SAT-certified block regions.

This is a second measured phase: initial work is charged to the combined total.
No direct circuit BDD or counting fallback is used.
"""
import argparse
import csv
import json
import pathlib
import time
from fractions import Fraction
from experiment import CachedNet, run_fault


def finish(initial, outdir, width=3, seconds=2):
    initial, outdir = pathlib.Path(initial), pathlib.Path(outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    start = time.monotonic()
    before = json.loads((initial/'summary.json').read_text())
    rows = list(map(json.loads, (initial/'faults.jsonl').read_text().splitlines()))
    net = CachedNet(initial/'expanded.v')
    retries = []
    for i, row in enumerate(rows):
        if row['complete']:
            continue
        prefix = outdir/f'retry_{i:05d}'
        result = run_fault(net, row['fault'], row['stuck'], prefix,
                           variant=before['variant'], width=width, seconds=seconds, simulate=True)
        retries.append({'fault': row['fault'], 'stuck': row['stuck'], 'prefix': str(prefix), **result})
        # Retain the strongest lower bound if neither enumeration finishes.
        if result['complete'] or Fraction(result['union']['probability']) >= Fraction(row['fdp']):
            rows[i] = {**row, 'complete': result['complete'], 'fdp': result['union']['probability'],
                       'cubes': result['cubes'], 'sat_calls': result['sat_calls'],
                       'enum_seconds': result['seconds'], 'union_seconds': result['union']['seconds'],
                       'prefix': str(prefix), 'reused': False, 'retried_width': width}
    with open(outdir/'faults.jsonl', 'w') as fp:
        for row in rows:
            fp.write(json.dumps(row)+'\n')
    with open(outdir/'fdp.csv', 'w', newline='') as fp:
        writer = csv.writer(fp)
        writer.writerow(['net_name', 'f_type', 'complete', 'fdp_exact', 'fdp', 'regions'])
        for row in rows:
            writer.writerow([row['fault'], f"sa{row['stuck']}", int(row['complete']), row['fdp'],
                             format(float(Fraction(row['fdp'])), '.17g'), row['cubes']])
    elapsed = time.monotonic()-start
    summary = {'faults': len(rows), 'complete': sum(r['complete'] for r in rows),
               'incomplete': sum(not r['complete'] for r in rows), 'retried': len(retries),
               'initial_seconds': before['wall_seconds'], 'completion_phase_seconds': elapsed,
               'combined_seconds': before['wall_seconds']+elapsed, 'retry_width': width,
               'retry_seconds_limit': seconds, 'retries': retries}
    (outdir/'summary.json').write_text(json.dumps(summary, indent=2))
    print(json.dumps({k: v for k, v in summary.items() if k != 'retries'}))
    return summary


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--initial', required=True)
    p.add_argument('--outdir', required=True)
    p.add_argument('--width', type=int, default=3)
    p.add_argument('--seconds', type=float, default=2)
    a = p.parse_args()
    finish(a.initial, a.outdir, a.width, a.seconds)
