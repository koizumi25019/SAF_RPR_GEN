"""Ablation: separate positive and negative PG CNFs for the two SAT roles."""
import csv
import json
from fractions import Fraction
from run import HERE, ROOT, FastNet, certificate, prepare, run
from expand_connections import expand

path = expand(ROOT/'input/circuit/c17a.v', HERE/'runs/split_c17/expanded.v')
net = FastNet(path)
tests = []
for row in csv.DictReader((ROOT/'expected/c17a_result.csv').open()):
    fault, stuck = row['net_name'], int(row['f_type'][-1])
    prepared = prepare(net, fault, stuck)
    for width in [1,3]:
        prefix = HERE/'runs/split_c17'/f'{fault}_{stuck}_w{width}'
        r = run(net,fault,stuck,prefix,prepared,'nnf_pg_split',width=width)
        assert r['complete']
        assert f"{float(Fraction(r['union']['probability'])):.10e}"==row['fdp']
        r['certificate']=certificate(path,prefix)
        tests.append(r)
(HERE/'results/split_c17.json').write_text(json.dumps(tests,indent=2))
print(json.dumps({'c17':len(tests),'verified':True}),flush=True)
nets={};results=[]
for circuit,fault,stuck in [('s5378_C','n673gat',0),('s5378_C','n1592gat',0),
                           ('s5378_C','n291gat',1),('s38584_C','g16349',1),
                           ('b19_C','P2_P1_P1_U3002',0)]:
    path=ROOT/'input/circuit'/f'{circuit}.v'
    if circuit not in nets:nets[circuit]=FastNet(path)
    net=nets[circuit];prepared=prepare(net,fault,stuck)
    for width in [1,3]:
        prefix=HERE/'runs/split_pilot'/f'{circuit}_{fault}_{stuck}_w{width}'
        r=run(net,fault,stuck,prefix,prepared,'nnf_pg_split',width=width)
        if r['complete']:r['certificate']=certificate(path,prefix)
        results.append(r)
        print(json.dumps({k:r[k] for k in ['fault','width','complete','cubes','seconds']}),flush=True)
        (HERE/'results/split_pilot.json').write_text(json.dumps(results,indent=2))
