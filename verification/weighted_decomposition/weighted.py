"""SAT/DC of independent nonlinear cuts followed by weighted top-cover union."""
import argparse
import csv
import importlib.util
import json
import math
import pathlib
import subprocess
import sys
import time
from fractions import Fraction

from decompose import HERE,ROOT,FastNet,compact,prepare,probe,select

# Resolve the older runner by explicit path to avoid legacy module ambiguity.
spec=importlib.util.spec_from_file_location('weighted_base_runner',HERE.parent/'linear_scaling/experiment.py')
base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base)


def weighted_union(prefix,nvars,groups,weights,original_nvars,timeout=10,order='weighted',result_path=None):
    if groups is None:
        groups=[[i+1] for i in range(nvars)]
        cubes=json.loads(prefix.with_suffix('.cubes.json').read_text())
        regions=[]
        for cube in cubes:
            r=[3]*nvars
            for l in cube:r[abs(l)-1]=2 if l>0 else 1
            regions.append(r)
    else:
        regions=json.loads(prefix.with_suffix('.regions.json').read_text())['regions']
    if order=='uniform':
        regions.sort(key=lambda r:sum(math.log2(m.bit_count())-len(g) for g,m in zip(groups,r)),reverse=True)
    elif order=='weighted':
        state_weights=[]
        for g in groups:
            values=[]
            for state in range(1<<len(g)):
                value=1.
                for j,v in enumerate(g):value*=float(weights[v-1] if state>>j&1 else 1-weights[v-1])
                values.append(value)
            state_weights.append(values)
        caches=[{} for _ in groups]
        def volume(region):
            result=0.
            for j,mask in enumerate(region):
                if mask not in caches[j]:
                    value=sum(p for s,p in enumerate(state_weights[j]) if mask>>s&1)
                    caches[j][mask]=math.log2(value) if value else -math.inf
                result+=caches[j][mask]
            return result
        regions.sort(key=volume,reverse=True)
    elif order!='natural':raise ValueError(order)
    data=f'{nvars} {len(groups)} {len(regions)} {original_nvars}\n'
    data+=' '.join(map(str,weights))+'\n'
    data+=''.join(str(len(g))+' '+' '.join(map(str,g))+'\n' for g in groups)
    data+=''.join(' '.join(map(str,r))+'\n' for r in regions)
    r=subprocess.run([str(HERE/'build/weighted_union')],input=data,text=True,capture_output=True,check=True,timeout=timeout)
    result=json.loads(r.stdout)
    result['insertion_order']=order
    (result_path or prefix.with_suffix('.weighted.json')).write_text(json.dumps(result,indent=2))
    return result


def run(net,fault,stuck,prefix,prepared=None,max_inputs=12,width=3,seconds=3,order='weighted',learning=0,mass_order=False):
    prefix=pathlib.Path(prefix);prefix.parent.mkdir(parents=True,exist_ok=True)
    start=time.monotonic()
    ex,out,rows,*_=prepared or prepare(net,fault,stuck)
    build_seconds=time.monotonic()-start
    t=time.monotonic();top,root,sources,stats=select(ex,out,max_inputs)
    decomposition_seconds=time.monotonic()-t
    weights=[];local_cubes=0;local_seconds=0;local_sat_calls=0
    # Local probability computations are shared modulo identical expression DAG.
    cache={}
    for source in sources:
        if source['kind']=='pi':
            source['probability']='1/2';weights.append(Fraction(1,2));continue
        local,output,active=compact(ex,2*source['node'])
        key,mapping=base.shape_key(local,output)
        if key in cache:
            prior,prior_mapping,probability=cache[key]
            inverse={label:(v,phase) for v,(label,phase) in mapping.items()}
            remap={}
            for v,(label,phase) in prior_mapping.items():
                target,other=inverse[label]
                remap[v]=active[target-1]*(-1 if phase^other else 1)
            source.update(prefix=str(prior),map=remap,probability=probability,reused=True)
        else:
            leaf=prefix.with_name(prefix.name+f'_cut{len(cache)}')
            t=time.monotonic()
            r=base.run_fault(net,fault,stuck,leaf,prepared=(local,output,[rows[i-1] for i in active],[],0,set()),
                             width=3,simulate=True,seconds=1,count_partial=False,union_order='large')
            local_seconds+=time.monotonic()-t;local_sat_calls+=r['sat_calls'];local_cubes+=r['cubes']
            if not r['complete']:
                result={**stats,'complete':False,'cause':'local_incomplete','seconds':time.monotonic()-start}
                prefix.with_suffix('.result.json').write_text(json.dumps(result,indent=2));return result
            probability=r['union']['probability']
            source.update(prefix=str(leaf),map={i+1:v for i,v in enumerate(active)},probability=probability,reused=False)
            cache[key]=(leaf,mapping,probability)
        weights.append(Fraction(source['probability']))
    top_prefix=prefix.with_name(prefix.name+'_top')
    def counter(path,nvars,groups,timeout,order):
        return weighted_union(path,nvars,groups,weights,ex.n,timeout=timeout,order=counter_order)
    counter_order=order
    top_prepared=(top,root,[1<<i for i in range(top.n)],[],0,set())
    groups=None;learning_seconds=0.;pilot_cubes=0;pilot_calls=0
    if learning:
        t=time.monotonic()
        pilot=prefix.with_name(prefix.name+'_pilot')
        pr=base.run_fault(net,fault,stuck,pilot,prepared=top_prepared,width=1,simulate=True,
                          seconds=.5,limit=learning,count_partial=False,union_counter=counter)
        spec=importlib.util.spec_from_file_location('weighted_pair_learning',HERE.parent/'predicate_sop/learned_groups.py')
        pairing=importlib.util.module_from_spec(spec);spec.loader.exec_module(pairing)
        cubes=json.loads(pilot.with_suffix('.cubes.json').read_text())
        groups,benefits=pairing.choose(top,root,cubes)
        prefix.with_suffix('.pairing.json').write_text(json.dumps({'groups':groups,'benefits':benefits},indent=2))
        learning_seconds=time.monotonic()-t;pilot_cubes=pr['cubes'];pilot_calls=pr['sat_calls']
    options=[]
    if mass_order and width>1:
        if groups is None:groups=base.groups_for(top,root,width)
        _,nv,_=top.cnf(root);_,_,states=base.state_encoding(groups,nv)
        scores=[]
        for g,qs in zip(groups,states):
            for state,q in enumerate(qs):
                value=Fraction(1)
                for j,v in enumerate(g):value*=weights[v-1] if state>>j&1 else 1-weights[v-1]
                scores.append((value,q))
        scores.sort(key=lambda item:(-item[0],item[1]))
        options=['--support',','.join(str(q) for _,q in scores)]
    t=time.monotonic()
    r=base.run_fault(net,fault,stuck,top_prefix,prepared=top_prepared,
                     width=width,simulate=True,seconds=seconds,count_partial=False,union_order='large',union_timeout=10,
                     union_counter=counter,groups_override=groups,native_options=options)
    top_seconds=time.monotonic()-t
    weighted=None
    if r['complete']:weighted=r['union']
    result={**stats,'fault':fault,'stuck':stuck,'max_inputs':max_inputs,'width':width,
            'complete':r['complete'],'probability':weighted['probability'] if weighted else None,
            'local_solves':len(cache),'local_cubes':local_cubes,'local_seconds':local_seconds,
            'learning':learning,'mass_order':mass_order,'learning_seconds':learning_seconds,
            'pilot_cubes':pilot_cubes,'pilot_sat_calls':pilot_calls,
            'local_sat_calls':local_sat_calls,'top_cubes':r['cubes'],'top_sat_calls':r['sat_calls'],
            'top_enum_seconds':r['seconds'],'top_seconds':top_seconds,'weighted':weighted,
            'build_seconds':build_seconds,'decomposition_seconds':decomposition_seconds,
            'seconds':time.monotonic()-start}
    manifest={'fault':fault,'stuck':stuck,'ncoords':ex.n,'basis_rows':[hex(row) for row in rows],
              'sources':sources,'top_prefix':str(top_prefix),'complete':r['complete'],
              'probability':result['probability']}
    prefix.with_suffix('.cover.json').write_text(json.dumps(manifest,indent=2))
    prefix.with_suffix('.result.json').write_text(json.dumps(result,indent=2))
    return result


if __name__=='__main__':
    (HERE/'results').mkdir(exist_ok=True)
    nets={};results=[]
    for circuit,fault,stuck in [('s5378_C','n673gat',0),('s5378_C','n1592gat',0),('s5378_C','n291gat',1),
                               ('s38584_C','g16349',1),('b19_C','P2_P1_P1_U3002',0)]:
        if circuit not in nets:nets[circuit]=FastNet(ROOT/'input/circuit'/f'{circuit}.v')
        net=nets[circuit];prepared=prepare(net,fault,stuck)
        for limit in [6,12]:
            prefix=HERE/'runs/pilot'/f'{circuit}_{fault}_{stuck}_k{limit}'
            r=run(net,fault,stuck,prefix,prepared,max_inputs=limit)
            r['manifest']=str(prefix.with_suffix('.cover.json'))
            results.append(r);print(json.dumps(r),flush=True)
            (HERE/'results/pilot.json').write_text(json.dumps(results,indent=2))
