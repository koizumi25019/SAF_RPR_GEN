#!/usr/bin/env python3
"""Independent exhaustive LOC/TDF simulation of CORE cubes, with/without power.
Requires NumPy for direct per-assignment switching counts; no SAT/BDD in the
reference simulator. The C executable supplies the cubes being verified.
"""
from collections import Counter
import argparse
import csv
from decimal import Decimal
import json
import os
from pathlib import Path
import re
import subprocess
import numpy as np
from simulate_expanded import Circuit, IDENTIFIER, ROOT, build_capture_binary, truth_vectors

class SequentialCircuit:
    def __init__(self, path):
        text = re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(), flags=re.S)
        def ports(kind):
            return [name for decl in re.findall(r'\b'+kind+r'\s+([^;]+);',text)
                    for name in re.findall(IDENTIFIER,decl)]
        self.pi,self.po=ports('input'),ports('output')
        gates,clocks=[],set()
        self.flops=[]
        for kind,instance,body in re.findall(r'('+IDENTIFIER+r')\s+('+IDENTIFIER+r')\s*\((.*?)\)\s*;',text,re.S):
            if kind=='module':continue
            pins=dict(re.findall(r'\.([A-Z]+)\s*\(\s*('+IDENTIFIER+r')\s*\)',body))
            if kind=='DFF':
                clocks.add(pins['CP']);self.flops.append((pins['Q'],pins['D']))
                gates.append(('DFF',(pins['D'],),pins['Q']))
            else:
                base=kind.rstrip('0123456789')
                assert base in ('INV','BUF','AND','NAND','OR','NOR','XOR','XNOR'),kind
                output=pins.pop('Z');inputs=tuple(pins[p] for p in sorted(pins))
                gates.append((base,inputs,output))
        self.pi=[p for p in self.pi if p not in clocks]
        assert self.flops
        signals=set(self.pi)|{g[2] for g in gates}
        uses=Counter(source for _,inputs,_ in gates for source in inputs)
        names={s:s for s in signals}
        branches=[]
        for _,inputs,output in gates:
            for pin,source in enumerate(inputs):
                if uses[source]+(source in self.po)>=2:
                    branches.append((names[source]+'_'+names[output]+'_'+chr(65+pin),output,pin,source))
                    if source in self.po and names[source]==source:names[source]=source+'_stem'
        self.alias=names
        self.sites={alias:('stem',s) for s,alias in names.items()}
        self.monitored={alias:s for s,alias in names.items()}
        for p in self.po:
            if names[p]!=p:self.sites[p]=('po',p);self.monitored[p]=p
        self.branch={}
        for label,output,pin,source in branches:
            assert label not in self.sites
            self.sites[label]=('branch',output,pin);self.monitored[label]=source
            self.branch[output,pin]=label
        self.sources={output:inputs for _,inputs,output in gates}
        pending=[g for g in gates if g[0]!='DFF'];done=set(self.pi)|{q for q,_ in self.flops};self.gates=[]
        while pending:
            remaining=[]
            for g in pending:
                if set(g[1])<=done:self.gates.append(g);done.add(g[2])
                else:remaining.append(g)
            assert len(remaining)<len(pending),'Unresolved sequential drivers'
            pending=remaining
    def evaluate(self,inputs,state,mask,site=None,stuck=0):
        values=dict(inputs,**state);constant=mask if stuck else 0
        if site and site[0]=='stem' and site[1] in values:values[site[1]]=constant
        for kind,sources,output in self.gates:
            args=[values[p] for p in sources]
            if site and site[0]=='branch' and site[1]==output:args[site[2]]=constant
            values[output]=constant if site==('stem',output) else Circuit.gate(kind,args,mask)
        return values
    def capture(self,values,site=None,stuck=0,mask=0):
        return {q:(mask if stuck else 0) if site==('branch',q,0) else values[d] for q,d in self.flops}

def verify(path,directory,percent):
    net=SequentialCircuit(path)
    covers=sorted(directory.glob('*.cover'));assert covers
    order=covers[0].read_text().splitlines()[1].split();n=len(order);assert n<=20
    expected={net.alias[p] for p in net.pi}|{net.alias[q]+'_1t' for q,_ in net.flops}
    assert set(order)==expected,(order,expected)
    mask,vectors=truth_vectors(order);count=1<<n
    inputs={p:vectors[net.alias[p]] for p in net.pi}
    state1={q:vectors[net.alias[q]+'_1t'] for q,_ in net.flops}
    first=net.evaluate(inputs,state1,mask)
    second=net.evaluate(inputs,net.capture(first),mask)
    changes=np.zeros(count,dtype=np.uint32)
    for source in net.monitored.values():
        delta=first[source]^second[source]
        bits=np.unpackbits(np.frombuffer(delta.to_bytes((count+7)//8,'little'),dtype=np.uint8),bitorder='little')[:count]
        changes+=bits
    budget=len(net.monitored)*percent//100 if percent is not None else len(net.monitored)
    legal=mask if percent is None else int.from_bytes(np.packbits(changes<=budget,bitorder='little').tobytes(),'little')&mask
    if percent is not None:
        recorded=re.search(r'\[POWER\] signals=(\d+) threshold=(\d+)% budget=(\d+)',(directory/'stderr.txt').read_text())
        assert tuple(map(int,recorded.groups()))==(len(net.monitored),percent,budget)
    records=list(csv.DictReader((directory/'fdp.csv').open()))
    all_rows={(r['net_name'],int(r['f_type']=='STF')):r for r in records}
    assert len(all_rows)==len(records)
    assert set(all_rows)=={(name,stuck) for name in net.monitored for stuck in (0,1)}
    rows={key:r for key,r in all_rows.items() if r['complete'] in ('0','1')}
    summary=dict(circuit=path.stem,threshold_percent=percent,signals=len(net.monitored),budget=budget,
                 inputs=n,representatives=len(rows),csv_faults=len(all_rows),legal_inputs=legal.bit_count(),cubes=0,
                 core_expanded_patterns=0,prime_expanded_patterns=0,power_sensitive_faults=0,
                 non_detecting_expansions=0,non_exciting_expansions=0,power_violating_expansions=0)
    seen=set();probe=None
    def expand(cube):
        assert len(cube)==n and set(cube)<=set('01X')
        result=mask
        for p,bit in zip(order,cube):
            if bit!='X':result&=vectors[p] if bit=='1' else mask^vectors[p]
        assert result.bit_count()==1<<cube.count('X')
        return result
    reference_capture=net.capture(second)
    def event(name,stuck):
        site=net.sites[name];source=net.monitored[name]
        initial,current=first[source],second[source]
        excitation=(initial&(mask^current)) if stuck else ((mask^initial)&current)
        faulty=net.evaluate(inputs,net.capture(first),mask,site,stuck)
        fault_capture=net.capture(faulty,site,stuck,mask)
        detection=0
        for q,_ in net.flops:detection|=reference_capture[q]^fault_capture[q]
        return detection,excitation,detection&excitation&legal
    for cover in covers:
        lines=cover.read_text().splitlines();name,stuck,ni=lines[0].split();stuck=int(stuck)
        assert int(ni)==n and lines[1].split()==order
        key=name,stuck;assert key in rows and name in net.sites and key not in seen,key
        seen.add(key)
        detection,excitation,accepted=event(name,stuck)
        if detection&excitation&(mask^legal):summary['power_sensitive_faults']+=1
        union=0;nc=0
        for line in lines[2:]:
            core,prime=line.split()
            for stage,cube in [('core',core),('prime',prime)]:
                expanded=expand(cube)
                for condition,label in [(detection,'detection'),(excitation,'excitation'),(legal,'power')]:
                    missed=expanded&(mask^condition)
                    if missed:
                        witness=(missed&-missed).bit_length()-1
                        raise AssertionError((path.stem,percent,key,stage,label,cube,witness))
                summary[stage+'_expanded_patterns']+=1<<cube.count('X')
            union|=expand(prime);nc+=1
        assert rows[key]['complete']=='1' and union==accepted,(path.stem,percent,key,'cover mismatch')
        assert nc==int(rows[key]['cube_cnt'])
        assert abs(Decimal(rows[key]['fdp'])-Decimal(accepted.bit_count())/Decimal(count))<=Decimal('5e-11')
        summary['cubes']+=nc
        if path.stem=='power_guard' and net.monitored[name] in ('q','d','o') and not stuck and accepted:
            probe=dict(fault=name,core_cubes=[l.split()[0] for l in lines[2:]],prime_cubes=[l.split()[1] for l in lines[2:]],
                       detecting_inputs=accepted.bit_count(),max_switching_count_in_detection=int(changes[[bool((accepted>>i)&1) for i in range(count)]].max()) if accepted else 0)
    assert seen==rows.keys()
    for (name,stuck),row in all_rows.items():
        accepted=event(name,stuck)[2]
        expected=Decimal(accepted.bit_count())/Decimal(count)
        assert abs(Decimal(row['fdp'])-expected)<=Decimal('5e-11'),(name,stuck,'CSV echo FDP mismatch')
    if probe:summary['probe']=probe
    (directory/'simulation.json').write_text(json.dumps(summary,indent=2)+'\n')
    return summary

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'output/tdf_power_checks')
    output=parser.parse_args().output.resolve();output.mkdir(parents=True,exist_ok=True)
    build=output/'build';build.mkdir(exist_ok=True);binary=build_capture_binary(build)
    fixture=output/'power_guard.v'
    fixture.write_text('module power_guard(a,b,CLK,r);\ninput a,b,CLK;\noutput r;\n'
        'DFF f0(.D(a),.CP(CLK),.Q(q));\nDFF f1(.D(b),.CP(CLK),.Q(r));\n'
        'DFF f2(.D(d),.CP(CLK),.Q(s));\nBUF g0(.A(q),.Z(d));\nendmodule\n')
    sources=[(ROOT/'input/circuit/s27.v',[None,0,20,50,100]),
             (ROOT/'input/circuit/s208.v',[None,20,50,100]),(fixture,[None,34])]
    hooks=set()
    for source in (ROOT/'src').rglob('*.c'):hooks.update(re.findall(r'getenv\("([A-Z0-9_]+)"\)',source.read_text(errors='replace')))
    env={k:v for k,v in os.environ.items() if k not in hooks}
    env['GT_BDD']='1'
    results=[]
    for source,thresholds in sources:
        for percent in thresholds:
            label='off' if percent is None else f'p{percent}'
            directory=output/f'{source.stem}_{label}';directory.mkdir(exist_ok=True)
            assert not list(directory.glob('*.cover')),'Use a fresh output directory'
            settings=directory/'run.set'
            settings.write_text(f'-tdf\n-net {source}\n-dc_method core\n-dom_reuse off\n-core_verify on\n'
                f'-low_power {"off" if percent is None else "on"}\n'+('' if percent is None else f'-wsa_threshold {percent}\n')+
                f'-fdp {directory}/fdp.csv\n-log {directory}/run.log\n')
            with (directory/'stdout.txt').open('w') as stdout,(directory/'stderr.txt').open('w') as stderr:
                subprocess.run([str(binary),'-set',str(settings)],cwd=ROOT,env=dict(env,PAPER_CORE_CAPTURE_DIR=str(directory)),
                               stdout=stdout,stderr=stderr,check=True,timeout=240)
            assert 'ALL VERIFIED' in (directory/'stderr.txt').read_text()
            result=verify(source,directory,percent);results.append(result);print(json.dumps(result),flush=True)
    guard_off=next(r for r in results if r['circuit']=='power_guard' and r['threshold_percent'] is None)
    guard_on=next(r for r in results if r['circuit']=='power_guard' and r['threshold_percent']==34)
    assert guard_off['probe']['detecting_inputs']>guard_on['probe']['detecting_inputs']>0
    assert max(c.count('X') for c in guard_off['probe']['prime_cubes'])>max(c.count('X') for c in guard_on['probe']['prime_cubes'])
    for circuit in ['s27','s208']:
        cases=[r for r in results if r['circuit']==circuit]
        off=next(r for r in cases if r['threshold_percent'] is None)
        full=next(r for r in cases if r['threshold_percent']==100)
        assert off['legal_inputs']==full['legal_inputs']
    (output/'summary.json').write_text(json.dumps(results,indent=2)+'\n')
    print('PASS: TDF core/prime expansions detect, excite and respect power; full covers/FDP match independent simulation')

if __name__=='__main__':main()
