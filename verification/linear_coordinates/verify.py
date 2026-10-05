"""Independent raw gate CNF certificate for transformed covers; no BDD/counting solver."""
import argparse
import csv
import itertools
import json
import time
from fractions import Fraction
from probe import Netlist, ROOT, TMP, SAT, build, enumerate_cover, eval_gate
from count_cubes import count_cubes

def raw_cnf(net,fault,stuck,rows):
    n=len(net.pi); nv=n; clauses=[]
    def fresh():
        nonlocal nv
        nv+=1;return nv
    def gate(t, ins):
        # Exhaustive gate relation, independent of optimized expression construction.
        z=fresh()
        for bits in itertools.product([0,1],repeat=len(ins)):
            val=eval_gate(t,bits,1)
            clauses.append([(-l if b else l) for l,b in zip(ins,bits)]+[z if val else -z])
        return z
    good={p:fresh() for p in net.pi}; original_pi=good.copy(); bad=good.copy()
    one=fresh();clauses.append([one])
    if fault in bad:bad[fault]=one if stuck else -one
    def rec(k, faulty):
        vals=bad if faulty else good
        if k not in vals:
            if faulty and k==fault:vals[k]=one if stuck else -one
            else:
                t,ins=net.g[k];vals[k]=gate(t,[rec(i,faulty) for i in ins])
        return vals[k]
    diffs=[gate('XOR',[rec(o,False),rec(o,True)]) for o in net.po]
    output=fresh()
    for d in diffs:clauses.append([-d,output])
    clauses.append([-output]+diffs)
    for j,row in enumerate(rows):
        ins=[original_pi[k] for i,k in enumerate(net.pi) if row>>i&1]
        acc=ins[0]
        for b in ins[1:]:acc=gate('XOR',[acc,b])
        y=j+1;clauses.extend([[-y,acc],[y,-acc]])
    return clauses,nv,output

def verify_cover(net,fault,stuck,rows,cubes,prefix,disjoint=False):
    clauses,nv,out=raw_cnf(net,fault,stuck,rows)
    path=TMP/(prefix+'.raw_verify.cnf')
    path.write_text(f'p cnf {nv} {len(clauses)}\n'+''.join(' '.join(map(str,c))+' 0\n' for c in clauses))
    off,det=SAT(path,len(net.pi)),SAT(path,len(net.pi));start=time.monotonic()
    try:
        for i,cube in enumerate(cubes):
            rc,model=off.solve([-out]+cube)
            if rc!=20:raise AssertionError(('unsound',i,model))
            det.add([-l for l in cube])
        rc,model=det.solve([out]);assert rc==20,('incomplete',model)
        if disjoint:
            sets=[set(c) for c in cubes]
            for i,c in enumerate(sets):
                for d in sets[:i]:assert any(-l in d for l in c),('overlap',i)
        return {'sound_cubes':len(cubes),'coverage_unsat':True,'pairwise_disjoint':disjoint,'seconds':time.monotonic()-start}
    finally:off.close();det.close()

def c17_regression():
    net=Netlist(ROOT/'input/circuit/c17a.v'); results=[]
    with open(ROOT/'expected/c17a_result.csv') as f:gold={(r['net_name'],int(r['f_type'][-1])):r['fdp'] for r in csv.DictReader(f)}
    # Raw netlist stem faults; branch faults are not implemented by this experiment.
    for fault in net.pi+list(net.g):
        for stuck in (0,1):
            ex,out,rows,_,_,_=build(net,fault,stuck,'linear')
            detected=0
            for bits in itertools.product([0,1],repeat=len(net.pi)):
                xb=sum(b<<i for i,b in enumerate(bits));yb=[(xb&r).bit_count()&1 for r in rows]
                expected=net.simulate(bits,fault,stuck)
                assert ex.evaluate(out,yb)==expected,(fault,stuck,bits)
                detected+=expected
            expected=Fraction(detected,1<<len(net.pi))
            assert f'{float(expected):.10e}'==gold[(fault,stuck)]
            prefix=f'c17a_{fault}_sa{stuck}'
            result=enumerate_cover(ex,out,prefix,1000,True,10,True)
            assert result['complete'] and Fraction(result['probability'])==expected
            union=count_cubes(prefix,len(net.pi))
            assert Fraction(union['probability'])==expected
            cubes=json.loads((TMP/(prefix+'.cubes.json')).read_text())
            verification=verify_cover(net,fault,stuck,rows,cubes,prefix,True)
            results.append({'fault':fault,'stuck':stuck,'fdp':str(expected),**verification})
    (TMP/'c17a_regression.json').write_text(json.dumps(results,indent=2))
    print(json.dumps({'c17a_stem_faults':len(results),'all_assignments_per_fault':32,'golden_match':True,'sat_coverage_verified':True,'cube_union_bdd_match':True}))

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--c17',action='store_true');p.add_argument('--net',default='input/circuit/s5378_C.v');p.add_argument('--fault',default='n673gat');p.add_argument('--stuck',type=int,default=0);p.add_argument('--mode',default='linear');p.add_argument('--prefix',default='n673gat_linear_overlap_prime');p.add_argument('--disjoint',action='store_true');a=p.parse_args()
    if a.c17:c17_regression()
    else:
        net=Netlist(ROOT/a.net);_,_,rows,_,_,_=build(net,a.fault,a.stuck,a.mode)
        cubes=json.loads((TMP/(a.prefix+'.cubes.json')).read_text())
        result=verify_cover(net,a.fault,a.stuck,rows,cubes,a.prefix,a.disjoint)
        (TMP/(a.prefix+'.verified.json')).write_text(json.dumps(result,indent=2));print(json.dumps(result))
