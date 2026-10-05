"""Reusable experiment runner: cached preprocessing, native SAT, generated-cover BDD."""
import argparse
import hashlib
import json
import math
import pathlib
import subprocess
import sys
import time

HERE=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(HERE/'basis'))
sys.path.insert(0,str(HERE/'blocks'))
from basis import CachedNet,build_cached,probe
from probe_blocks import groups_for,state_encoding

def write_cnf(path,clauses,nv):
    path.write_text(f'p cnf {nv} {len(clauses)}\n'+''.join(' '.join(map(str,c))+' 0\n' for c in clauses))

def shape_key(ex,out):
    """Exact DAG serialization modulo encountered PI renaming and PI polarity.

    Matching keys certify a bijection of uniform coordinates, not just equal
    hashes or similar graphs. Commutative tie order can cause missed reuse.
    """
    inputs={};memo={};definitions=[]
    def visit(lit):
        k=lit//2;sign=lit&1
        if k==0:return ('c',sign)
        if k<=ex.n:
            if k not in inputs:inputs[k]=(len(inputs),sign)
            label,phase=inputs[k]
            return ('p',label,sign^phase)
        if k not in memo:
            t,args=ex.nodes[k]
            children=tuple(visit(a) for a in args)
            memo[k]=len(definitions);definitions.append((t,children))
        return ('g',memo[k],sign)
    root=visit(out)
    return (tuple(definitions),root),inputs

def count_union(prefix,nvars,groups=None,timeout=60,order='natural'):
    if order not in ('natural','large'):
        raise ValueError('Unknown cover insertion order')
    if groups is None:
        cubes=json.loads(prefix.with_suffix('.cubes.json').read_text())
        if order=='large':cubes=sorted(cubes,key=len)
        data=f'{nvars} {len(cubes)}\n'+''.join(' '.join(map(str,c))+' 0\n' for c in cubes)
        binary=HERE.parent/'linear_coordinates/build/cube_union'
    else:
        regions=json.loads(prefix.with_suffix('.regions.json').read_text())['regions']
        if order=='large':
            regions=sorted(regions,key=lambda r:sum(math.log2(m.bit_count())-len(g) for m,g in zip(r,groups)),reverse=True)
        data=f'{nvars} {len(groups)} {len(regions)}\n'
        data+=''.join(str(len(g))+' '+' '.join(map(str,g))+'\n' for g in groups)
        data+=''.join(' '.join(map(str,m))+'\n' for m in regions)
        binary=HERE/'blocks/build/region_union'
    r=subprocess.run([str(binary)],input=data,text=True,capture_output=True,check=True,timeout=timeout)
    result=json.loads(r.stdout)
    result['insertion_order']=order
    prefix.with_suffix('.union.json').write_text(json.dumps(result,indent=2))
    return result

def run_fault(net,fault,stuck,prefix,variant='long',width=1,method='greedy',seconds=5,limit=100000,prepared=None,simulate=False,native_binary=None,native_options=None,count_partial=True,union_timeout=60,groups_override=None,union_order='natural',union_counter=None):
    if not 1 <= width <= 6:
        raise ValueError('Block width must be between 1 and 6')
    prefix=pathlib.Path(prefix);prefix.parent.mkdir(parents=True,exist_ok=True)
    start=time.monotonic()
    ex,out,rows,_,npar,_=prepared or build_cached(net,fault,stuck,variant)
    build_seconds=time.monotonic()-start
    clauses,nv,ol=ex.cnf(out);groups=None;states=None
    if width>1:
        groups=groups_for(ex,out,width) if groups_override is None else groups_override
        assert all(1 <= len(g) <= width and len(set(g))==len(g) for g in groups)
        assert {v for g in groups for v in g} == {v for v in ex.reachable(out) if v<=ex.n}
        if simulate and len([v for g in groups for v in g]) != len({v for g in groups for v in g}):
            raise ValueError('Sensitivity simulation for overlapping blocks is not implemented')
        extra,nv,states=state_encoding(groups,nv);clauses+=extra
        support=[q for qs in states for q in qs];projection_n=nv
    else:
        support=sorted(i for i in ex.reachable(out) if i<=ex.n);projection_n=ex.n
    cnf=prefix.with_suffix('.cnf');write_cnf(cnf,clauses,nv)
    cmd=[str(native_binary or HERE/'native/build/enumerator'),'--cnf',str(cnf),'--npi',str(projection_n),'--output',str(ol),
         '--support',','.join(map(str,support)),'--method',method,'--seconds',str(seconds),'--limit',str(limit),
         '--prefix',str(prefix),'--negative-only',str(int(width>1))]
    cmd += list(native_options or [])
    if simulate:
        reach=sorted(ex.reachable(out));gates=[(i,ex.nodes[i]) for i in reach if i>ex.n]
        if groups is None:perturb=[(v,[(v,2)]) for v in support]
        else:perturb=[(q,[(var,(state>>j)&1) for j,var in enumerate(g)]) for g,qs in zip(groups,states) for state,q in enumerate(qs)]
        text=f'{ex.n} {len(ex.nodes)-1} {out} {len(gates)} {len(perturb)}\n'
        text+=''.join(f'{i} {kind} {len(args)} '+' '.join(map(str,args))+'\n' for i,(kind,args) in gates)
        text+=''.join(f'{atom} {len(assign)} '+' '.join(f'{v} {b}' for v,b in assign)+'\n' for atom,assign in perturb)
        sim=prefix.with_suffix('.sim');sim.write_text(text);cmd+=['--sim',str(sim)]
    run=subprocess.run(cmd,capture_output=True,text=True,check=True,timeout=seconds+10)
    result=json.loads(run.stdout)
    if groups is not None:
        cubes=json.loads(prefix.with_suffix('.cubes.json').read_text())
        regions=[]
        for cube in cubes:
            assert all(l<0 for l in cube);forbidden=set(cube)
            regions.append([sum(1<<v for v,q in enumerate(qs) if -q not in forbidden) for qs in states])
        prefix.with_suffix('.regions.json').write_text(json.dumps({'nvars':ex.n,'groups':groups,'regions':regions}))
    union=(union_counter or count_union)(prefix,ex.n,groups,timeout=union_timeout,order=union_order) if result['complete'] or count_partial else None
    key,mapping=shape_key(ex,out)
    metadata={'fault':fault,'stuck':stuck,'variant':variant,'width':width,'simulate':simulate,'method':method,
              'seconds_limit':seconds,'cube_limit':limit,'npi':ex.n,'basis_rows':[hex(r) for r in rows],
              'pi_order':net.pi,'groups':groups,'states':states,'output':ol,'projection_support':support,
              'build_seconds':build_seconds,'shape_sha256':hashlib.sha256(repr(key).encode()).hexdigest(),
              'coordinate_map':mapping,'end_to_end_seconds':time.monotonic()-start}
    prefix.with_suffix('.metadata.json').write_text(json.dumps(metadata,indent=2))
    combined={**result,'union':union,'build_seconds':build_seconds,'end_to_end_seconds':metadata['end_to_end_seconds']}
    prefix.with_suffix('.combined.json').write_text(json.dumps(combined,indent=2))
    return combined

if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--net',default='input/circuit/s5378_C.v');p.add_argument('--fault',default='n673gat');p.add_argument('--stuck',type=int,default=0)
    p.add_argument('--variant',default='long');p.add_argument('--width',type=int,default=1);p.add_argument('--method',default='greedy')
    p.add_argument('--seconds',type=float,default=10);p.add_argument('--prefix',required=True);p.add_argument('--simulate',action='store_true');a=p.parse_args()
    net=CachedNet(probe.ROOT/a.net)
    result=run_fault(net,a.fault,a.stuck,a.prefix,a.variant,a.width,a.method,a.seconds,simulate=a.simulate)
    print(json.dumps({'preprocessing_seconds':net.preprocessing_seconds,**result}),flush=True)
