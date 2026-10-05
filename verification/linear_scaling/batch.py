"""All-fault benchmark with exact coordinate-isomorphism cover reuse."""
import argparse
import collections
import csv
import json
import pathlib
import time
from experiment import CachedNet,build_cached,shape_key,run_fault,probe,HERE
from expand_connections import expand

def batch(netpath,outdir,variant='long',sharing=True,width=1,seconds=2,reps=None,expand_branches=False,simulate=False,fault_file=None):
    outdir=pathlib.Path(outdir);outdir.mkdir(parents=True,exist_ok=True)
    start=time.monotonic()
    if expand_branches:netpath=expand(netpath,outdir/'expanded.v')
    net=CachedNet(netpath)
    if reps and fault_file:raise ValueError('Choose either --reps or --faults')
    if fault_file:
        faults=[]
        for line in pathlib.Path(fault_file).read_text().splitlines():
            if not line.strip() or line.lstrip().startswith('#'):continue
            name,kind=line.split();assert kind in ('sa0','sa1')
            faults.append((name,int(kind[-1])))
        faults=list(dict.fromkeys(faults))
    elif reps:
        rows=list(csv.DictReader(open(reps)))
        faults=[(r['net_name'],int(r['f_type'][-1])) for r in rows if r.get('complete','').strip()]
        faults=list(dict.fromkeys(faults))
    else:faults=[(f,s) for f in net.pi+list(net.g) for s in (0,1)]
    known=set(net.pi)|set(net.g)
    missing=[f for f,s in faults if f not in known]
    if missing:raise ValueError(('unknown faults',missing[:20],len(missing)))
    cache={};results=[];build_total=0;solve_total=0;union_total=0;reuse=0;cnt=0
    fp=open(outdir/'faults.jsonl','w')
    for index,(fault,stuck) in enumerate(faults):
        t=time.monotonic();prepared=build_cached(net,fault,stuck,variant);ex,out,*_=prepared
        key,mapping=shape_key(ex,out);build_total+=time.monotonic()-t
        if sharing and key in cache:
            prior=cache[key];row={**prior,'fault':fault,'stuck':stuck,'reused':True};reuse+=1
        else:
            prefix=outdir/f'f{index:05d}'
            r=run_fault(net,fault,stuck,prefix,variant,width,'greedy',seconds,prepared=prepared,simulate=simulate)
            row={'fault':fault,'stuck':stuck,'complete':r['complete'],'fdp':r['union']['probability'],
                 'cubes':r['cubes'],'sat_calls':r['sat_calls'],'enum_seconds':r['seconds'],
                 'union_seconds':r['union']['seconds'],'representative':index,'reused':False,'prefix':str(prefix)}
            # A cached partial cover remains a partial result, never complete.
            cache[key]=row;cnt+=1;solve_total+=r['seconds'];union_total+=r['union']['seconds']
        results.append(row);fp.write(json.dumps(row)+'\n');fp.flush()
        if (index+1)%100==0:
            print(json.dumps({'done':index+1,'faults':len(faults),'solved_classes':cnt,'reused':reuse,
                              'incomplete':sum(not r['complete'] for r in results),'seconds':time.monotonic()-start}),flush=True)
    fp.close()
    stats={'faults':len(faults),'independent_enumerations':cnt,'reused':reuse,'complete':sum(r['complete'] for r in results),
           'incomplete':sum(not r['complete'] for r in results),'enum_seconds':solve_total,'union_seconds':union_total,
           'build_seconds':build_total,'preprocess_seconds':net.preprocessing_seconds,'wall_seconds':time.monotonic()-start,
           'variant':variant,'sharing':sharing,'width':width,'per_fault_limit_seconds':seconds,'representative_csv':str(reps) if reps else None,
           'expanded_branches':expand_branches,'simulate':simulate,'fault_file':str(fault_file) if fault_file else None}
    (outdir/'summary.json').write_text(json.dumps(stats,indent=2));print(json.dumps(stats),flush=True)
    return stats

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--net',required=True);p.add_argument('--outdir',required=True)
    p.add_argument('--variant',default='long');p.add_argument('--no-sharing',action='store_true');p.add_argument('--width',type=int,default=1)
    p.add_argument('--seconds',type=float,default=2);p.add_argument('--reps');p.add_argument('--expand-branches',action='store_true')
    p.add_argument('--simulate',action='store_true');p.add_argument('--faults');a=p.parse_args()
    batch(probe.ROOT/a.net,a.outdir,a.variant,not a.no_sharing,a.width,a.seconds,a.reps,a.expand_branches,a.simulate,a.faults)
