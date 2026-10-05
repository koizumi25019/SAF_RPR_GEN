"""Output phase assignment: enumerate the non-detection cover with SAT/DC."""
import importlib.util,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
import blocks
from fractions import Fraction
cases=[('s5378_C','n1592gat',0),('s5378_C','n291gat',1),('s38584_C','g16349',1),('b19_C','P2_P1_P1_U3002',0)]
nets={};results=[]
for circuit,fault,stuck in cases:
 if circuit not in nets:nets[circuit]=blocks.FastNet(blocks.ROOT/'input/circuit'/f'{circuit}.v')
 net=nets[circuit]
 old,out,rows,_,npar,_=blocks.build_fast(net,fault,stuck,difference=True)
 ex,out,active=blocks.compact(old,out)
 prepared=(ex,out^1,[rows[i-1] for i in active],[],npar,set())
 for width in [3,6]:
  prefix=HERE/'runs'/f'{circuit}_{fault}_{stuck}_negative{width}'
  r=blocks.run(net,fault,stuck,prefix,'partition',width,5,prepared,True)
  r['phase']='negative'
  if r['complete']:r['detection_probability']=str(1-Fraction(r['union']['probability']))
  results.append(r);print(json.dumps(r),flush=True)
(HERE/'results/phase_pilot.json').write_text(json.dumps(results,indent=2))
