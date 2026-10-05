#!/usr/bin/env python3
"""Power option errors, Debug/Release equivalence, BDD fallback and default GT regression."""
import csv
import os
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'output/power_settings_checks'
OUT.mkdir(parents=True,exist_ok=True)
hooks=set()
for source in (ROOT/'src').rglob('*.c'):hooks.update(re.findall(r'getenv\("([A-Z0-9_]+)"\)',source.read_text(errors='replace')))
ENV={k:v for k,v in os.environ.items() if k not in hooks}

def run(label,text,binary='main_release',extra=None,expected=0):
    settings=OUT/f'{label}.set'
    settings.write_text(text+f'\n-fdp {OUT}/{label}.csv\n-log {OUT}/{label}.log\n')
    with (OUT/f'{label}.stdout').open('w') as out,(OUT/f'{label}.stderr').open('w') as err:
        result=subprocess.run([str(ROOT/'build'/binary),'-set',str(settings)],cwd=ROOT/'build',
            env=dict(ENV,**(extra or {})),stdout=out,stderr=err,timeout=300)
    assert result.returncode==expected,(label,result.returncode)
    if not expected:assert 'ALL VERIFIED' in (OUT/f'{label}.stderr').read_text(),label
    return OUT/f'{label}.csv'

def probabilities(path):
    return {(r['net_name'],r['f_type']):r['fdp'] for r in csv.DictReader(path.open())}

def main():
    subprocess.run(['gcc','-std=gnu11','-O2','-fcommon','-ffunction-sections','-fdata-sections',
        '-I'+str(ROOT/'src'),'-I'+str(ROOT/'external/cadical/src'),
        str(ROOT/'verification/paper_core/test_power_encoding.c'),str(ROOT/'src/fdp/power_constraint.c'),
        str(ROOT/'src/fdp/cnf_dump.c'),str(ROOT/'external/cadical/build/libcadical.a'),
        '-Wl,--gc-sections','-lstdc++','-lm','-o',str(OUT/'test_encoding')],check=True)
    subprocess.run([str(OUT/'test_encoding')],check=True)
    base=f'-tdf\n-net {ROOT}/input/circuit/s27.v\n-dc_method core\n'
    invalid=['-low_power on','-low_power yes','-low_power','-low_power on\n-wsa_threshold',
             '-low_power on\n-wsa_threshold -1','-low_power on\n-wsa_threshold 101',
             '-low_power on\n-wsa_threshold 20.0','-low_power on\n-wsa_threshold 20bad',
             '-low_power on\n-wsa_threshold 99999999999999999999999',
             '-low_power on\n-wsa_threshold 20\n-dc_method xid',
             '-low_power on\n-wsa_threshold 20\n-saf']
    for i,case in enumerate(invalid):run(f'invalid_{i}',base+case,expected=1)
    for circuit,percent in [('s27',50),('s208',20)]:
        text=f'-tdf\n-net {ROOT}/input/circuit/{circuit}.v\n-dc_method core\n-dom_reuse off\n-core_verify on\n-low_power on\n-wsa_threshold {percent}\n'
        release=run(f'{circuit}_release',text,extra={'GT_BDD':'1'})
        debug=run(f'{circuit}_debug',text,binary='main_debug',extra={'GT_BDD':'1'})
        assert probabilities(release)==probabilities(debug)
        fallback=run(f'{circuit}_bdd_exact',text+'-limit 1\n',extra={'GT_BDD':'1','BDD_EXACT':'1'})
        assert probabilities(fallback)==probabilities(release)
        assert all(r['complete'] in ('1','') for r in csv.DictReader(fallback.open()))
        print(circuit+': Debug/Release, BDD_EXACT power FDP match',flush=True)
    # Required default SAF golden and the usual nine independent BDD checks.
    for binary in ['main_debug','main_release']:
        p=run('c17a_'+binary,f'-net {ROOT}/input/circuit/c17a.v\n',binary=binary,extra={'GT_BDD':'1'})
        assert probabilities(p)==probabilities(ROOT/'expected/c17a_result.csv')
    for circuit in ['s27','s208','s298','s344','s510','s641','s713','s1494','s5378']:
        source=ROOT/f'verification/gt_bdd/{circuit}.set'
        text='\n'.join(line for line in source.read_text().splitlines()
                       if line.split() and line.split()[0] in ('-net','-fault','-limit','-saf','-tdf'))
        run('default_'+circuit,text,extra={'GT_BDD':'1'})
        print('default SAF '+circuit+': ALL VERIFIED',flush=True)
    print('PASS: power settings, exact reification, Debug/Release, BDD fallback, golden and nine default GT cases')

if __name__=='__main__':main()
