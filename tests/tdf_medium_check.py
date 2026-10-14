#!/usr/bin/env python3
"""Verify s5378 TDF sample covers with the isolated verification-branch BDD oracle.

First run python3 tests/fault_pool_build.py --checker.
The production executable does not contain the capture hook or BDD oracle.
"""
import csv
import json
import os
from pathlib import Path
import re
import subprocess
from tdf_check import ROOT, run, equal


def main():
    output = ROOT / 'build/tdf_medium_checks'
    binary = ROOT / 'build/main_release'
    capture = ROOT / 'build/fault_pool_checks/main_capture'
    checker = ROOT / 'build/fault_pool_checks/check_covers'
    circuit = ROOT / 'input/circuit/s5378.v'
    extra = f'-fault {ROOT}/input/fault/s5378_tdf_sample.txt\n-limit 30\n'
    serial = run(binary, output / 'serial', circuit, extra)
    parallel = run(capture, output / 'parallel', circuit, extra + '-jobs 4\n', capture=True)
    equal(serial, parallel)
    rows = list(csv.DictReader((serial / 'fdp.csv').open()))
    assert len(rows) == 150
    # Same defined model and sample in the reference implementation.
    reference = run(checker, output / 'reference', circuit, extra)
    refs = {(r['net_name'], r['f_type']): r for r in csv.DictReader((reference / 'fdp.csv').open())}
    assert len(refs) == len(rows)
    compared = 0
    for row in rows:
        ref = refs[row['net_name'], row['f_type']]
        if row['complete'] == ref['complete'] == '1':
            assert row['fdp'] == ref['fdp'], (row, ref)
            compared += 1
    env = dict(os.environ, BASELINE_COVER_DIR=str(parallel), GT_BDD='1')
    gt_config = output / 'gt.set'
    gt_config.write_text((reference / 'run.set').read_text() +
                         f'-fdp {output}/gt_unused.csv\n-log {output}/gt.log\n')
    result = subprocess.run([str(checker), '-set', str(gt_config)],
                            env=env, cwd=ROOT / 'build', capture_output=True, text=True,
                            check=True, timeout=180)
    (output / 'gt.stderr.txt').write_text(result.stderr)
    assert re.search(r'checked=150\s+UNSOUND=0\s+complete-but-NOT-exact=0\s+ALL VERIFIED', result.stderr)
    report = {'circuit': 's5378', 'faults': 150, 'limit': 30,
              'serial_parallel_csv_identical': True, 'oracle_commit': 'bf8faa8c',
              'gt': 'ALL VERIFIED', 'reference_complete_fdp_matches': compared,
              'completed': sum(r['complete'] == '1' for r in rows)}
    (output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
