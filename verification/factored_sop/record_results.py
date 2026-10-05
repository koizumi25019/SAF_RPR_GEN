"""Export compact numerical evidence while keeping bulky intermediates in runs."""
import csv
import json
import pathlib
from fractions import Fraction

HERE = pathlib.Path(__file__).resolve().parent


def main():
    output = HERE/'results'
    output.mkdir(exist_ok=True)
    names = ['s5378_difference', 's5378_complement', 's38584_flat', 's38584_factored',
             's38584_difference', 's38584_all_bounded', 'b19_original16', 'b19_fast16',
             'b19_flat', 'b19_factored', 'b19_difference']
    measured = {}
    for name in names:
        directory = HERE/'runs'/name
        if not (directory/'summary.json').exists():
            continue
        measured[name] = json.loads((directory/'summary.json').read_text())
        rows = list(map(json.loads, (directory/'faults.jsonl').read_text().splitlines()))
        with open(output/(name+'.csv'), 'w', newline='') as fp:
            writer = csv.writer(fp)
            writer.writerow(['net_name', 'f_type', 'complete', 'fdp_fraction', 'value_kind', 'seconds', 'time_kind'])
            for row in rows:
                writer.writerow([row['fault'], f"sa{row['stuck']}", int(row['complete']), row['fdp'],
                                 'exact' if row['complete'] else 'lower_bound', row.get('seconds', row.get('enum_seconds')),
                                 'fault_total' if 'seconds' in row else 'enumeration_only'])
    for name in ['original_preprocess', 'fast_preprocess', 'pilot', 'difference_pilot', 'lean_pilot', 'sweep_pilot']:
        path = HERE/'runs'/(name+'.log')
        if path.exists():
            measured[name] = [json.loads(line) for line in path.read_text().splitlines() if line.startswith('{')]
    measured['c17'] = json.loads((HERE/'runs/c17/summary.json').read_text())
    measured['sweep_regression'] = json.loads((HERE/'runs/sweep_regression/summary.json').read_text())
    (output/'measurements.json').write_text(json.dumps(measured, indent=2))
    old = {(r['net_name'], r['f_type']): Fraction(r['fdp_exact']) for r in csv.DictReader(open(HERE.parent/'linear_scaling/results/s5378_exact.csv'))}
    for name in ['s5378_difference', 's5378_complement']:
        rows = list(csv.DictReader(open(output/(name+'.csv'))))
        assert all(r['complete'] == '1' and Fraction(r['fdp_fraction']) == old[(r['net_name'], r['f_type'])] for r in rows)
    print(json.dumps({'exported_conditions': len([n for n in names if n in measured]), 's5378_exact_crosschecks': True}))


if __name__ == '__main__':
    main()
