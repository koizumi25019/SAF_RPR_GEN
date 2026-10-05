"""SAT/DC cover enumeration over redundant, overlapping local input decoders.

Rudell ERL-86-65 §3.3 explicitly discusses redundant pairings (ab)(ac)(ad).
Keep correlations in the SAT state encoding; count generated regions on the
original independent coordinates, never as independent predicate variables.
"""
import collections,functools,importlib.util,json,pathlib,sys,time
HERE=pathlib.Path(__file__).resolve().parent
ROOT=HERE.parent.parent
sys.path.insert(0,str(HERE.parent/'factored_sop'))
from fast_basis import FastNet,build_fast,compact,probe
from experiment import run_fault,groups_for

# This file is executed as __main__, never imported as plain 'experiment'.
def overlapping_groups(ex,out,width=3):
 reach=ex.reachable(out)
 @functools.cache
 def support(k):
  if k==0:return frozenset()
  if k<=ex.n:return frozenset([k])
  s=set()
  for lit in ex.nodes[k][1]:
   child=support(lit//2)
   if child is None:return None
   s.update(child)
   if len(s)>width:return None
  return frozenset(s)
 scores=collections.Counter()
 for k in sorted(reach):
  s=support(k)
  if s and 2<=len(s)<=width:scores[tuple(sorted(s))]+=1
 base=list(groups_for(ex,out,width));known=set(base)
 # Keep the disjoint baseline expressible, then add bounded redundant decoders.
 additions=[g for g,_ in sorted(scores.items(),key=lambda item:(-item[1],-len(item[0]),item[0])) if g not in known]
 return base+additions[:2*ex.n]

def run(net,fault,stuck,prefix,mode='overlap',width=3,seconds=3,prepared=None,simulate=False):
 t=time.monotonic()
 if prepared is None:
  old,out,rows,_,npar,_=build_fast(net,fault,stuck,difference=True)
  ex,out,active=compact(old,out);rows=[rows[i-1] for i in active]
  prepared=(ex,out,rows,[],npar,set())
 ex,out=prepared[:2]
 groups=overlapping_groups(ex,out,width) if mode=='overlap' else groups_for(ex,out,width)
 result=run_fault(net,fault,stuck,prefix,width=width,seconds=seconds,prepared=prepared,simulate=simulate,
                  groups_override=groups,count_partial=False,union_timeout=10)
 result.update({'fault':fault,'stuck':stuck,'mode':mode,'width':width,'groups':len(groups),
                'overlap_memberships':sum(map(len,groups))-len({v for g in groups for v in g}),
                'total_seconds':time.monotonic()-t})
 pathlib.Path(prefix).with_suffix('.comparison.json').write_text(json.dumps(result,indent=2))
 return result

if __name__=='__main__':
 cases=[('s5378_C','n673gat',0),('s5378_C','n1592gat',0),('s5378_C','n291gat',1),('s38584_C','g16349',1),
        ('b19_C','P2_P1_P1_U3002',0)]
 nets={};allrows=[]
 for circuit,fault,stuck in cases:
  if circuit not in nets:nets[circuit]=FastNet(ROOT/'input/circuit'/f'{circuit}.v')
  net=nets[circuit]
  old,out,rows,_,npar,_=build_fast(net,fault,stuck,difference=True)
  ex,out,active=compact(old,out);prepared=(ex,out,[rows[i-1] for i in active],[],npar,set())
  for mode,width,sim in [('partition',3,True),('partition',3,False),('overlap',3,False),('overlap',6,False)]:
   prefix=HERE/'runs'/f'{circuit}_{fault}_{stuck}_{mode}{width}_sim{int(sim)}'
   r=run(net,fault,stuck,prefix,mode,width,3,prepared,sim)
   allrows.append(r);print(json.dumps(r),flush=True)
 (HERE/'results/pilot.json').write_text(json.dumps(allrows,indent=2))
