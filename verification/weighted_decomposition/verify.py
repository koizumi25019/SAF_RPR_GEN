"""Prove hierarchical covers against original gate CNF and check independence."""
import collections
import csv
import importlib.util
import json
import pathlib
import random
import time
from fractions import Fraction

from weighted import HERE,ROOT,FastNet,base,prepare,probe,run,weighted_union
from expand_connections import expand

spec=importlib.util.spec_from_file_location('weighted_raw',HERE.parent/'linear_coordinates/verify.py')
raw=importlib.util.module_from_spec(spec);spec.loader.exec_module(raw)


def read_cover(prefix):
    prefix=pathlib.Path(prefix)
    meta=json.loads(prefix.with_suffix('.metadata.json').read_text())
    if meta['groups'] is None:
        return {'cubes':json.loads(prefix.with_suffix('.cubes.json').read_text()),'nvars':meta['npi']}
    return json.loads(prefix.with_suffix('.regions.json').read_text())


def covers(data,assignment):
    if 'cubes' in data:
        return any(all(bool(assignment>>(abs(l)-1)&1)==(l>0) for l in c) for c in data['cubes'])
    states=[sum(((assignment>>(v-1))&1)<<j for j,v in enumerate(g)) for g in data['groups']]
    return any(all(mask>>s&1 for mask,s in zip(r,states)) for r in data['regions'])


def certificate(netpath,manifest_path):
    start=time.monotonic();manifest_path=pathlib.Path(manifest_path)
    data=json.loads(manifest_path.read_text());assert data['complete']
    net=probe.Netlist(netpath);rows=[int(r,16) for r in data['basis_rows']]
    pivots={}
    for row in rows:
        while row:
            k=row.bit_length()-1
            if k not in pivots:pivots[k]=row;break
            row^=pivots[k]
        assert row,'Dependent base coordinates'
    if len(net.g)>10000:
        fanout=collections.defaultdict(list)
        for output,(_,inputs) in net.g.items():
            for source in inputs:fanout[source].append(output)
        seen={data['fault']};stack=list(seen)
        while stack:
            for node in fanout[stack.pop()]:
                if node not in seen:seen.add(node);stack.append(node)
        net.po=[o for o in net.po if o in seen]
    clauses,nv,detection=raw.raw_cnf(net,data['fault'],data['stuck'],rows)
    nv+=1;one=nv;clauses.append([one]);cache={}
    def and_(lits):
        nonlocal nv
        lits=set(lits)
        if -one in lits or any(-l in lits for l in lits):return -one
        lits.discard(one)
        if not lits:return one
        if len(lits)==1:return next(iter(lits))
        key=tuple(sorted(lits))
        if key not in cache:
            nv+=1;z=nv
            clauses.extend([[-z,l] for l in key]);clauses.append([z]+[-l for l in key]);cache[key]=z
        return cache[key]
    def or_(lits):return -and_([-l for l in lits])
    def encode_cover(cover,mapping):
        def remap(l):return mapping[abs(l)]*(1 if l>0 else -1)
        if 'cubes' in cover:return or_([and_([remap(l) for l in c]) for c in cover['cubes']])
        regions=[]
        for region in cover['regions']:
            terms=[]
            for group,mask in zip(cover['groups'],region):
                if mask==(1<<(1<<len(group)))-1:continue
                states=[]
                for s in range(1<<len(group)):
                    if mask>>s&1:states.append(and_([remap(v if s>>j&1 else -v) for j,v in enumerate(group)]))
                terms.append(or_(states))
            regions.append(and_(terms))
        return or_(regions)
    used=set();topmap={};local_checked=0;probabilities={}
    for i,source in enumerate(data['sources'],1):
        support=set(source['support'])
        assert support and not used&support,'Dependent weighted coordinates'
        assert all(1<=v<=len(rows) for v in support)
        used.update(support)
        if source['kind']=='pi':
            assert support=={source['node']} and source['probability']=='1/2'
            topmap[i]=source['node'];continue
        mapping={int(k):v for k,v in source['map'].items()}
        assert set(map(abs,mapping.values()))==support
        assert len(mapping)==len(support)
        cover=read_cover(source['prefix'])
        n=len(mapping);assert n<=16
        if source['prefix'] not in probabilities:
            probability=Fraction(sum(covers(cover,a) for a in range(1<<n)),1<<n)
            probabilities[source['prefix']]=probability;local_checked+=1
        assert probabilities[source['prefix']]==Fraction(source['probability'])
        topmap[i]=encode_cover(cover,mapping)
    assert used==set(range(1,len(rows)+1))
    covered=encode_cover(read_cover(data['top_prefix']),topmap)
    path=manifest_path.with_suffix('.raw.cnf');base.write_cnf(path,clauses,nv)
    sat=probe.SAT(path,0)
    try:
        rc,_=sat.solve([covered,-detection]);assert rc==20,'Unsound cover'
        rc,_=sat.solve([-covered,detection]);assert rc==20,'Incomplete cover'
    finally:sat.close()
    result={'fault':data['fault'],'stuck':data['stuck'],'sound':True,'exact':True,
            'disjoint_supports':True,'coordinate_rank':len(rows),'local_probabilities_exhaustive':local_checked,
            'seconds':time.monotonic()-start}
    manifest_path.with_suffix('.verified.json').write_text(json.dumps(result,indent=2))
    return result


def weighted_counter():
    rng=random.Random(607);results=[]
    directory=HERE/'runs/counter';directory.mkdir(exist_ok=True)
    for trial in range(20):
        n=5;prefix=directory/f'case{trial}'
        cubes=[]
        for _ in range(12):
            cubes.append([i if rng.randrange(2) else -i for i in range(1,n+1) if rng.randrange(3)])
        weights=[Fraction(rng.randrange(9),8) for _ in range(n)]
        prefix.with_suffix('.cubes.json').write_text(json.dumps(cubes))
        expected=Fraction(0)
        for a in range(1<<n):
            if covers({'cubes':cubes},a):
                value=Fraction(1)
                for i,p in enumerate(weights):value*=p if a>>i&1 else 1-p
                expected+=value
        r=weighted_union(prefix,n,None,weights,3*n)
        assert Fraction(r['probability'])==expected
        results.append({'trial':trial,'probability':str(expected),'verified':True})
    return results


if __name__=='__main__':
    (HERE/'results').mkdir(exist_ok=True)
    counter=weighted_counter();(HERE/'results/counter_verification.json').write_text(json.dumps(counter,indent=2))
    path=expand(ROOT/'input/circuit/c17a.v',HERE/'runs/c17/expanded.v');net=FastNet(path);tests=[]
    for row in csv.DictReader((ROOT/'expected/c17a_result.csv').open()):
        fault,stuck=row['net_name'],int(row['f_type'][-1]);prefix=HERE/'runs/c17'/f'{fault}_{stuck}'
        r=run(net,fault,stuck,prefix)
        assert r['complete'] and f"{float(Fraction(r['probability'])):.10e}"==row['fdp']
        r['certificate']=certificate(path,prefix.with_suffix('.cover.json'));tests.append(r)
    (HERE/'results/c17_verification.json').write_text(json.dumps(tests,indent=2))
    print(json.dumps({'c17':len(tests),'weighted_random':len(counter),'all_verified':True}),flush=True)
    results=[]
    for row in json.loads((HERE/'results/pilot.json').read_text()):
        if row['complete']:
            circuit='s38584_C' if row['fault']=='g16349' else 's5378_C'
            r=certificate(ROOT/'input/circuit'/f'{circuit}.v',row['manifest'])
            results.append(r);print(json.dumps(r),flush=True)
    (HERE/'results/pilot_verification.json').write_text(json.dumps(results,indent=2))
