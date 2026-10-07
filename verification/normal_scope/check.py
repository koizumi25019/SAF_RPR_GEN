"""Scope regression: full/partial covers, independent simulation, GT, and speed.
All runs disable cross-fault cube reuse. No production output is overwritten.
"""
from pathlib import Path
import argparse,csv,importlib.util,json,os,re,statistics,subprocess,time,sys
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[1];RUNS=HERE/'runs'
sys.path.insert(0,str(ROOT/'verification/paper_core'))
from simulate_expanded import Circuit,truth_vectors

def run(binary,label,net,scope=True,method='xid',limit=0,gt=True,tdf=False,power=None,faults=None,capture=True):
    directory=RUNS/label;directory.mkdir(parents=True,exist_ok=True)
    for old in directory.glob('*.cover'):old.unlink()
    # Keep each invocation reproducible, even under an experimental shell env.
    hooks=set()
    for source in (ROOT/'src').rglob('*.c'):hooks.update(re.findall(r'getenv\("([A-Z0-9_]+)"\)',source.read_text()))
    env={k:v for k,v in os.environ.items() if k not in hooks}
    env.update(FDP_NORMAL_SCOPE=str(int(scope)))
    if scope and gt:env['FDP_NORMAL_SCOPE_VALIDATE']='1'
    if gt:env['GT_BDD']='1'
    if capture:env['PROFILE_CUBE_DIR']=str(directory)
    lines=[f'-net {net}',f'-fdp {directory}/fdp.csv',f'-log {directory}/time.log',f'-dc_method {method}','-dom_reuse off',f'-core_verify {"on" if gt and method=="core" else "off"}',f'-limit {limit}']
    if tdf:lines.append('-tdf')
    if power is not None:lines+=['-low_power on',f'-wsa_threshold {power}']
    if faults is not None:
        path=directory/'faults.txt';path.write_text(''.join(f'{n}\t{t}\n' for n,t in faults));lines.append(f'-fault {path}')
    settings=directory/'run.set';settings.write_text('\n'.join(lines)+'\n')
    start=time.monotonic()
    with (directory/'stdout.txt').open('w') as out,(directory/'stderr.txt').open('w') as err:
        subprocess.run([str(binary),'-set',str(settings)],cwd=ROOT,env=env,stdout=out,stderr=err,check=True,timeout=180)
    elapsed=time.monotonic()-start
    if gt:assert 'ALL VERIFIED' in (directory/'stderr.txt').read_text(),label
    rows=list(csv.DictReader((directory/'fdp.csv').open()))
    return directory,rows,elapsed

def signature(rows):return sorted((r['net_name'],r['f_type'],r['fdp'],r['complete']) for r in rows)

def simulate(netfile,directory):
    net=Circuit(netfile);covers=sorted(directory.glob('*.cover'));assert covers
    order=covers[0].read_text().splitlines()[1].split();assert set(order)=={net.alias[p] for p in net.pi}
    mask,vectors=truth_vectors(order);inputs={p:vectors[net.alias[p]] for p in net.pi};good=net.simulate(inputs,mask)
    checked=0;seen=set()
    rows={(r['net_name'],int(r['f_type']=='sa1')):r for r in csv.DictReader((directory/'fdp.csv').open()) if r['complete'] in ('0','1')}
    for path in covers:
        lines=path.read_text().splitlines();fault,stuck,complete,n,nc=lines[0].split();stuck=int(stuck);key=fault,stuck
        assert key not in seen and key in rows;seen.add(key)
        assert int(n)==len(order) and lines[1].split()==order and int(nc)==len(lines)-2
        faulty=net.simulate(inputs,mask,net.sites[fault],stuck);detection=0
        for p in net.po:detection|=good[p]^faulty[p]
        union=0
        for text in lines[2:]:
            assert len(text)==len(order) and set(text)<=set('01X')
            expanded=mask
            for p,bit in zip(order,text):
                if bit!='X':expanded&=vectors[p] if bit=='1' else mask^vectors[p]
            assert not expanded&(mask^detection),(key,text,'unsound')
            union|=expanded;checked+=1
        assert complete==rows[key]['complete']
        assert complete!='1' or union==detection,(key,'incomplete')
        assert abs(float(rows[key]['fdp'])-union.bit_count()/(1<<len(order)))<5e-11
    assert seen==set(rows)
    return dict(inputs=len(order),faults=len(seen),cubes=checked,all_expansions_sound=True,completed_covers_exact=True)

def tdf_simulate(netfile,directory,power):
    # Adapt final-cover dumps to the existing independent sequential simulator.
    from verify_tdf_power import verify
    adapted=directory/'sequential';adapted.mkdir(exist_ok=True)
    for src in directory.glob('*.cover'):
        lines=src.read_text().splitlines();fault,stuck,complete,n,nc=lines[0].split();assert complete=='1'
        (adapted/src.name).write_text(f'{fault} {stuck} {n}\n'+lines[1]+'\n'+''.join(f'{cube} {cube}\n' for cube in lines[2:]))
    import shutil
    shutil.copyfile(directory/'fdp.csv',adapted/'fdp.csv');shutil.copyfile(directory/'stderr.txt',adapted/'stderr.txt')
    return verify(netfile,adapted,power)

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--large',action='store_true');parser.add_argument('--benchmark',action='store_true');args=parser.parse_args()
    binary=HERE/'build/main_capture';assert binary.exists()
    report={'scope_default':'off','cube_reuse':'off','checks':[],
        'environment':{'cpu_quota':Path('/sys/fs/cgroup/cpu.max').read_text().strip() if Path('/sys/fs/cgroup/cpu.max').exists() else None,
            'cpu_affinity_count':len(os.sched_getaffinity(0)) if hasattr(os,'sched_getaffinity') else None,
            'memory_limit_bytes':Path('/sys/fs/cgroup/memory.max').read_text().strip() if Path('/sys/fs/cgroup/memory.max').exists() else None}}
    def record(label,**data):
        item=dict(label=label,**data);report['checks'].append(item);print(json.dumps(item),flush=True)
        (HERE/'results.json').write_text(json.dumps(report,indent=2)+'\n')
    for name in ['c17a','s27_C','s208_C','s298_C','s344_C','s510_C','s641_C','s713_C','s1494_C']:
        net=ROOT/f'input/circuit/{name}.v'
        d,rows,_=run(binary,name+'_scoped',net)
        _,reference,_=run(binary,name+'_full',net,scope=False,capture=False)
        assert signature(rows)==signature(reference),name
        if name=='c17a':
            gold=list(csv.DictReader((ROOT/'expected/c17a_result.csv').open()));assert sorted((r['net_name'],r['f_type'],r['fdp']) for r in rows)==sorted((r['net_name'],r['f_type'],r['fdp']) for r in gold)
        independent=simulate(net,d) if name in ['c17a','s27_C','s208_C','s298_C'] else None
        record(name,gt='ALL VERIFIED',full_fdp_matches=True,independent=independent)
    RUNS.mkdir(exist_ok=True)
    cases=[('disjoint','''module disjoint(a,b,c,d,e,y,z);
input a,b,c,d,e;
output y,z;
wire a,b,c,d,e,y,z;
AND2 G0(.A(a),.B(b),.Z(y));
AND3 G1(.A(c),.B(d),.C(e),.Z(z));
endmodule
''','y',0.25),('side_input','''module side_input(a,b,c,d,f,s,y,z);
input a,b,c,d;
output y,z;
wire a,b,c,d,f,s,y,z;
BUF G0(.A(a),.Z(f));
OR2 G1(.A(b),.B(c),.Z(s));
AND2 G2(.A(f),.B(s),.Z(y));
INV G3(.A(d),.Z(z));
endmodule
''','f',0.375)]
    for label,text,fault,fdp in cases:
        net=RUNS/(label+'.v');net.write_text(text)
        for method in ['xid','core']:
            d,rows,_=run(binary,label+'_'+method,net,method=method,faults=[(fault,'sa0')])
            assert any(r['net_name']==fault and abs(float(r['fdp'])-fdp)<1e-12 for r in rows)
            sim=simulate(net,d)
            lines=next(d.glob('*.cover')).read_text().splitlines();order=lines[1].split()
            outside=['c','d','e'] if label=='disjoint' else ['d']
            assert all(cube[order.index(p)]=='X' for cube in lines[2:] for p in outside)
            record(label+'_'+method,independent=sim,outside_inputs_always_X=True,fdp=fdp)
    for name in ['c17a','s208_C']:
        d,rows,_=run(binary,name+'_core',ROOT/f'input/circuit/{name}.v',method='core')
        _,ref,_=run(binary,name+'_core_full',ROOT/f'input/circuit/{name}.v',scope=False,method='core',capture=False)
        assert signature(rows)==signature(ref)
        record(name+'_core',gt='ALL VERIFIED',core_verify=True,full_fdp_matches=True,independent=simulate(ROOT/f'input/circuit/{name}.v',d))
    for name,method,power in [('s27','xid',None),('s208','xid',None),('s27','core',None),('s208','core',20),('s27','core',50)]:
        label=f'{name}_tdf_{method}_{power}';net=ROOT/f'input/circuit/{name}.v'
        # GT does not implement the power predicate; sequential exhaustive
        # simulation separately verifies detection, excitation, and power.
        d,rows,_=run(binary,label,net,method=method,tdf=True,power=power,gt=power is None)
        _,ref,_=run(binary,label+'_full',net,scope=False,method=method,tdf=True,power=power,gt=power is None,capture=False)
        assert signature(rows)==signature(ref)
        record(label,full_fdp_matches=True,independent=tdf_simulate(net,d,power))
    if args.large:
        net=ROOT/'input/circuit/s5378_C.v'
        d,rows,dt=run(binary,'s5378_all_l30_gt',net,limit=30,capture=False)
        reps=[r for r in rows if r['complete'] in ('0','1')];assert len(reps)==4551
        m=re.search(r'normal_gates=(\d+)/(\d+) outside_pi=(\d+)',(d/'stderr.txt').read_text());assert m
        report['scope_counts_s5378']={'kept_normal_gate_instances':int(m[1]),'full_normal_gate_instances':int(m[2]),'outside_pi_instances':int(m[3])}
        record('s5378_all_l30',gt='ALL VERIFIED',faults=len(reps),completed=sum(r['complete']=='1' for r in reps),wall_seconds=dt)
    if args.benchmark:
        net=ROOT/'input/circuit/s5378_C.v';timings={False:[],True:[]};outputs={}
        for repeat in range(2):
            for scope in ([False,True] if repeat==0 else [True,False]):
                _,rows,dt=run(binary,f's5378_bench_r{repeat}_{scope}',net,scope=scope,limit=30,gt=False,capture=False)
                timings[scope].append(dt);outputs[scope]=rows
                print(f'benchmark scope={scope} seconds={dt:.3f}',flush=True)
        off={ (r['net_name'],r['f_type']):r for r in outputs[False] if r['complete'] in ('0','1')}
        on={ (r['net_name'],r['f_type']):r for r in outputs[True] if r['complete'] in ('0','1')}
        assert off.keys()==on.keys()
        assert all(a['fdp']==on[k]['fdp'] for k,a in off.items() if a['complete']==on[k]['complete']=='1')
        for scope in [False,True]:
            assert (RUNS/f's5378_bench_r0_{scope}/fdp.csv').read_bytes()==(RUNS/f's5378_bench_r1_{scope}/fdp.csv').read_bytes()
        med={str(k):statistics.median(v) for k,v in timings.items()}
        record('s5378_benchmark',runs={str(k):v for k,v in timings.items()},medians=med,speedup=med['False']/med['True'],completed={str(k):sum(r['complete']=='1' for r in outputs[k]) for k in outputs},shared_complete_fdp_mismatches=0,repeat_csv_identical=True,
            complete_to_incomplete=sum(a['complete']=='1' and on[k]['complete']=='0' for k,a in off.items()),
            incomplete_to_complete=sum(a['complete']=='0' and on[k]['complete']=='1' for k,a in off.items()))
    print('PASS: normal CNF scope regression',flush=True)
if __name__=='__main__':main()
