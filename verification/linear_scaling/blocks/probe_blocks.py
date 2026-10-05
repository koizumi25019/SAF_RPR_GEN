"""SAT-certified Cartesian products of small input-block value sets."""
import argparse
import collections
import functools
import json
import pathlib
import sys
import time

HERE = pathlib.Path(__file__).resolve().parent
OLD = HERE.parents[1]/'linear_coordinates'
sys.path.insert(0,str(OLD))
from probe import Netlist, ROOT, SAT, build

def groups_for(ex,out,max_width=3):
    reachable=ex.reachable(out)
    @functools.cache
    def support(k):
        if k<=ex.n:return frozenset([k])
        return frozenset().union(*(support(v//2) for v in ex.nodes[k][1]))
    scores=collections.Counter()
    for k in sorted(reachable):
        s=support(k)
        if 2<=len(s)<=max_width:scores[tuple(sorted(s))]+=1
    remaining={k for k in reachable if k<=ex.n};groups=[]
    for g,w in sorted(scores.items(),key=lambda kv:(-kv[1],-len(kv[0]),kv[0])):
        if set(g)<=remaining:groups.append(g);remaining.difference_update(g)
    groups.extend((i,) for i in sorted(remaining))
    return groups

def state_encoding(groups,nv):
    clauses=[];states=[]
    for g in groups:
        qs=[]
        for value in range(1<<len(g)):
            nv+=1;q=nv;qs.append(q)
            bits=[v if value>>i&1 else -v for i,v in enumerate(g)]
            clauses.extend([[-q,b] for b in bits]);clauses.append([q]+[-b for b in bits])
        states.append(qs)
    return clauses,nv,states

def enumerate_blocks(ex,out,prefix,groups,limit=20000,seconds=60,method='prime',seed='bitprime'):
    prefix=pathlib.Path(prefix);prefix.parent.mkdir(parents=True,exist_ok=True)
    clauses,nv,ol=ex.cnf(out)
    extra,nv,states=state_encoding(groups,nv);clauses+=extra
    path=prefix.with_suffix('.cnf')
    path.write_text(f'p cnf {nv} {len(clauses)}\n'+''.join(' '.join(map(str,c))+' 0\n' for c in clauses))
    det,off=SAT(path,ex.n),SAT(path,ex.n);det.add([ol])
    support=sorted(v for g in groups for v in g)
    masks_by_condition=set();regions=[];complete=False;init_calls=0;block_calls=0
    start=time.monotonic()
    try:
        for idx in range(limit):
            rc,model=det.solve()
            if rc==20:complete=True;break
            if seed in ('bitcore','bitprime'):
                fixed=[model[v-1] for v in support]
                rc,core=off.solve([-ol]+fixed);init_calls+=1;assert rc==20
                fixed=[l for l in fixed if l in core]
                if seed=='bitprime':
                    for l in fixed[:]:
                        if l not in fixed:continue
                        trial=[x for x in fixed if x!=l]
                        rc,core=off.solve([-ol]+trial);init_calls+=1
                        if rc==20:fixed=[x for x in trial if x in core]
                fixed=set(fixed)
                allowed=[]
                for g in groups:
                    mask=0
                    for value in range(1<<len(g)):
                        if all((-v not in fixed) if value>>i&1 else (v not in fixed) for i,v in enumerate(g)):
                            mask|=1<<value
                    allowed.append(mask)
            else:
                allowed=[1<<sum((model[v-1]>0)<<i for i,v in enumerate(g)) for g in groups]
            forbidden=[-q for qs,mask in zip(states,allowed) for value,q in enumerate(qs) if not mask>>value&1]
            rc,core=off.solve([-ol]+forbidden);block_calls+=1;assert rc==20
            forbidden=[l for l in forbidden if l in core]
            if method=='prime':
                for l in forbidden[:]:
                    if l not in forbidden:continue
                    trial=[x for x in forbidden if x!=l]
                    rc,core=off.solve([-ol]+trial);block_calls+=1
                    if rc==20:forbidden=[x for x in trial if x in core]
            rc,_=off.solve([-ol]+forbidden);block_calls+=1;assert rc==20
            forbidden=set(forbidden)
            masks=[sum(1<<value for value,q in enumerate(qs) if -q not in forbidden) for qs in states]
            assert all(masks)
            regions.append(masks)
            # C = AND of forbidden-state negations. Its exact negation is one
            # clause over the state indicators; no per-region extension variables.
            det.add([-l for l in sorted(forbidden)])
            for i,(qs,mask) in enumerate(zip(states,masks)):
                if mask==(1<<len(qs))-1:continue
                masks_by_condition.add((i,mask))
            if (idx+1)%1000==0:print(json.dumps({'regions':idx+1,'seconds':time.monotonic()-start}),flush=True)
            if time.monotonic()-start>seconds:break
        else:
            rc,_=det.solve();complete=rc==20
        elapsed=time.monotonic()-start
        result={'regions':len(regions),'complete':complete,'seconds':elapsed,'det_calls':det.calls,'oracle_calls':off.calls,
                'seed_calls':init_calls,'block_calls':block_calls,'method':method,'seed':seed,
                'group_widths':dict(collections.Counter(map(len,groups))),'conditions':len(masks_by_condition)}
        prefix.with_suffix('.regions.json').write_text(json.dumps({'nvars':ex.n,'groups':groups,'regions':regions}))
        prefix.with_suffix('.result.json').write_text(json.dumps(result,indent=2))
        return result
    finally:det.close();off.close()

if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--net',default='input/circuit/s5378_C.v');p.add_argument('--fault',default='n673gat');p.add_argument('--stuck',type=int,default=0)
    p.add_argument('--mode',default='linear');p.add_argument('--width',type=int,default=3)
    p.add_argument('--cached-variant',choices=['old','full','short','long','binary'])
    p.add_argument('--method',choices=['core','prime'],default='prime');p.add_argument('--seed',choices=['minterm','bitcore','bitprime'],default='bitprime')
    p.add_argument('--limit',type=int,default=10000);p.add_argument('--seconds',type=int,default=45)
    p.add_argument('--prefix',required=True);a=p.parse_args()
    if a.cached_variant:
        sys.path.insert(0,str(HERE.parent/'basis'))
        from basis import CachedNet,build_cached
        net=CachedNet(ROOT/a.net);ex,out,rows,*_=build_cached(net,a.fault,a.stuck,a.cached_variant)
    else:
        net=Netlist(ROOT/a.net);ex,out,rows,*_=build(net,a.fault,a.stuck,a.mode)
    groups=groups_for(ex,out,a.width)
    result=enumerate_blocks(ex,out,a.prefix,groups,a.limit,a.seconds,a.method,a.seed)
    pathlib.Path(a.prefix).with_suffix('.basis.json').write_text(json.dumps({'pi':net.pi,'rows':[hex(r) for r in rows],'args':vars(a)}))
    print(json.dumps(result),flush=True)
