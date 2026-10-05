"""Independent raw-CNF checks for the completed large-circuit batch."""
import json,pathlib,random
from verify import verify
HERE=pathlib.Path(__file__).resolve().parent
ROOT=HERE.parent.parent
rows=list(map(json.loads,(HERE/'runs/s38584_all_bounded/faults.jsonl').read_text().splitlines()))
complete=[r for r in rows if r['complete']]
chosen=random.Random(20260911).sample(complete,16)
chosen += [r for r in rows if not r['complete']]
results=[]
for r in chosen:
 result=verify(str(ROOT/'input/circuit/s38584_C.v'),ROOT/r['manifest'])
 results.append(result)
 print(json.dumps(result),flush=True)
(HERE/'results/s38584_independent_sample.json').write_text(json.dumps(results,indent=2))
