"""Run selected real faults, or alternate completed cases for three repeats."""
import argparse
import json
import statistics
from ffr import HERE, ROOT, run

parser = argparse.ArgumentParser()
parser.add_argument('--repeat', action='store_true')
args = parser.parse_args()
cases = [('n2428gat', 0), ('n2432gat', 0)] if args.repeat else [
    ('n1592gat', 0), ('n2428gat', 0), ('n2432gat', 0), ('n673gat', 0), ('n291gat', 1)]
rows = []
for iteration in range(3 if args.repeat else 1):
    for fault, stuck in cases:
        for flat in ([False, True] if iteration % 2 == 0 else [True, False]):
            directory = 'repeats' if args.repeat else 'runs'
            label = f'{fault}_{iteration}' if args.repeat else f's5378_{fault}_{stuck}'
            prefix = HERE/directory/(label+('_flat' if flat else '_ffr'))
            result = run(ROOT/'input/circuit/s5378_C.v', fault, stuck, prefix,
                         3 if args.repeat else 2, flat=flat)
            if args.repeat:
                assert result['complete']
            rows.append(result)
            print(json.dumps({k: v for k, v in result.items() if k != 'factor_results'}), flush=True)
(HERE/'results').mkdir(exist_ok=True)
(HERE/'results'/('repeats.json' if args.repeat else 'real.json')).write_text(json.dumps(rows, indent=2))
if args.repeat:
    for fault, _ in cases:
        print(fault, [(flat, statistics.median(r['total_seconds_excluding_raw_proof']
              for r in rows if r['fault'] == fault and r['flat'] == flat)) for flat in [False, True]])
