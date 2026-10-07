#!/usr/bin/env python3
"""Check default regressions and core-only controls before benchmarking."""
import csv
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'output/core_only_regression'
OUT.mkdir(parents=True, exist_ok=True)
hooks = set()
for source in (ROOT / 'src').rglob('*.c'):
    hooks.update(re.findall(r'getenv\("([A-Z0-9_]+)"\)', source.read_text(errors='replace')))
ENV = {k: v for k, v in os.environ.items() if k not in hooks}
ENV['GT_BDD'] = '1'
checks = []


def load(path):
    with path.open() as fp:
        return list(csv.DictReader(fp))


def run(label, source, extra='', binary='main_release', cli=False):
    stem = OUT / label
    inputs = '\n'.join(line for line in source.read_text().splitlines()
                       if line.split() and line.split()[0] in ('-net', '-fault', '-limit', '-saf', '-tdf'))
    text = inputs + f'\n-fdp {stem}.csv\n-log {stem}.log\n' + extra
    stem.with_suffix('.set').write_text(text)
    command = [str(ROOT / 'build' / binary)]
    command += text.split() if cli else ['-set', str(stem.with_suffix('.set'))]
    with stem.with_suffix('.stdout').open('w') as stdout, stem.with_suffix('.stderr').open('w') as stderr:
        subprocess.run(command, cwd=ROOT / 'build', env=ENV, stdout=stdout, stderr=stderr, check=True, timeout=600)
    err = stem.with_suffix('.stderr').read_text()
    assert '[GT] summary:' in err and 'ALL VERIFIED' in err, (label, err[-2000:])
    rows = load(stem.with_suffix('.csv'))
    assert all(r['complete'] in ('1', '') for r in rows), label
    checks.append(dict(case=label, BDD='ALL VERIFIED', representatives=sum(r['complete'] == '1' for r in rows)))
    print(label + ': ALL VERIFIED', flush=True)
    return rows, err


subprocess.run([str(ROOT / 'build/test_generalize')], check=True)
golden = {(r['net_name'], r['f_type']): r['fdp'] for r in load(ROOT / 'expected/c17a_result.csv')}
source = ROOT / 'input/script/c17a.set'
for binary in ('main_debug', 'main_release'):
    rows, _ = run('c17_default_' + binary, source, binary=binary)
    assert {(r['net_name'], r['f_type']): r['fdp'] for r in rows} == golden
for circuit in ('s27', 's208', 's298', 's344', 's510', 's641', 's713', 's1494'):
    run(circuit + '_default', ROOT / f'verification/gt_bdd/{circuit}.set')

for minimize in ('off', 'on'):
    for recheck in ('off', 'on'):
        extra = f'-dc_method core\n-dom_reuse off\n-core_minimize {minimize}\n-core_recheck {recheck}\n-core_verify off\n'
        rows, err = run(f'c17_core_{minimize}_{recheck}', source, extra)
        assert {(r['net_name'], r['f_type']): r['fdp'] for r in rows} == golden
        match = re.search(r'cubes=(\d+) solves=(\d+) care: input=(\d+) core=(\d+) prime=(\d+)', err)
        cubes, solves, _, core, final = map(int, match.groups())
        expected = cubes * (1 + (recheck == 'on')) + (core if minimize == 'on' else 0)
        assert solves == expected
        if minimize == 'off': assert core == final
run('c17_raw_verified_debug', source,
    '-dc_method core\n-dom_reuse off\n-core_minimize off\n-core_recheck off\n-core_verify on\n', binary='main_debug')
run('c17_raw_cli', source, '-dc_method core\n-core_minimize 0\n-core_recheck 0\n', cli=True)
for bad in ('-core_minimize maybe', '-core_minimize', '-core_recheck bad', '-core_recheck'):
    path = OUT / 'invalid.set'; path.write_text(bad + '\n')
    result = subprocess.run([str(ROOT / 'build/main_release'), '-set', str(path)], env=ENV, capture_output=True, text=True)
    assert result.returncode == 1 and 'requires' in result.stderr, bad
result = dict(truth_tables=256, mode_combinations=4, detecting_minterms=4096,
              checks=checks, invalid_options_checked=4, c17_golden='MATCH', default_BDD_circuits=9)
(OUT / 'summary.json').write_text(json.dumps(result, indent=2) + '\n')
(ROOT / 'verification/paper_core/results/core_only_regression.json').write_text(json.dumps(result, indent=2) + '\n')
print('PASS: golden, nine default BDD circuits, core control combinations, raw verified cubes and rejected options')
