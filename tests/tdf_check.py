#!/usr/bin/env python3
"""Independent exhaustive LOC simulation from original Verilog (no SAT/BDD oracle).

python3 tests/tdf_check.py --binary build/main_release --debug build/main_debug
Add --capture build/fault_pool_checks/main_capture to verify every X expansion.
Generate that isolated capture binary with tests/fault_pool_build.py.
"""
import argparse
from collections import Counter
import csv
from functools import lru_cache
from decimal import Decimal
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
IDENTIFIER = r'[A-Za-z_][A-Za-z0-9_]*'
class Circuit:
    @staticmethod
    def gate(kind, values, mask):
        if kind in ('INV', 'BUF'):
            return values[0] ^ mask if kind == 'INV' else values[0]
        value = mask if kind in ('AND', 'NAND') else 0
        for item in values:
            if kind in ('AND', 'NAND'):
                value &= item
            elif kind in ('OR', 'NOR'):
                value |= item
            else:
                value ^= item
        return value ^ mask if kind in ('NAND', 'NOR', 'XNOR') else value


def truth_vectors(names):
    count = 1 << len(names)
    mask = (1 << count) - 1
    # Input i alternates blocks of 2**i zeroes and ones over all assignments.
    vectors = {}
    for i, name in enumerate(names):
        block = 1 << i
        pattern = ((1 << block) - 1) << block
        width = 2 * block
        while width < count:
            pattern |= pattern << width
            width *= 2
        vectors[name] = pattern
    return mask, vectors


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
                base={'EXOR':'XOR','EXNOR':'XNOR'}.get(base,base)
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


@lru_cache(maxsize=4)
def reference_events(path):
    net = SequentialCircuit(path)
    names = net.pi + [q + '_1t' for q, _ in net.flops]
    assert len(names) <= 20
    mask, vectors = truth_vectors(names)
    inputs = {p: vectors[p] for p in net.pi}
    first = net.evaluate(inputs, {q: vectors[q + '_1t'] for q, _ in net.flops}, mask)
    state = net.capture(first)
    second = net.evaluate(inputs, state, mask)
    good = net.capture(second)
    events = {}
    for name, site in net.sites.items():
        source = net.monitored[name]
        for stuck, kind in ((0, 'STR'), (1, 'STF')):
            excitation = (first[source] & (mask ^ second[source])) if stuck else (
                (mask ^ first[source]) & second[source])
            faulty = net.evaluate(inputs, state, mask, site, stuck)
            observed = net.capture(faulty, site, stuck, mask)
            difference = 0
            for q in good:
                difference |= good[q] ^ observed[q]
            events[name, kind] = excitation & difference
    return net, mask, names, vectors, events


def verify(path, directory, capture=False):
    net, mask, names, vectors, events = reference_events(path)
    rows = list(csv.DictReader((directory / 'fdp.csv').open()))
    actual = {(r['net_name'], r['f_type']): r for r in rows}
    assert len(actual) == len(rows) and set(actual) == set(events), path
    for key, row in actual.items():
        expected = Decimal(events[key].bit_count()) / Decimal(mask.bit_count())
        assert abs(Decimal(row['fdp']) - expected) <= Decimal('5e-11'), (path, key, row, expected)
        assert row['complete'] in ('1', ''), (path, key, row)
    count = 0
    if capture:
        covers = list(directory.glob('*.cover'))
        assert len(covers) == sum(r['complete'] == '1' for r in rows)
        for cover in covers:
            lines = cover.read_text().splitlines()
            name, stuck, complete, n, nc = lines[0].split()
            assert complete == '1' and int(n) == len(names) and int(nc) == len(lines) - 2
            order = lines[1].split()
            # Initial state names and PI aliases follow the production PI order.
            inverse = {net.alias[p]: p for p in net.pi}
            inverse.update({net.alias[q] + '_1t': q + '_1t' for q, _ in net.flops})
            assert set(order) == set(inverse)
            key = name, 'STF' if int(stuck) else 'STR'
            union = 0
            for cube in lines[2:]:
                assert len(cube) == len(order) and set(cube) <= set('01X')
                expanded = mask
                for p, bit in zip(order, cube):
                    if bit != 'X':
                        v = vectors[inverse[p]]
                        expanded &= v if bit == '1' else mask ^ v
                assert not (expanded & (mask ^ events[key])), (path, key, cube, 'unsound cube')
                union |= expanded
                count += 1
            assert union == events[key], (path, key, 'incomplete cover')
    return {'circuit': path.stem, 'inputs': len(names), 'assignments': mask.bit_count(),
            'faults': len(rows), 'verified_cubes': count}


def run(binary, directory, circuit, extra='', cli=(), capture=False, success=True, model='-tdf\n'):
    directory.mkdir(parents=True, exist_ok=True)
    config = directory / 'run.set'
    config.write_text(model + f'-net {circuit}\n-fdp {directory}/fdp.csv\n'
                      f'-log {directory}/time.log\n' + extra)
    env = {k: v for k, v in os.environ.items() if k != 'MDC_NOPROP'}
    env['FDP_NORMAL_SCOPE_VALIDATE'] = '1'
    if capture:
        for old in directory.glob('*.cover'): old.unlink()
        env['BASELINE_COVER_DIR'] = str(directory)
    result = subprocess.run([str(binary), '-set', str(config), *cli], cwd=ROOT / 'build',
                            env=env, capture_output=True, text=True, timeout=120)
    (directory / 'stdout.txt').write_text(result.stdout)
    (directory / 'stderr.txt').write_text(result.stderr)
    assert (result.returncode == 0) == success, (directory, result.returncode, result.stderr[-1000:])
    return directory


def equal(a, b):
    assert (a / 'fdp.csv').read_bytes() == (b / 'fdp.csv').read_bytes(), (a, b)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build/main_release')
    parser.add_argument('--debug', type=Path)
    parser.add_argument('--capture', type=Path)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/tdf_checks')
    args = parser.parse_args()
    binary = args.binary.resolve()
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=True)
    fixture = output / 'boundary.v'
    fixture.write_text('''module boundary(CLK,a,b,y);
input CLK,a,b;
output y;
DFF f0(.D(a),.CP(CLK),.Q(q0));
DFF f1(.D(d),.CP(CLK),.Q(q1));
DFF f2(.D(d),.CP(CLK),.Q(q2));
INV g0(.A(q0),.Z(i));
BUF g1(.A(i),.Z(d));
EXOR2 g2(.A(q1),.B(q2),.Z(x));
EXNOR2 g3(.A(x),.B(b),.Z(y));
endmodule
''')
    reports = []
    for circuit in (ROOT / 'input/circuit/tdf/s27.v', ROOT / 'input/circuit/tdf/s208.v', fixture):
        serial = run(binary, output / (circuit.stem + '_serial'), circuit)
        reports.append(verify(circuit, serial))
        parallel = run(binary, output / (circuit.stem + '_parallel'), circuit, '-jobs 4\n')
        equal(serial, parallel)
        if args.debug:
            debug = run(args.debug.resolve(), output / (circuit.stem + '_debug'), circuit)
            equal(serial, debug)
        if args.capture:
            captured = run(args.capture.resolve(), output / (circuit.stem + '_capture'), circuit,
                           '-jobs 4\n', capture=True)
            equal(serial, captured)
            reports.append(verify(circuit, captured, capture=True))
        # Explicit lists contain all sites, independently of representative collapse.
        faults = output / (circuit.stem + '.faults')
        faults.write_text(''.join(f'{name}\t{kind}\n' for name, kind in reference_events(circuit)[-1]))
        explicit = run(binary, output / (circuit.stem + '_explicit'), circuit, f'-fault {faults}\n')
        reports.append(verify(circuit, explicit))
    s27 = ROOT / 'input/circuit/tdf/s27.v'
    # Last encountered model option wins, for both CLI and .set.
    override = run(binary, output / 'cli_tdf', s27, '-saf\n', cli=('-tdf',))
    equal(output / 's27_serial', override)
    override = run(binary, output / 'set_tdf', s27, '-saf\n-tdf\n')
    equal(output / 's27_serial', override)
    for label, extra, cli in (('default_saf', '', ()), ('set_saf', '-saf\n', ()),
                              ('cli_saf', '', ('-saf',))):
        circuit = ROOT / 'input/circuit/saf/c17a.v'
        directory = output / label; directory.mkdir(exist_ok=True)
        run(binary, directory, circuit, extra, cli, model='' if label == 'default_saf' else '-tdf\n')
        projection = lambda p: sorted((r['net_name'],r['f_type'],r['fdp']) for r in csv.DictReader(p.open()))
        assert projection(directory / 'fdp.csv') == projection(ROOT / 'tests/expected/saf/c17a_result.csv')
    unsupported = output / 'reset_ff.v'
    unsupported.write_text(s27.read_text().replace('DFF ', 'RDFF ').replace('.D(G10)', '.CD(G0), .D(G10)')
                          .replace('.D(G11)', '.CD(G0), .D(G11)').replace('.D(G13)', '.CD(G0), .D(G13)'))
    for label, circuit, extra, message in (
        ('combinational', ROOT / 'input/circuit/saf/c17a.v', '', '順序回路'),
        ('reset_ff', unsupported, '', '素の DFF'),
        ('wrong_type', s27, '-fault ' + str(output / 'bad.faults') + '\n', 'STR/STF'),
        ('frame1_site', s27, '-fault ' + str(output / 'frame1.faults') + '\n', '2nd-frame')):
        (output / 'bad.faults').write_text('G5 sa0\n')
        (output / 'frame1.faults').write_text('G5_1t STR\n')
        directory = run(binary, output / label, circuit, extra, success=False)
        assert message in (directory / 'stdout.txt').read_text()
    directory = run(binary, output / 'saf_wrong_type', s27,
                    f'-saf\n-fault {ROOT}/input/fault/tdf/s27_tdf.txt\n', success=False)
    assert 'unexpected type' in (directory / 'stdout.txt').read_text()
    report = {'source': 'independent exhaustive two-frame Verilog simulation',
              'cases': reports, 'serial_parallel_debug_equal': True if args.debug else None, 'saf_c17_golden': True,
              'settings_and_invalid_inputs': True}
    (output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
