"""Golden and independent raw-CNF checks for the common-CNF prototype."""
import csv
import importlib.util
import json
import pathlib
from fractions import Fraction
from factored_cnf_sop import run, FastNet

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent.parent
spec = importlib.util.spec_from_file_location('clausal_verify_local', HERE/'verify.py')
checker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checker)

netpath = ROOT/'input/circuit/c17a.v'
net = FastNet(netpath)
with (ROOT/'expected/c17a_result.csv').open() as file:
    golden = {(r['net_name'], int(r['f_type'][-1])): r['fdp'] for r in csv.DictReader(file)}
results = []
for fault in net.pi+list(net.g):
    for stuck in (0, 1):
        prefix = HERE/'regression'/f'{fault}_{stuck}'
        result = run(netpath, fault, stuck, prefix, width=6, implicate_mode='propagation')
        assert result['complete']
        probability = result['union']['probability']
        assert f'{float(Fraction(probability)):.10e}' == golden[(fault, stuck)]
        results.append({'probability': probability, **checker.verify(netpath, prefix)})
(HERE/'regression/summary.json').write_text(json.dumps(results, indent=2))
print(json.dumps({'verified': len(results), 'golden': True}))
