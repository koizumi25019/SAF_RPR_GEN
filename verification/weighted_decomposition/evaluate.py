"""Controlled old/new comparisons, then independent checks outside timing."""
import argparse
import csv
import importlib.util
import json
import statistics
import time
from fractions import Fraction

from weighted import HERE, ROOT, FastNet, prepare, run

spec=importlib.util.spec_from_file_location('comparison_previous',HERE.parent/'predicate_sop/learned_groups.py')
previous=importlib.util.module_from_spec(spec);spec.loader.exec_module(previous)


def benchmark():
    net=FastNet(ROOT/'input/circuit/s38584_C.v');results=[]
    expected=Fraction(137438953471,274877906944)
    for fault,stuck in [('g16349',1),('II17661',0),('g13329',1),('II15893',0)]:
        t=time.monotonic();prepared=prepare(net,fault,stuck);prepare_seconds=time.monotonic()-t
        for repeat in range(3 if fault=='g16349' else 1):
            methods=['previous','weighted'] if repeat%2==0 else ['weighted','previous']
            for method in methods:
                prefix=HERE/'runs/comparison'/f'{fault}_{stuck}_{method}_{repeat}'
                prefix.parent.mkdir(parents=True,exist_ok=True)
                if method=='weighted':
                    r=run(net,fault,stuck,prefix,prepared,seconds=5,learning=512,mass_order=True)
                    probability=r['probability'];elapsed=r['seconds'];cubes=r['top_cubes']
                    enum_seconds=r['top_enum_seconds'];bdd_seconds=r['weighted']['seconds']
                else:
                    r=previous.run(net,fault,stuck,prefix,prepared,seconds=5,pilot_limit=512,union_order='large')
                    probability=r['union']['probability'];elapsed=r['total_seconds'];cubes=r['cubes']
                    enum_seconds=r['seconds'];bdd_seconds=r['union']['seconds']
                assert r['complete'] and Fraction(probability)==expected
                row={'fault':fault,'stuck':stuck,'method':method,'repeat':repeat,
                     'complete':True,'probability':probability,'seconds':elapsed,
                     'top_cubes':cubes,'pilot_cubes':r['pilot_cubes'],
                     'local_cubes':r.get('local_cubes',0),'enum_seconds':enum_seconds,
                     'bdd_seconds':bdd_seconds,'prepare_seconds_excluded':prepare_seconds,
                     'prefix':str(prefix)}
                results.append(row);print(json.dumps(row),flush=True)
                (HERE/'results/comparison.json').write_text(json.dumps(results,indent=2))
    with (HERE/'results/comparison.csv').open('w') as stream:
        writer=csv.DictWriter(stream,fieldnames=results[0].keys());writer.writeheader();writer.writerows(results)
    medians={m:statistics.median(r['seconds'] for r in results if r['method']==m and r['fault']=='g16349')
             for m in ['previous','weighted']}
    print(json.dumps({'g16349_medians':medians,'speedup':medians['previous']/medians['weighted']}),flush=True)


def verify():
    spec=importlib.util.spec_from_file_location('comparison_proof',HERE/'verify.py')
    proof=importlib.util.module_from_spec(spec);spec.loader.exec_module(proof)
    results=[]
    manifests=[r['manifest'] for r in json.loads((HERE/'results/refine.json').read_text()) if r['complete']]
    manifests += [r['prefix']+'.cover.json' for r in json.loads((HERE/'results/comparison.json').read_text())
                  if r['method']=='weighted']
    for manifest in manifests:
        r=proof.certificate(ROOT/'input/circuit/s38584_C.v',manifest)
        r['manifest']=manifest;results.append(r);print(json.dumps(r),flush=True)
    (HERE/'results/comparison_verification.json').write_text(json.dumps(results,indent=2))
    # Exercise pilot learning, weighted DC order, and weighted counting together.
    path=proof.expand(ROOT/'input/circuit/c17a.v',HERE/'runs/c17_refined/expanded.v')
    net=FastNet(path);results=[]
    for row in csv.DictReader((ROOT/'expected/c17a_result.csv').open()):
        fault,stuck=row['net_name'],int(row['f_type'][-1]);prefix=HERE/'runs/c17_refined'/f'{fault}_{stuck}'
        r=run(net,fault,stuck,prefix,learning=512,mass_order=True)
        assert r['complete'] and f"{float(Fraction(r['probability'])):.10e}"==row['fdp']
        r['certificate']=proof.certificate(path,prefix.with_suffix('.cover.json'));results.append(r)
    (HERE/'results/c17_refined_verification.json').write_text(json.dumps(results,indent=2))
    print(json.dumps({'c17_refined':len(results),'all_verified':True}),flush=True)


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--verify',action='store_true');args=parser.parse_args()
    verify() if args.verify else benchmark()
