import json
from pathlib import Path
from learned_groups import blocks,run,HERE,choose
# Preserve the completed enumeration whose counting timed out during the first pilot.
prefix=HERE/'runs/s38584_C_g16349_1_learned512'
r=json.loads(prefix.with_suffix('.result.json').read_text())
r.update({'fault':'g16349','stuck':1,'mode':'learned','width':3,'pilot_cubes':512,'enumeration_complete':True,
          'complete':False,'union':None,'cause':'bdd_timeout','total_seconds':None})
prefix.with_suffix('.comparison.json').write_text(json.dumps(r,indent=2))
net=blocks.FastNet(blocks.ROOT/'input/circuit/b19_C.v')
old,out,rows,_,npar,_=blocks.build_fast(net,'P2_P1_P1_U3002',0,difference=True)
ex,out,active=blocks.compact(old,out);prepared=(ex,out,[rows[i-1] for i in active],[],npar,set())
for size in [128,512]:
 r=run(net,'P2_P1_P1_U3002',0,HERE/'runs'/f'b19_C_P2_P1_P1_U3002_0_learned{size}',prepared,pilot_limit=size)
 print(json.dumps(r),flush=True)
results=[json.loads(p.read_text()) for p in sorted((HERE/'runs').glob('*_learned*.comparison.json'))]
(HERE/'results/learned_pilot.json').write_text(json.dumps(results,indent=2))
