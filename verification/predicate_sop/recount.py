"""Reorder only generated-cover BDD variables, preserving all input assignments."""
import json,pathlib,subprocess,time,math
HERE=pathlib.Path(__file__).resolve().parent
BINARY=HERE.parent/'linear_scaling/blocks/build/region_union'
def recount(prefix,order='groups',timeout=20):
 prefix=pathlib.Path(prefix)
 data=json.loads(prefix.with_suffix('.regions.json').read_text());n=data['nvars'];groups=data['groups'];regions=data['regions']
 if order.startswith('large'):
  regions=sorted(regions,key=lambda r:sum(math.log2(m.bit_count())-len(g) for m,g in zip(r,groups)),reverse=True)
 if order in ('groups','large_groups'):sequence=list(dict.fromkeys(v for g in groups for v in g))
 elif order=='reverse_groups':sequence=list(dict.fromkeys(v for g in reversed(groups) for v in g))
 elif order=='minimum':sequence=list(dict.fromkeys(v for g in sorted(groups,key=min) for v in g))
 else:sequence=list(range(1,n+1))
 sequence += [v for v in range(1,n+1) if v not in sequence]
 assert sorted(sequence)==list(range(1,n+1))
 remap={v:i+1 for i,v in enumerate(sequence)}
 text=f'{n} {len(groups)} {len(regions)}\n'+''.join(str(len(g))+' '+' '.join(str(remap[v]) for v in g)+'\n' for g in groups)+''.join(' '.join(map(str,r))+'\n' for r in regions)
 start=time.monotonic()
 try:
  p=subprocess.run([str(BINARY)],input=text,text=True,capture_output=True,check=True,timeout=timeout)
  result=json.loads(p.stdout);result.update({'order':order,'permutation':sequence,'complete':True})
 except subprocess.TimeoutExpired:result={'order':order,'complete':False,'cause':'bdd_timeout','seconds':time.monotonic()-start}
 prefix.with_suffix('.recount_'+order+'.json').write_text(json.dumps(result,indent=2));return result
if __name__=='__main__':
 prefix=HERE/'runs/s38584_C_g16349_1_learned512'
 for order in ['large','large_groups']:
  r=recount(prefix,order);print(json.dumps(r),flush=True)
  if r['complete']:break
