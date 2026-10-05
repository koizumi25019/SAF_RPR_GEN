"""Refine the new weighted coordinates on the previously difficult fault."""
import json
import subprocess
import time
from weighted import HERE,ROOT,FastNet,prepare,run

path=ROOT/'input/circuit/s38584_C.v';net=FastNet(path)
prepared=prepare(net,'g16349',1);results=[]
for learning,mass in [(0,False),(0,True),(128,False),(512,False),(512,True)]:
    prefix=HERE/'runs/refine'/f'g16349_learn{learning}_mass{int(mass)}'
    t=time.monotonic()
    try:
        r=run(net,'g16349',1,prefix,prepared,seconds=5,learning=learning,mass_order=mass)
        r['manifest']=str(prefix.with_suffix('.cover.json'))
    except subprocess.TimeoutExpired as exc:
        if not str(exc.cmd[0]).endswith('weighted_union'):raise
        r={'fault':'g16349','learning':learning,'mass_order':mass,'complete':False,'cause':'bdd_timeout',
           'seconds':time.monotonic()-t}
    results.append(r);print(json.dumps(r),flush=True)
    (HERE/'results/refine.json').write_text(json.dumps(results,indent=2))
