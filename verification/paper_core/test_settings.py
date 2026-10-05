#!/usr/bin/env python3
"""Check .set/CLI settings, legacy fallback, precedence and rejected values."""
import csv
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'output/paper_core_settings_checks'
OUT.mkdir(parents=True, exist_ok=True)
BASE = dict(os.environ)
for key in ('PAPER_CORE', 'PAPER_CORE_VERIFY', 'MDC_NODOM', 'MAXDC', 'MAXHAM',
            'DUAL', 'SPLIT', 'PCOUNT', 'BDD_EXACT', 'XID_EXTERNAL', 'TDF_NOXID',
            'DIVPO', 'DIVPHASE', 'GT_ISOP', 'GT_GAIN', 'GT_COVER', 'GT_CUBEDUMP',
            'CUBE_TREND', 'AIG_DUMP', 'AIG_DUMP_DIR', 'DUMP_CNF', 'MDC_FAULT_TIMEOUT'):
    BASE.pop(key, None)
BASE['GT_BDD'] = '1'


def run(label, directives='', env_values=None, expected=('XID', 'on', 'off'),
        circuit='c17a', binary='main_release', cli=False):
    stem = OUT / label
    env = dict(BASE, **(env_values or {}))
    inputs = f'-net ../input/circuit/{circuit}.v\n-fdp {stem}.csv\n-log {stem}.log\n'
    path = stem.with_suffix('.set')
    path.write_text(inputs + directives)
    command = [str(ROOT / 'build' / binary)]
    command += (inputs + directives).split() if cli else ['-set', str(path)]
    with stem.with_suffix('.stdout').open('w') as stdout, stem.with_suffix('.stderr').open('w') as stderr:
        subprocess.run(command, cwd=ROOT / 'build', env=env, stdout=stdout,
                       stderr=stderr, check=True, timeout=300)
    assert 'ALL VERIFIED' in stem.with_suffix('.stderr').read_text(), label
    log = stem.with_suffix('.log').read_text()
    for key, value in zip(("Don't-care Method", 'Dominance Cube Reuse', 'CORE Extra Verification'), expected):
        assert any(key in line and line.split(':')[-1].strip() == value for line in log.splitlines()), (label, key)
    with stem.with_suffix('.csv').open() as f:
        rows = list(csv.DictReader(f))
    assert all(r['complete'] in ('', '1') for r in rows), label
    if circuit == 'c17a':
        with (ROOT / 'expected/c17a_result.csv').open() as f:
            golden = {(r['net_name'], r['f_type']): r['fdp'] for r in csv.DictReader(f)}
        assert {(r['net_name'], r['f_type']): r['fdp'] for r in rows} == golden, label
    print(f'{label}: settings reflected, FDP verified', flush=True)
    return rows


for binary in ('main_debug', 'main_release'):
    run('default_' + binary, binary=binary)

legacy = run('legacy_core', env_values={'PAPER_CORE': '1', 'MDC_NODOM': '1'},
             expected=('CORE', 'off', 'off'))
configured = run('set_core', '-dc_method core\n-dom_reuse off\n-core_verify off\n',
                 expected=('CORE', 'off', 'off'))
assert legacy == configured
run('set_core_verified', '-dc_method\tcore\n-dom_reuse off\n-core_verify on\n',
    expected=('CORE', 'off', 'on'))
run('set_overrides_env', '-dc_method xid\n-dom_reuse on\n-core_verify off\n',
    env_values={'PAPER_CORE': '1', 'MDC_NODOM': '1', 'PAPER_CORE_VERIFY': '1'})
run('set_numeric', '-dc_method core\n-dom_reuse 0\n-core_verify 1\n',
    expected=('CORE', 'off', 'on'))
run('cli_core', '-dc_method core\n-dom_reuse off\n-core_verify on\n',
    expected=('CORE', 'off', 'on'), cli=True)
for bad in ('-dc_method unknown', '-dc_method', '-dom_reuse maybe', '-core_verify'):
    stem = OUT / ('invalid_' + bad.replace(' ', '_'))
    path = stem.with_suffix('.set')
    path.write_text(bad + '\n')
    result = subprocess.run([str(ROOT / 'build/main_release'), '-set', str(path)],
                            cwd=ROOT / 'build', env=BASE, capture_output=True, text=True)
    assert result.returncode == 1 and 'requires' in result.stderr, bad

for circuit in ('s208_C', 's298_C'):
    a = run(circuit + '_xid', '-dc_method xid\n-dom_reuse off\n-core_verify off\n',
            expected=('XID', 'off', 'off'), circuit=circuit)
    b = run(circuit + '_core', '-dc_method core\n-dom_reuse off\n-core_verify off\n',
            expected=('CORE', 'off', 'off'), circuit=circuit)
    assert {(r['net_name'], r['f_type']): r['fdp'] for r in a} == \
           {(r['net_name'], r['f_type']): r['fdp'] for r in b}
print('PASS: .set/CLI selection, explicit precedence, legacy compatibility, errors and FDP')
