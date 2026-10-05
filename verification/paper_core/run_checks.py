#!/usr/bin/env python3
"""Run independent CORE and BDD regression checks with isolated output files.
Build main_debug/main_release first; this script never updates golden files.
"""
import csv
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
RESULTS = ROOT / 'output/paper_core_checks'
RESULTS.mkdir(parents=True, exist_ok=True)


def load_csv(path):
    with path.open(encoding='utf-8-sig') as f:
        return {(r['net_name'], r['f_type']): r for r in csv.DictReader(f)}


def run(name, source, core, nodom=False, binary='main_release', verify=True):
    mode = 'core_nodom' if nodom else 'core' if core else 'xid'
    label = f'{name}_{mode}_{binary}'
    csv_path = RESULTS / f'{label}.csv'
    set_path = RESULTS / f'{label}.set'
    text = source.read_text()
    # Retain only input/model/limit directives, then assign isolated outputs.
    text = '\n'.join(line for line in text.splitlines()
                     if line.split() and line.split()[0] in ('-net', '-fault', '-limit', '-saf', '-tdf'))
    set_path.write_text(text + f'\n-fdp {csv_path}\n-log {RESULTS / (label + ".log")}\n')
    env = {k: v for k, v in os.environ.items() if k not in (
        'PAPER_CORE', 'PAPER_CORE_VERIFY', 'MAXDC', 'MAXDC_CORE', 'MAXDC_QX',
        'XID_EXTERNAL', 'TDF_NOXID', 'DUAL', 'SPLIT', 'PCOUNT', 'BDD_EXACT',
        'MAXHAM', 'DIVPO', 'MDC_NODOM', 'MDC_NOEA', 'MDC_NOPROP',
        'AIG_DUMP', 'AIG_DUMP_DIR', 'DUMP_CNF', 'MDC_FAULT_TIMEOUT')}
    env['GT_BDD'] = '1'
    if core:
        env['PAPER_CORE'] = '1'
        if verify:
            env['PAPER_CORE_VERIFY'] = '1'
    if nodom:
        env['MDC_NODOM'] = '1'
    with (RESULTS / (label + '.stdout')).open('w') as out, \
            (RESULTS / (label + '.stderr')).open('w') as err:
        subprocess.run([str(ROOT / 'build' / binary), '-set', str(set_path)],
                       cwd=ROOT / 'build', env=env, stdout=out, stderr=err, check=True, timeout=600)
    stderr = (RESULTS / (label + '.stderr')).read_text()
    assert '[GT] summary:' in stderr and 'ALL VERIFIED' in stderr, stderr[-4000:]
    rows = load_csv(csv_path)
    if core:
        assert '[PAPER_CORE] cubes=' in stderr
    summaries = re.findall(r'^\[(?:GT|PAPER_CORE)\].*$', stderr, re.M)
    print(label + ': ' + ' | '.join(summaries[-2:]), flush=True)
    return rows


def compare_exact(a, b):
    assert a.keys() == b.keys()
    for key in a:
        # Equivalent-fault echo rows have empty cube_cnt/complete fields.
        assert a[key]['complete'] == b[key]['complete'], key
        assert a[key]['complete'] in ('', '1'), key
        assert a[key]['fdp'] == b[key]['fdp'], (key, a[key], b[key])


def main():
    env = dict(os.environ, PAPER_CORE='1', MAXDC='1')
    result = subprocess.run([str(ROOT / 'build/main_release'), '-set',
                             '../input/script/c17a.set'], cwd=ROOT / 'build',
                            env=env, capture_output=True, text=True, timeout=10)
    assert result.returncode == 1 and 'cannot combine with MAXDC' in result.stderr
    subprocess.run([
        'gcc', '-std=gnu11', '-O2', '-fcommon', '-ffunction-sections', '-fdata-sections',
        '-I' + str(ROOT / 'src/fdp'), '-I' + str(ROOT / 'external/cadical/src'),
        str(ROOT / 'verification/paper_core/test_generalize.c'), str(ROOT / 'src/fdp/paper_core.c'),
        str(ROOT / 'external/cadical/build/libcadical.a'), '-Wl,--gc-sections', '-lstdc++', '-lm',
        '-o', str(RESULTS / 'test_generalize')], check=True)
    subprocess.run([str(RESULTS / 'test_generalize')], check=True)

    golden = load_csv(ROOT / 'expected/c17a_result.csv')
    for binary in ('main_debug', 'main_release'):
        for core in (False, True):
            compare_exact(run('c17a', ROOT / 'input/script/c17a.set', core, binary=binary), golden)
    compare_exact(run('c17a', ROOT / 'input/script/c17a.set', True, nodom=True), golden)

    for circuit in ('s27', 's208', 's298', 's344', 's510', 's641', 's713', 's1494'):
        source = ROOT / f'verification/gt_bdd/{circuit}.set'
        compare_exact(run(circuit, source, False), run(circuit, source, True))

    # Medium circuit with limit: partial FDP may differ between methods. GT checks
    # soundness of every partial cover and exactness for every completed fault.
    run('s5378', ROOT / 'verification/gt_bdd/s5378.set', False)
    # Small/medium complete runs already check every prime. Avoid duplicating
    # all deletion queries on the large limited run; independent GT remains on.
    run('s5378', ROOT / 'verification/gt_bdd/s5378.set', True, verify=False)
    for circuit in ('s27', 's208'):
        source = ROOT / f'input/script/{circuit}_tdf_auto.set'
        compare_exact(run(circuit + '_tdf', source, False), run(circuit + '_tdf', source, True))
    print('PASS: golden values, exact XID/CORE agreement, and all BDD regression checks', flush=True)


if __name__ == '__main__':
    main()
