"""Official TabularAllSAT comparison; generated PI cubes are counted by BDD.

The upstream model count is only a cross-check, never the FDP source. Uses
positive-only PG for the one-sided solver, and dual PG for our SAT/DC oracle.
"""
import csv
import json
import pathlib
import re
import resource
import subprocess
import time
from fractions import Fraction

from run import HERE, ROOT, Encoded, FastNet, certificate, prepare, probe
from experiment import count_union, write_cnf
from expand_connections import expand

VENDOR = HERE/'vendor/tabularAllSAT'
BINARY = VENDOR/'cdcl-vsads/solver'


def output_limit():
    resource.setrlimit(resource.RLIMIT_FSIZE, (64*1024*1024,64*1024*1024))


def run_tabular(net, fault, stuck, prefix, prepared, encoding='nnf_pg', seconds=3):
    prefix = pathlib.Path(prefix)
    prefix.parent.mkdir(parents=True,exist_ok=True)
    work = prefix.parent/(prefix.name+'_work')
    work.mkdir(exist_ok=True)
    (work/'output.txt').unlink(missing_ok=True)
    ex, out, rows = prepared[:3]
    start = time.monotonic()
    clauses, nv, ol = Encoded(ex,encoding).cnf(out,dual=False)
    clauses.append([ol])
    support = sorted(i for i in ex.reachable(out) if i<=ex.n)
    cnf = prefix.with_suffix('.cnf')
    # Upstream format: fourth header field and c p show without trailing zero.
    data = f'p cnf {nv} {len(clauses)} {len(support)}\n'
    if support:
        data += 'c p show '+' '.join(map(str,support))+'\n'
    data += ''.join(' '.join(map(str,c))+' 0\n' for c in clauses)
    cnf.write_text(data)
    timed_out = False
    command = [str(BINARY), '-q', '--output-file=1', str(cnf)]
    t = time.monotonic()
    with prefix.with_suffix('.stdout').open('w') as stdout, prefix.with_suffix('.stderr').open('w') as stderr:
        try:
            p = subprocess.run(command, cwd=work, stdout=stdout, stderr=stderr,
                               timeout=seconds, preexec_fn=output_limit)
            rc = p.returncode
        except subprocess.TimeoutExpired:
            timed_out, rc = True, None
    elapsed = time.monotonic()-t
    text = prefix.with_suffix('.stdout').read_text()
    counts = re.findall(r's MODEL COUNT\s*\n(\d+)', text)
    enumerated = rc==20 and bool(counts)
    upstream_count = int(counts[-1]) if enumerated else None
    path = work/'output.txt'
    cubes = []
    if path.exists():
        with path.open() as f:
            for line in f:
                # A killed writer can leave a partial line. It is not a cube.
                if not line.endswith('\n'):
                    continue
                cube = list(map(int,line.split()))
                assert len(set(map(abs,cube)))==len(cube)
                assert all(abs(l) in support for l in cube)
                cubes.append(cube)
    if enumerated and upstream_count and not cubes:
        # Some upstream trivial-formula paths report a count but print no cube.
        # Only reconstruct the universal cube after independent UNSAT proof.
        cc,nvv,root=ex.cnf(out)
        check=prefix.with_suffix('.trivial.cnf');write_cnf(check,cc,nvv)
        sat=probe.SAT(check,0)
        try:
            code,_=sat.solve([-root])
            assert code==20 and upstream_count==1<<len(support)
        finally:
            sat.close()
        cubes=[[]]
    prefix.with_suffix('.cubes.json').write_text(json.dumps(cubes))
    union = None
    bdd_timeout = False
    if enumerated:
        try:
            union=count_union(prefix,ex.n,timeout=10,order='large')
            assert Fraction(union['probability']) == Fraction(upstream_count,1<<len(support))
            # Equality of union volume and sum of cube volumes verifies disjointness.
            assert sum(1<<(ex.n-len(c)) for c in cubes)==int(union['model_count'])
        except subprocess.TimeoutExpired:
            bdd_timeout=True
    complete = enumerated and not bdd_timeout
    result={'fault':fault,'stuck':stuck,'encoding':encoding,'method':'tabular',
            'complete':complete,'enumeration_complete':enumerated,'timed_out':timed_out,
            'bdd_timeout':bdd_timeout,'returncode':rc,'cubes':len(cubes),
            'seconds':elapsed,'total_seconds':time.monotonic()-start,'cnf_vars':nv,
            'cnf_clauses':len(clauses),'union':union,'upstream_count':upstream_count}
    prefix.with_suffix('.result.json').write_text(json.dumps(result,indent=2))
    meta={'fault':fault,'stuck':stuck,'basis_rows':[hex(r) for r in rows],
          'pi_order':net.pi,'npi':ex.n,'groups':None,'output':ol}
    prefix.with_suffix('.metadata.json').write_text(json.dumps(meta,indent=2))
    return result


def main():
    provenance={'repository':'https://github.com/giuspek/tabularAllSAT',
                'revision':subprocess.check_output(['git','rev-parse','HEAD'],cwd=VENDOR,text=True).strip(),
                'modified_upstream_source':False}
    (HERE/'results/tabular_source.json').write_text(json.dumps(provenance,indent=2))
    path=expand(ROOT/'input/circuit/c17a.v',HERE/'runs/tabular_c17/expanded.v')
    net=FastNet(path);tests=[]
    for row in csv.DictReader((ROOT/'expected/c17a_result.csv').open()):
        fault,stuck=row['net_name'],int(row['f_type'][-1])
        prepared=prepare(net,fault,stuck)
        for encoding in ['tseitin','nnf_pg']:
            prefix=HERE/'runs/tabular_c17'/f'{fault}_{stuck}_{encoding}'
            r=run_tabular(net,fault,stuck,prefix,prepared,encoding)
            assert r['complete'],r
            assert f"{float(Fraction(r['union']['probability'])):.10e}"==row['fdp']
            r['certificate']=certificate(path,prefix)
            tests.append(r)
    (HERE/'results/tabular_c17.json').write_text(json.dumps(tests,indent=2))
    print(json.dumps({'c17':len(tests),'all_verified':True}),flush=True)
    nets={};results=[]
    for circuit,fault,stuck in [('s5378_C','n673gat',0),('s5378_C','n1592gat',0),
                               ('s5378_C','n291gat',1),('s38584_C','g16349',1),
                               ('b19_C','P2_P1_P1_U3002',0)]:
        path=ROOT/'input/circuit'/f'{circuit}.v'
        if circuit not in nets:nets[circuit]=FastNet(path)
        net=nets[circuit];prepared=prepare(net,fault,stuck)
        for encoding in ['tseitin','nnf_pg']:
            prefix=HERE/'runs/tabular_pilot'/f'{circuit}_{fault}_{stuck}_{encoding}'
            r=run_tabular(net,fault,stuck,prefix,prepared,encoding)
            if r['complete']:r['certificate']=certificate(path,prefix)
            results.append(r)
            print(json.dumps({k:r[k] for k in ['fault','encoding','complete','cubes','seconds']}),flush=True)
            (HERE/'results/tabular_pilot.json').write_text(json.dumps(results,indent=2))


if __name__=='__main__':main()
