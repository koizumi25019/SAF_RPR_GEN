"""Check whether the selected s38584 heuristic transfers to other hard faults."""
import importlib.util
import json
import subprocess
import time
from fractions import Fraction
from evaluate import previous
from weighted import HERE, ROOT, FastNet, prepare, run

results=[];nets={}
for circuit,fault,stuck in [('s5378_C','n673gat',0),('s5378_C','n1592gat',0),
                           ('s5378_C','n291gat',1),('b19_C','P2_P1_P1_U3002',0)]:
    if circuit not in nets:nets[circuit]=FastNet(ROOT/'input/circuit'/f'{circuit}.v')
    net=nets[circuit];prepared=prepare(net,fault,stuck)
    for method in ['previous','weighted']:
        prefix=HERE/'runs/scope'/f'{circuit}_{fault}_{stuck}_{method}'
        prefix.parent.mkdir(parents=True,exist_ok=True);t=time.monotonic()
        try:
            if method=='weighted':
                r=run(net,fault,stuck,prefix,prepared,seconds=3,learning=512,mass_order=True)
                elapsed=r['seconds'];cubes=r.get('top_cubes');probability=r.get('probability')
            else:
                r=previous.run(net,fault,stuck,prefix,prepared,seconds=3,pilot_limit=512,union_order='large')
                elapsed=r['total_seconds'];cubes=r['cubes'];probability=(r['union'] or {}).get('probability')
            row={'method':method,'complete':r['complete'],'seconds':elapsed,'top_cubes':cubes,
                 'probability':probability,'selected':r.get('selected')}
        except subprocess.TimeoutExpired as exc:
            if not str(exc.cmd[0]).endswith('weighted_union'):raise
            row={'method':method,'complete':False,'cause':'bdd_timeout','seconds':time.monotonic()-t}
        row.update(circuit=circuit,fault=fault,stuck=stuck,prefix=str(prefix))
        results.append(row);print(json.dumps(row),flush=True)
        (HERE/'results/scope.json').write_text(json.dumps(results,indent=2))
    a,b=results[-2:]
    if a['complete'] and b['complete']:assert Fraction(a['probability'])==Fraction(b['probability'])

# Proof costs are outside the measured comparison.
spec=importlib.util.spec_from_file_location('scope_proof',HERE/'verify.py')
proof=importlib.util.module_from_spec(spec);spec.loader.exec_module(proof)
checks=[]
for r in results:
    if r['method']=='weighted' and r['complete']:
        checked=proof.certificate(ROOT/'input/circuit'/f"{r['circuit']}.v",r['prefix']+'.cover.json')
        checks.append(checked);print(json.dumps(checked),flush=True)
(HERE/'results/scope_verification.json').write_text(json.dumps(checks,indent=2))
