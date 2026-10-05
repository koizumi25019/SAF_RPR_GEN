"""Exploratory process sharding benchmark, independent faults with MDC_NODOM.

Run from repository root after build.py. Production src/build/output are untouched.
This is an external batch experiment, not a thread-safe implementation.
"""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import argparse
import csv
import json
import os
import re
import statistics
import subprocess
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
BIN = HERE / 'build/bin/main_release'
RESEARCH_ENV = ('MAXDC MAXDC_CORE MAXDC_QX MAXDC_HYB MAXDC_NOMUT MAXHAM DUAL '
                'SPLIT PCOUNT BDD_EXACT GT_BDD GT_VERBOSE GT_CUBES GT_COVER GT_ISOP '
                'GT_CUBEDUMP GT_GAIN CUBE_TREND CUBE_TREND_CSV MDC_NODOM MDC_NOEA '
                'MDC_NOPROP MDC_FAULT_TIMEOUT TDF_NOXID XID_EXTERNAL XSTAT '
                'DUMP_CNF AIG_DUMP AIG_DUMP_DIR PF_SNAPSHOT PF_TIMING').split()

def run_case(net, label, faults=None, nodom=False, limit=30, gt=False, snapshot=False):
    directory = HERE / 'runs' / label
    directory.mkdir(parents=True, exist_ok=True)
    settings = [f'-net ../input/circuit/{net}.v',
                f'-log {directory}/timing.log', f'-fdp {directory}/fdp.csv']
    if limit > 0:
        settings.append(f'-limit {limit}')
    if faults is not None:
        fault_file = directory / 'faults.txt'
        fault_file.write_text(''.join(f'{name}\t{kind}\n' for name, kind in faults))
        settings.append(f'-fault {fault_file}')
    set_file = directory / 'run.set'
    set_file.write_text('\n'.join(settings) + '\n')
    env = {k: v for k, v in os.environ.items() if k not in RESEARCH_ENV}
    env['PF_TIMING'] = '1'
    if nodom:
        env['MDC_NODOM'] = '1'
    if gt:
        env['GT_BDD'] = '1'
    if snapshot:
        env['PF_SNAPSHOT'] = str(directory / 'snapshot.csv')
    started = time.perf_counter()
    with (directory / 'stdout.log').open('w') as out, (directory / 'stderr.log').open('w') as err:
        process = subprocess.run(['/usr/bin/time', '-f', '%M', '-o', str(directory / 'rss_kib.txt'),
                                  str(BIN), '-set', str(set_file)],
                                 cwd=ROOT / 'build', env=env, stdout=out, stderr=err)
    wall = time.perf_counter() - started
    if process.returncode != 0:
        raise RuntimeError(f'{label}: exit {process.returncode}; see logs')
    rows = list(csv.DictReader((directory / 'fdp.csv').open()))
    representatives = {(r['net_name'], r['f_type']): r for r in rows if r['complete'] != ''}
    assert len(representatives) == sum(r['complete'] != '' for r in rows), label
    text = (directory / 'stdout.log').read_text()
    dom = re.search(r'\[DOM\] total_cubes=(\d+)\s+seeded=(\d+)\s+sat_calls=(\d+)', text)
    gt_text = (directory / 'stderr.log').read_text()
    if gt:
        assert 'ALL VERIFIED' in gt_text, label
    return {'wall': wall, 'rss_kib': int((directory / 'rss_kib.txt').read_text()),
            'faults': len(representatives),
            'complete': sum(r['complete'] == '1' for r in representatives.values()),
            'cubes': sum(int(r['cube_cnt']) for r in representatives.values()),
            'dom': list(map(int, dom.groups())) if dom else None,
            'gt_verified': gt, 'representatives': representatives}

def snapshot(net):
    run_case(net, f'{net}/snapshot', snapshot=True)
    rows = list(csv.DictReader((HERE / f'runs/{net}/snapshot/snapshot.csv').open()))
    return sorted(rows, key=lambda r: int(r['level']))

def sharded(net, rows, label, workers, chunk, gt=False, limit=30):
    faults = [(r['name'], r['type']) for r in rows]
    # Round-robin distribution across batches avoids packing all high-level faults last.
    batches = [[] for _ in range((len(faults) + chunk - 1) // chunk)]
    for index, fault in enumerate(faults):
        batches[index % len(batches)].append(fault)
    started = time.perf_counter()
    def execute(item):
        index, batch = item
        return run_case(net, f'{label}/batch{index:03}', batch, nodom=True, limit=limit, gt=gt)
    with ThreadPoolExecutor(max_workers=workers) as pool:
        results = list(pool.map(execute, enumerate(batches)))
    wall = time.perf_counter() - started
    merged = {}
    for result in results:
        assert merged.keys().isdisjoint(result['representatives'])
        merged.update(result.pop('representatives'))
    assert set(merged) == set(faults)
    return {'wall': wall, 'workers': workers, 'batches': len(batches),
            'complete': sum(r['complete'] == '1' for r in merged.values()),
            'cubes': sum(int(r['cube_cnt']) for r in merged.values()),
            'max_child_rss_kib': max(r['rss_kib'] for r in results),
            'sum_child_wall': sum(r['wall'] for r in results),
            'children': results, 'representatives': merged}

def signature(rows):
    return {key: (r['fdp'], r['complete'], r['cube_cnt'], r['seeded_cnt']) for key, r in rows.items()}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repeats', type=int, default=2)
    parser.add_argument('--chunk', type=int, default=128)
    args = parser.parse_args()
    (HERE / 'results').mkdir(exist_ok=True)
    report = {'repeats': args.repeats, 'chunk': args.chunk, 'runs': []}
    # Full-enumeration regression: compare all rows to the fixed golden file.
    c17 = run_case('c17a', 'c17a/golden', limit=0, gt=True)
    def golden_signature(path):
        return sorted((r['net_name'], r['f_type'], r['fdp']) for r in csv.DictReader(path.open()))
    assert golden_signature(HERE / 'runs/c17a/golden/fdp.csv') == golden_signature(ROOT / 'expected/c17a_result.csv')
    c17rows = snapshot('c17a')
    c17parallel = sharded('c17a', c17rows, 'c17a/parallel', 4, 3, gt=True, limit=0)
    assert {k: r['fdp'] for k, r in c17parallel['representatives'].items()} == {k: r['fdp'] for k, r in c17['representatives'].items()}
    report['c17_golden_and_parallel_gt'] = True
    rows = snapshot('s5378_C')
    assert len(rows) == 4551
    reference = None
    for repeat in range(args.repeats):
        baseline = run_case('s5378_C', f's5378_C/r{repeat}/baseline')
        baseline.pop('representatives')
        baseline.update(mode='serial_dom', repeat=repeat)
        report['runs'].append(baseline)
        print(json.dumps({k: v for k, v in baseline.items() if k != 'children'}), flush=True)
        nodom = run_case('s5378_C', f's5378_C/r{repeat}/nodom', nodom=True)
        sig = signature(nodom.pop('representatives'))
        if reference is not None:
            assert sig == reference
        reference = sig
        nodom.update(mode='serial_nodom', repeat=repeat)
        report['runs'].append(nodom)
        print(json.dumps(nodom), flush=True)
        # Reverse order on the second repeat to reduce fixed-order bias.
        for workers in ([1, 2, 4, 8] if repeat % 2 == 0 else [8, 4, 2, 1]):
            result = sharded('s5378_C', rows, f's5378_C/r{repeat}/p{workers}', workers, args.chunk)
            assert signature(result.pop('representatives')) == reference
            result.update(mode=f'batch_nodom_p{workers}', repeat=repeat)
            report['runs'].append(result)
            print(json.dumps({k: v for k, v in result.items() if k != 'children'}), flush=True)
        (HERE / 'results/benchmark.json').write_text(json.dumps(report, indent=2) + '\n')
    verification = sharded('s5378_C', rows, 's5378_C/parallel_gt', 8, args.chunk, gt=True)
    assert signature(verification.pop('representatives')) == reference
    report['s5378_parallel_gt'] = verification
    report['representative_rows_identical_to_serial_nodom'] = True
    modes = sorted({r['mode'] for r in report['runs']})
    report['medians'] = {mode: statistics.median(r['wall'] for r in report['runs'] if r['mode'] == mode)
                         for mode in modes}
    (HERE / 'results/benchmark.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report['medians']), flush=True)

if __name__ == '__main__':
    main()
