"""Entailing SOP over bounded local Boolean predicates plus original PI bits.

Only emitted predicate conjunctions go to the cover BDD. Local predicate truth
relations have at most six inputs; the circuit detection BDD is never built.
"""
import functools,json,pathlib,subprocess,sys,time
HERE=pathlib.Path(__file__).resolve().parent
ROOT=HERE.parent.parent
sys.path.insert(0,str(HERE.parent/'factored_sop'))
from fast_basis import FastNet,build_fast,compact,probe
from experiment import write_cnf,count_union

def select(ex,out,width,frontier=False):
 reach=ex.reachable(out)
 @functools.cache
 def support(k):
  if not k:return frozenset()
  if k<=ex.n:return frozenset([k])
  s=set()
  for lit in ex.nodes[k][1]:
   child=support(lit//2)
   if child is None:return None
   s.update(child)
   if len(s)>width:return None
  return frozenset(s)
 if frontier:
  # Every path from output to PI meets this cut. D is a function of these
  # predicates, even though the predicates need not be independent/injective.
  cut=set();visited=set()
  def visit(k):
   if not k or k in visited:return
   visited.add(k)
   if support(k) is not None:cut.add(k)
   else:
    for lit in ex.nodes[k][1]:visit(lit//2)
  visit(out//2);reach=cut
 candidates=[];seen=set()
 for k in sorted(reach):
  group=support(k)
  if not group or len(group)>width:continue
  group=sorted(group)
  positions={v:i for i,v in enumerate(group)}
  @functools.cache
  def table(i):
   if i==0:return 0
   if i<=ex.n:return sum(1<<state for state in range(1<<len(group)) if state>>positions[i]&1)
   kind,args=ex.nodes[i]
   acc=(1<<(1<<len(group)))-1 if kind=='a' else 0
   for lit in args:
    value=table(lit//2)
    if lit&1:value^=(1<<(1<<len(group)))-1
    acc=acc & value if kind=='a' else acc ^ value
   return acc
  mask=table(k);full=(1<<(1<<len(group)))-1
  # PI predicates guarantee full precision even if internal predicates are capped.
  key=(tuple(group),min(mask,full^mask))
  if key in seen:continue
  seen.add(key);candidates.append((k,group,mask))
 return candidates

def prepare(net,fault,stuck):
 old,out,rows,_,npar,_=build_fast(net,fault,stuck,difference=True)
 ex,out,active=compact(old,out)
 return ex,out,[rows[i-1] for i in active]

def run(net,fault,stuck,prefix,width=6,seconds=3,prepared=None,frontier=False):
 start=time.monotonic();prefix=pathlib.Path(prefix);prefix.parent.mkdir(parents=True,exist_ok=True)
 ex,out,rows=prepared or prepare(net,fault,stuck)
 predicates=select(ex,out,width,frontier)
 clauses,nv,ol=ex.cnf(out)
 write_cnf(prefix.with_suffix('.cnf'),clauses,nv)
 cmd=[str(HERE.parent/'linear_scaling/native/build/enumerator'),'--cnf',str(prefix.with_suffix('.cnf')),
      '--npi',str(nv),'--output',str(ol),'--support',','.join(str(p[0]) for p in predicates),
      '--method','greedy','--seconds',str(seconds),'--limit','100000','--prefix',str(prefix),
      '--freeze-support-only','1']
 native=subprocess.run(cmd,text=True,capture_output=True,check=True,timeout=seconds+10)
 result=json.loads(native.stdout)
 cubes=json.loads(prefix.with_suffix('.cubes.json').read_text())
 groups=[p[1] for p in predicates];regions=[]
 for cube in cubes:
  literals=set(cube)
  regions.append([mask if atom in literals else ((1<<(1<<len(group)))-1)^mask if -atom in literals else (1<<(1<<len(group)))-1
                  for atom,group,mask in predicates])
 prefix.with_suffix('.regions.json').write_text(json.dumps({'nvars':ex.n,'groups':groups,'regions':regions}))
 # The generic independent verifier reads region files and original-coordinate rows.
 meta={'fault':fault,'stuck':stuck,'npi':ex.n,'basis_rows':[hex(r) for r in rows],'pi_order':net.pi,
       'groups':groups,'predicates':predicates,'width':width,'seconds_limit':seconds,'frontier':frontier}
 prefix.with_suffix('.metadata.json').write_text(json.dumps(meta,indent=2))
 union=count_union(prefix,ex.n,groups,timeout=10) if result['complete'] else None
 result.update({'fault':fault,'stuck':stuck,'mode':'frontier' if frontier else 'predicates','width':width,'groups':len(groups),'union':union,
                'total_seconds':time.monotonic()-start})
 prefix.with_suffix('.comparison.json').write_text(json.dumps(result,indent=2))
 return result

if __name__=='__main__':
 cases=[('s5378_C','n673gat',0),('s5378_C','n1592gat',0),('s5378_C','n291gat',1),('s38584_C','g16349',1),('b19_C','P2_P1_P1_U3002',0)]
 nets={};results=[]
 for circuit,fault,stuck in cases:
  if circuit not in nets:nets[circuit]=FastNet(ROOT/'input/circuit'/f'{circuit}.v')
  net=nets[circuit];prepared=prepare(net,fault,stuck)
  for width in [3,6]:
   r=run(net,fault,stuck,HERE/'runs'/f'{circuit}_{fault}_{stuck}_predicates{width}',width,3,prepared)
   results.append(r);print(json.dumps(r),flush=True)
 (HERE/'results/predicates_pilot.json').write_text(json.dumps(results,indent=2))
