"""Validate migrated baseline outputs, then measure one before/after run."""
import csv,json,os,re,subprocess,sys,time
from pathlib import Path
from build import BASELINE,VERIFICATION,HERE,BUILD
RUNS=HERE/'runs'
from importlib.util import spec_from_file_location, module_from_spec
reference_spec=spec_from_file_location('normal_scope_reference',VERIFICATION/'verification/normal_scope/check.py')
reference=module_from_spec(reference_spec);reference_spec.loader.exec_module(reference)
simulate=reference.simulate

def environment():
    hooks={'BASELINE_COVER_DIR','GT_BDD','FDP_NORMAL_SCOPE','FDP_NORMAL_SCOPE_VALIDATE','MDC_NODOM'}
    for root in [BASELINE,VERIFICATION]:
        for source in (root/'src').rglob('*.c'):hooks.update(re.findall(r'getenv\("([A-Z0-9_]+)"\)',source.read_text()))
    return {k:v for k,v in os.environ.items() if k not in hooks}

def run(binary,label,net,limit=0,scope=None,reuse=False,capture=False,extra=None):
    p=RUNS/label;p.mkdir(parents=True,exist_ok=True)
    for old in p.glob('*.cover'):old.unlink()
    env=environment()
    if scope is not None:env['FDP_NORMAL_SCOPE']=str(int(scope))
    if not reuse:env['MDC_NODOM']='1'
    if capture:env.update(BASELINE_COVER_DIR=str(p),FDP_NORMAL_SCOPE_VALIDATE='1')
    env.update(extra or {})
    settings=p/'run.set';settings.write_text(f'-net {net}\n-fdp {p}/fdp.csv\n-log {p}/time.log\n-limit {limit}\n')
    start=time.monotonic()
    with (p/'stdout.txt').open('w') as out,(p/'stderr.txt').open('w') as err:
        subprocess.run([str(binary),'-set',str(settings)],cwd=BASELINE/'build',env=env,stdout=out,stderr=err,check=True,timeout=240)
    elapsed=time.monotonic()-start
    rows=list(csv.DictReader((p/'fdp.csv').open()))
    return p,rows,elapsed

def verify_gt(net,directory,rows):
    path=directory/'gt';path.mkdir(exist_ok=True)
    representatives=[r for r in rows if r['complete'] in ('0','1')]
    faults=path/'faults.txt';faults.write_text(''.join(f"{r['net_name']}\t{r['f_type']}\n" for r in representatives))
    settings=path/'run.set';settings.write_text(f'-net {net}\n-fault {faults}\n-fdp {path}/unused.csv\n-log {path}/time.log\n-dc_method xid\n-dom_reuse off\n')
    env=dict(environment(),GT_BDD='1',BASELINE_COVER_DIR=str(directory))
    with (path/'stdout.txt').open('w') as out,(path/'stderr.txt').open('w') as err:
        subprocess.run([str(BUILD/'check_covers'),'-set',str(settings)],cwd=VERIFICATION,env=env,stdout=out,stderr=err,check=True,timeout=180)
    text=(path/'stderr.txt').read_text();assert 'ALL VERIFIED' in text
    matched=re.search(r'checked=(\d+)\s+UNSOUND=(\d+)\s+complete-but-NOT-exact=(\d+)',text);assert matched
    assert tuple(map(int,matched.groups()))==(len(representatives),0,0)
    return {'faults':len(representatives),'unsound':0,'completed_not_exact':0,'gt':'ALL VERIFIED'}

def signature(rows):return sorted((r['net_name'],r['f_type'],r['fdp'],r['complete']) for r in rows)

def main():
    release=BASELINE/'build/main_release';original=BUILD/'main_original';capture=BUILD/'main_capture'
    report={'pre_migration_commit':'5da133b','default_scope':'on','cnf_insertion':'direct ccadical_add, no wrapper or macro',
            'scope_strategy':'TFI closure of all TFO nodes and all emitted essential-assignment signals','checks':[],
            'benchmark_repeats':1,'cpu_quota':Path('/sys/fs/cgroup/cpu.max').read_text().strip()}
    def save(label,**data):
        item={'label':label,**data};report['checks'].append(item);print(json.dumps(item),flush=True)
        (HERE/'results.json').write_text(json.dumps(report,indent=2)+'\n')
    for name in ['c17a','s27_C','s208_C','s298_C','s344_C','s510_C','s641_C','s713_C','s1494_C']:
        net=BASELINE/f'input/circuit/{name}.v'
        d,rows,_=run(capture,name+'_new',net,capture=True)
        _,ref,_=run(original,name+'_old',net)
        assert signature(rows)==signature(ref),name
        gt=verify_gt(net,d,rows)
        independent=simulate(net,d) if name in ['c17a','s27_C','s208_C','s298_C'] else None
        save(name,full_fdp_matches_original=True,independent_simulation=independent,**gt)
        if name=='c17a':
            gold=list(csv.DictReader((BASELINE/'expected/c17a_result.csv').open()))
            sig=lambda rs:sorted((r['net_name'],r['f_type'],r['fdp']) for r in rs)
            assert sig(rows)==sig(gold)
            _,debug,_=run(BASELINE/'build/main_debug','c17a_debug',net)
            _,disabled,_=run(release,'c17a_scope_off',net,scope=False)
            assert signature(rows)==signature(debug)==signature(disabled)
            save('c17a_golden',golden=True,debug_release=True,scope_off=True)
    for name in ['c17a','s208_C']:
        net=BASELINE/f'input/circuit/{name}.v'
        d,rows,_=run(capture,name+'_reuse_on',net,reuse=True,capture=True)
        _,ref,_=run(original,name+'_old_reuse_on',net,reuse=True)
        assert signature(rows)==signature(ref)
        save(name+'_reuse_on',full_fdp_matches_original=True,independent_simulation=simulate(net,d),**verify_gt(net,d,rows))
    for flag in ['MDC_NOEA','MDC_NOPROP']:
        net=BASELINE/'input/circuit/s208_C.v'
        d,rows,_=run(capture,flag,net,capture=True,extra={flag:'1'})
        save(flag,**verify_gt(net,d,rows))
    net=BASELINE/'input/circuit/s5378_C.v'
    # Validate actual baseline cubes, independently from the verification ATPG.
    d,rows,_=run(capture,'s5378_all_l30_gt',net,limit=30,capture=True)
    gt=verify_gt(net,d,rows);assert gt['faults']==4551
    save('s5378_all_l30',completed=sum(r['complete']=='1' for r in rows),**gt)
    # Nothing else builds or verifies during these two timed executions.
    _,before,t0=run(original,'s5378_before',net,limit=30)
    print(f'BEFORE {t0:.3f} seconds',flush=True)
    _,after,t1=run(release,'s5378_after',net,limit=30)
    a={(r['net_name'],r['f_type']):r for r in before if r['complete'] in ('0','1')}
    b={(r['net_name'],r['f_type']):r for r in after if r['complete'] in ('0','1')}
    assert a.keys()==b.keys() and len(a)==4551
    assert all(r['fdp']==b[k]['fdp'] for k,r in a.items() if r['complete']==b[k]['complete']=='1')
    # The capture hook and value validator must not change the actual cover.
    assert signature(rows)==signature(after)
    save('s5378_once',before_seconds=t0,after_seconds=t1,speedup=t0/t1,
        before_completed=sum(r['complete']=='1' for r in before),after_completed=sum(r['complete']=='1' for r in after),
        shared_complete_fdp_mismatches=0,generated_output_matches_validated_run=True,reuse='off',limit=30)
    print('PASS baseline migration: direct insertion, default scope, correctness and single-run timing',flush=True)
if __name__=='__main__':main()
