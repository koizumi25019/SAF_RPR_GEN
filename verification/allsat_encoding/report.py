"""Export one comparison table; distinguish incomplete enumeration from exact FDP."""
import csv
import json
import platform
import subprocess
from run import HERE

rows = []
for source in ['pilot', 'split_pilot', 'tabular_pilot']:
    for data in json.loads((HERE/'results'/f'{source}.json').read_text()):
        row = {k:data.get(k) for k in ['fault','stuck','encoding','method','seed','width',
            'cubes','complete','seconds','total_seconds','cnf_vars','cnf_clauses']}
        row.update(source=source, probability=(data.get('union') or {}).get('probability'),
                   sound=(data.get('certificate') or {}).get('sound'),
                   exact=(data.get('certificate') or {}).get('exact'))
        rows.append(row)
with (HERE/'results/comparison.csv').open('w') as file:
    writer = csv.DictWriter(file, list(rows[0]))
    writer.writeheader();writer.writerows(rows)
environment = {'platform':platform.platform(),'python':platform.python_version(),
               'compiler':subprocess.check_output(['g++','--version'],text=True).splitlines()[0],
               'timing':'exploratory single runs, some checks overlapped; seconds excludes cover BDD',
               'cases':len(rows),'completed':sum(r['complete'] for r in rows),
               'raw_verified_completed':sum(bool(r['exact']) for r in rows)}
(HERE/'results/environment.json').write_text(json.dumps(environment,indent=2))
print(json.dumps(environment,indent=2))
