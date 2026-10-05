"""Compare insertion orders on the same completed, already-generated cover."""
import json
import pathlib
import subprocess
import time
from fractions import Fraction
from weighted import HERE,weighted_union

prefix=HERE/'runs/pilot/s38584_C_g16349_1_k12'
manifest=json.loads(prefix.with_suffix('.cover.json').read_text())
top=pathlib.Path(manifest['top_prefix'])
meta=json.loads(top.with_suffix('.metadata.json').read_text())
weights=[Fraction(s['probability']) for s in manifest['sources']]
results=[]
for order in ['weighted','uniform','natural']:
    t=time.monotonic()
    try:
        r=weighted_union(top,len(weights),meta['groups'],weights,manifest['ncoords'],timeout=10,order=order,
                         result_path=prefix.with_suffix(f'.recount_{order}.json'))
        assert r['probability']==manifest['probability']
        r.update(complete=True)
    except subprocess.TimeoutExpired:r={'complete':False,'timeout':10}
    r.update(order=order,wall_seconds=time.monotonic()-t)
    results.append(r);print(json.dumps(r),flush=True)
    (HERE/'results/recount.json').write_text(json.dumps(results,indent=2))
