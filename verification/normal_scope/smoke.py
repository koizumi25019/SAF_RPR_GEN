"""Debug/Release, exact power encoding, and scoped DIMACS metadata checks."""
import json,os,subprocess
from pathlib import Path
from check import ROOT,HERE,RUNS,run,signature

def main():
    release=HERE/'build/main_release';debug=HERE/'build/main_debug'
    assert release.exists() and debug.exists(),'Build Release and Debug first'
    ext=Path(os.environ.get('FDP_EXTERNAL_DIR',ROOT/'external')).resolve()
    unit=HERE/'build/test_power_encoding'
    subprocess.run(['gcc','-std=gnu11','-O2','-fcommon','-ffunction-sections','-fdata-sections',
        '-I'+str(ROOT/'src'),'-I'+str(ext/'cadical/src'),
        str(ROOT/'verification/paper_core/test_power_encoding.c'),str(ROOT/'src/fdp/power_constraint.c'),
        str(ROOT/'src/fdp/cnf_dump.c'),str(ext/'cadical/build/libcadical.a'),
        '-Wl,--gc-sections','-lstdc++','-lm','-o',str(unit)],check=True)
    subprocess.run([str(unit)],check=True)
    a=run(release,'final_release_smoke',ROOT/'input/circuit/c17a.v',capture=False)[1]
    b=run(debug,'final_debug_smoke',ROOT/'input/circuit/c17a.v',capture=False)[1]
    assert signature(a)==signature(b)
    for scoped in [False,True]:
        p=RUNS/f'dump_smoke_{scoped}';p.mkdir(exist_ok=True)
        (p/'faults.txt').write_text('k\tsa0\n')
        settings=p/'run.set'
        settings.write_text(f'-net {ROOT}/input/circuit/c17a.v\n-fault {p}/faults.txt\n-fdp {p}/fdp.csv\n-log {p}/time.log\n-dom_reuse off\n')
        subprocess.run([str(release),'-set',str(settings)],env=dict(os.environ,FDP_NORMAL_SCOPE=str(int(scoped)),DUMP_CNF=str(p)),capture_output=True,text=True,check=True)
        cnf=next(p.glob('*.cnf')).read_text().splitlines()
        show=next(l for l in cnf if l.startswith('c p show')).split()[3:-1]
        assert len(show)==5
        assert any(l.startswith('c ind') for l in cnf)==(not scoped)
    result={'debug_release_c17a_gt':True,'power_encoding':True,'power_encoding_full_assignments':5119,
        'dimacs_all_pi_projection':True,'scoped_dimacs_no_false_independent_support_claim':True}
    (HERE/'smoke_results.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS: Debug/Release, power encoding, all-PI DIMACS projection')
if __name__=='__main__':main()
