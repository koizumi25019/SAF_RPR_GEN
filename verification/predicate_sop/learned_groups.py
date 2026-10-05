"""Cover-guided decoder assignment, inspired by Rudell §3.3 / Sasao pairing.

Estimate merge benefit from pairs of pilot PI cubes differing in <=3 variable
positions. Select disjoint decoder groups greedily. This is a bounded heuristic,
not Espresso's exact matching or full minimization of every candidate pairing.
"""
import collections,json,pathlib,sys,time,subprocess
HERE=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
import blocks

def choose(ex,out,cubes,width=3):
 encoded=[]
 for cube in cubes:
  zero=one=0
  for l in cube:
   if l>0:one |= 1<<(l-1)
   else:zero |= 1<<(-l-1)
  encoded.append((zero,one))
 benefit=collections.Counter()
 for i,(z,o) in enumerate(encoded):
  for zz,oo in encoded[:i]:
   delta=(z^zz)|(o^oo)
   if 2<=delta.bit_count()<=width:
    group=tuple(j+1 for j in range(ex.n) if delta>>j&1)
    benefit[group]+=1
 remaining={i for i in ex.reachable(out) if i<=ex.n};groups=[]
 for group,score in sorted(benefit.items(),key=lambda x:(-x[1],len(x[0]),x[0])):
  if set(group)<=remaining:groups.append(group);remaining.difference_update(group)
 for group in blocks.groups_for(ex,out,width):
  g=tuple(v for v in group if v in remaining)
  if g:groups.append(g);remaining.difference_update(g)
 assert not remaining
 return groups,[(list(g),n) for g,n in benefit.most_common()]

def run(net,fault,stuck,prefix,prepared,seconds=3,pilot_limit=128,union_order='natural'):
 prefix=pathlib.Path(prefix);t=time.monotonic()
 pilot=blocks.run_fault(net,fault,stuck,prefix.with_name(prefix.name+'_pilot'),width=1,seconds=.5,limit=pilot_limit,prepared=prepared,simulate=True,count_partial=False)
 pilotprefix=prefix.with_name(prefix.name+'_pilot')
 cubes=json.loads(pilotprefix.with_suffix('.cubes.json').read_text())
 ex,out=prepared[:2]
 groups,benefits=choose(ex,out,cubes)
 selected=time.monotonic()-t
 try:
  result=blocks.run_fault(net,fault,stuck,prefix,width=3,seconds=seconds,prepared=prepared,simulate=True,
                           groups_override=groups,count_partial=False,union_timeout=10,union_order=union_order)
 except subprocess.TimeoutExpired as exc:
  if not str(exc.cmd[0]).endswith('region_union'):raise
  result=json.loads(prefix.with_suffix('.result.json').read_text())
  result.update({'enumeration_complete':result['complete'],'complete':False,'union':None,'cause':'bdd_timeout'})
  prefix.with_suffix('.metadata.json').write_text(json.dumps({'fault':fault,'stuck':stuck,'npi':ex.n,
   'basis_rows':[hex(r) for r in prepared[2]],'pi_order':net.pi,'groups':groups,'status':'count_pending'},indent=2))
 result.update({'fault':fault,'stuck':stuck,'mode':'learned','width':3,'groups':len(groups),
                'union_order':union_order,
                'pilot_cubes':len(cubes),'pilot_complete':pilot['complete'],'pilot_seconds':selected,
                'scored_groups':len(benefits),'total_seconds':time.monotonic()-t})
 prefix.with_suffix('.pairing.json').write_text(json.dumps({'groups':groups,'benefits':benefits},indent=2))
 prefix.with_suffix('.comparison.json').write_text(json.dumps(result,indent=2))
 return result

if __name__=='__main__':
 cases=[('s5378_C','n673gat',0),('s5378_C','n1592gat',0),('s5378_C','n291gat',1),('s38584_C','g16349',1),('b19_C','P2_P1_P1_U3002',0)]
 nets={};results=[]
 for circuit,fault,stuck in cases:
  if circuit not in nets:nets[circuit]=blocks.FastNet(blocks.ROOT/'input/circuit'/f'{circuit}.v')
  net=nets[circuit]
  old,out,rows,_,npar,_=blocks.build_fast(net,fault,stuck,difference=True)
  ex,out,active=blocks.compact(old,out);prepared=(ex,out,[rows[i-1] for i in active],[],npar,set())
  for size in [128,512]:
   r=run(net,fault,stuck,HERE/'runs'/f'{circuit}_{fault}_{stuck}_learned{size}',prepared,pilot_limit=size)
   results.append(r);print(json.dumps(r),flush=True)
 (HERE/'results/learned_pilot.json').write_text(json.dumps(results,indent=2))
