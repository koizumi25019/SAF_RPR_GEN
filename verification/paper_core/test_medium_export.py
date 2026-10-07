#!/usr/bin/env python3
"""Exercise partial-FDP handling using a completed medium XID CSV as a fixture."""
import argparse
import contextlib
import csv
from decimal import Decimal
import importlib.util
import io
from pathlib import Path
import re
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--run-dir', type=Path, required=True)
args = parser.parse_args()
source = args.run_dir.resolve()
spec = importlib.util.spec_from_file_location('export_core_only', Path(__file__).with_name('export_core_only.py'))
module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
with (source / 's5378_C_xid_1.csv').open() as fp:
    original = list(csv.DictReader(fp))
fields = list(original[0])
partial = next(i for i, row in enumerate(original) if row['complete'] == '0')
complete = next(i for i, row in enumerate(original) if row['complete'] == '1')

with tempfile.TemporaryDirectory(prefix='core_only_export_') as temp:
    root = Path(temp); out = root / 'run'; out.mkdir()
    (root / 'verification/paper_core/results').mkdir(parents=True)
    for name in ('source_parent.txt', 'binary.sha256', 'cpu.txt'):
        (out / name).write_bytes((source / name).read_bytes())
    for name, value in [('profile', 'medium'), ('limit', '30'), ('repeats', '1'), ('warmups', '0'), ('verification', 'off')]:
        (out / (name + '.txt')).write_text(value)
    module.ROOT = root
    def write(mode, rows):
        stem = out / f's5378_C_{mode}_1'
        with stem.with_suffix('.csv').open('w', newline='') as fp:
            writer = csv.DictWriter(fp, fieldnames=fields); writer.writeheader(); writer.writerows(rows)
        log = (source / 's5378_C_xid_1.log').read_text()
        values = {"Don't-care Method": 'XID' if mode == 'xid' else 'CORE',
                  'CORE Minimization': 'off' if mode == 'core_only' else 'on',
                  'CORE Recheck': 'off' if mode == 'core_only' else 'on'}
        for key, value in values.items():
            log = re.sub(r'(//\s+' + re.escape(key) + r'\s+: )\w+', r'\g<1>' + value, log)
        stem.with_suffix('.log').write_text(log)
        cubes = sum(int(r['cube_cnt']) for r in rows if r['complete'] in ('0', '1'))
        bits = 214 * cubes
        if mode == 'xid': stderr = ''
        else:
            count = cubes if mode == 'core_only' else 2 * cubes + bits
            enabled = 'off' if mode == 'core_only' else 'on'
            deleted = 0 if mode == 'core_only' else bits
            stderr = f'[PAPER_CORE] cubes={cubes} solves={count} care: input={bits} core={bits} prime={bits}\n'
            stderr += f'[PAPER_CORE_MODE] minimize={enabled} recheck={enabled} deletion_queries={deleted} final_care={bits}\n'
        stem.with_suffix('.stderr').write_text(stderr)
        (out / (stem.name + '.timing.csv')).write_text('cpu_s,user_s,system_s,wall_s,exit_code\n1,1,0,1,0\n')
    def export():
        import sys
        saved = sys.argv; sys.argv = ['export', '--run-dir', str(out)]
        try:
            with contextlib.redirect_stdout(io.StringIO()): module.main()
        finally: sys.argv = saved
    for mode in module.MODES: write(mode, original)
    changed = [dict(row) for row in original]
    changed[partial]['fdp'] = str(Decimal(changed[partial]['fdp']) + Decimal('0.00001'))
    write('core_only', changed); export()
    print('PASS: partial FDP differences accepted; completed FDP equal')
    for case in ('completed_mismatch', 'missing_fault', 'cube_limit_exceeded', 'partial_above_exact'):
        changed = [dict(row) for row in original]
        if case == 'completed_mismatch':
            changed[complete]['fdp'] = str(Decimal(changed[complete]['fdp']) + Decimal('0.00001'))
        elif case == 'missing_fault':
            changed[complete]['net_name'] = '__missing_fault__'
        elif case == 'cube_limit_exceeded':
            changed[partial]['cube_cnt'] = '31'
        else:
            changed[complete]['complete'] = '0'
            changed[complete]['cube_cnt'] = '30'
            changed[complete]['fdp'] = str(Decimal(changed[complete]['fdp']) + Decimal('0.00001'))
        write('core_only', changed)
        try: export()
        except AssertionError: print('PASS: rejected ' + case)
        else: raise AssertionError('Export wrongly accepted ' + case)
