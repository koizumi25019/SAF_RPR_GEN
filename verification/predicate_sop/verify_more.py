import importlib.util,json,pathlib,random,sys
HERE=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
import predicates,blocks
spec=importlib.util.spec_from_file_location('new_verify',HERE/'verify.py')
v=importlib.util.module_from_spec(spec);spec.loader.exec_module(v)
results=[]
# Explicit proof of the completed negative-phase cover, as well as local predicates.
for circuit,fault,stuck,mode in [('s5378_C','n1592gat',0,'negative3'),('s5378_C','n673gat',0,'predicates3'),
                               ('s5378_C','n673gat',0,'learned512')]:
 prefix=HERE/'runs'/f'{circuit}_{fault}_{stuck}_{mode}'
 r=v.certificate(predicates.ROOT/'input/circuit'/f'{circuit}.v',prefix,mode.startswith('negative'))
 results.append(r)
# New frontier selection must itself entail the circuit; validate every golden fault.
netpath=HERE/'runs/c17/expanded.v';net=predicates.FastNet(netpath)
import csv
from fractions import Fraction
for row in csv.DictReader(open(predicates.ROOT/'expected/c17a_result.csv')):
 fault=row['net_name'];stuck=int(row['f_type'][-1]);prefix=HERE/'runs/c17'/f'{fault}_{stuck}_frontier'
 r=predicates.run(net,fault,stuck,prefix,3,3,frontier=True)
 assert r['complete'] and f"{float(Fraction(r['union']['probability'])):.10e}"==row['fdp']
 results.append(v.certificate(netpath,prefix))
# Verify sampled phase-selected factored covers independently, including complemented leaves.
def negated(tree):return tree.get('negated',False) or any(negated(c) for c in tree.get('children',[]))
batch=HERE/'runs/s5378_phase';rows=list(map(json.loads,(batch/'faults.jsonl').read_text().splitlines()))
candidates=[]
for r in rows:
 m=json.loads(pathlib.Path(r['manifest']).read_text())
 if negated(m['tree']):candidates.append(r)
for r in random.Random(20260912).sample(candidates,min(16,len(candidates))):
 results.append(v.rawverify.verify(str(batch/'expanded.v'),r['manifest']))
(HERE/'results/additional_verification.json').write_text(json.dumps(results,indent=2))
print(json.dumps({'checks':len(results),'all_raw_verified':True}))
