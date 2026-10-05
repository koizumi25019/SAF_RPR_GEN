import json
from predicates import HERE,ROOT,FastNet,prepare,run
cases=[('s5378_C','n673gat',0),('s5378_C','n1592gat',0),('s5378_C','n291gat',1),('s38584_C','g16349',1),('b19_C','P2_P1_P1_U3002',0)]
nets={};results=[]
for circuit,fault,stuck in cases:
 if circuit not in nets:nets[circuit]=FastNet(ROOT/'input/circuit'/f'{circuit}.v')
 net=nets[circuit];prepared=prepare(net,fault,stuck)
 for width in [3,6]:
  r=run(net,fault,stuck,HERE/'runs'/f'{circuit}_{fault}_{stuck}_frontier{width}',width,3,prepared,True)
  results.append(r);print(json.dumps(r),flush=True)
(HERE/'results/frontier_pilot.json').write_text(json.dumps(results,indent=2))
