#!/usr/bin/env python3
"""Exhaustive two-valued SAF simulation of captured CORE and prime cubes.

Uses original Verilog gates, no production parser, SAT solver, or BDD oracle.
Each bit of a Python integer represents one fully specified input pattern.
Every input assignment is simulated, including every expansion of every X.
"""
import argparse
from collections import Counter
import csv
from decimal import Decimal
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
CIRCUITS = ('c17a', 's27_C', 's208_C', 's298_C')
IDENTIFIER = r'[A-Za-z_][A-Za-z0-9_]*'


class Circuit:
    def __init__(self, path):
        text = re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(), flags=re.S)
        def ports(kind):
            return [name for decl in re.findall(r'\b' + kind + r'\s+([^;]+);', text)
                    for name in re.findall(IDENTIFIER, decl)]
        self.pi, self.po = ports('input'), ports('output')
        assert self.pi and self.po and len(set(self.pi)) == len(self.pi)
        assert len(self.pi) <= 20, 'Exhaustive validation is limited to small circuits'
        gates = []
        for kind, instance, body in re.findall(r'(' + IDENTIFIER + r')\s+(' + IDENTIFIER + r')\s*\((.*?)\)\s*;', text, re.S):
            if kind == 'module':
                continue
            base = kind.rstrip('0123456789')
            assert base in ('AND', 'OR', 'NAND', 'NOR', 'INV', 'BUF', 'XOR', 'XNOR'), kind
            pins = dict(re.findall(r'\.([A-JZ])\s*\(\s*(' + IDENTIFIER + r')\s*\)', body))
            output = pins.pop('Z')
            assert list(sorted(pins)) == [chr(65 + i) for i in range(len(pins))], instance
            inputs = tuple(pins[p] for p in sorted(pins))
            assert inputs and (base not in ('INV', 'BUF') or len(inputs) == 1)
            gates.append((base, inputs, output))
        assert gates and len({g[2] for g in gates}) == len(gates)
        done, self.gates = set(self.pi), []
        while gates:
            remaining = []
            for gate in gates:
                if set(gate[1]) <= done:
                    self.gates.append(gate)
                    done.add(gate[2])
                else:
                    remaining.append(gate)
            assert len(remaining) < len(gates), 'Unresolved drivers or cycle'
            gates = remaining
        assert set(self.po) <= done
        uses = Counter(p for _, inputs, _ in self.gates for p in inputs)
        self.alias = {p: p + '_stem' if p in self.po and uses[p] else p for p in done}
        self.sites = {alias: ('stem', name) for name, alias in self.alias.items()}
        for p in self.po:
            if self.alias[p] != p:
                self.sites[p] = ('po', p)
        # Gate-input branches, including distinct branches feeding repeated pins.
        self.branch = {}
        for _, inputs, output in self.gates:
            for pin, source in enumerate(inputs):
                if uses[source] + (source in self.po) >= 2:
                    # Names are created in original gate order before a PO stem
                    # is renamed. Verify resolution against actual dumped names.
                    label = source + '_' + output + '_' + chr(65 + pin)
                    self.sites[label] = ('branch', output, pin)
                    self.branch[output, pin] = label

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

    def simulate(self, inputs, mask, site=None, stuck=0):
        values = dict(inputs)
        constant = mask if stuck else 0
        if site and site[0] == 'stem' and site[1] in values:
            values[site[1]] = constant
        for kind, sources, output in self.gates:
            args = [values[p] for p in sources]
            if site and site[0] == 'branch' and site[1] == output:
                args[site[2]] = constant
            result = self.gate(kind, args, mask)
            values[output] = constant if site == ('stem', output) else result
        return {p: constant if site == ('po', p) else values[p] for p in self.po}


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


def verify(circuit, directory, historical=None):
    net = Circuit(ROOT / f'input/circuit/{circuit}.v')
    covers = sorted(directory.glob('*.cover'))
    assert covers
    rows = { (r['net_name'], int(r['f_type'] == 'sa1')): r
             for r in csv.DictReader((directory / 'fdp.csv').open()) if r['complete'] in ('0', '1') }
    if historical is not None:
        old = list(csv.DictReader((historical / f'{circuit}_core_1.csv').open()))
        actual = list(csv.DictReader((directory / 'fdp.csv').open()))
        assert actual == old, 'Instrumentation changed generated patterns or FDP'
    order = covers[0].read_text().splitlines()[1].split()
    assert set(order) == {net.alias[p] for p in net.pi}
    original = {net.alias[p]: p for p in net.pi}
    mask, vectors = truth_vectors(order)
    inputs = {original[p]: v for p, v in vectors.items()}
    good = net.simulate(inputs, mask)
    summary = dict(circuit=circuit, n_inputs=len(order), representatives=len(rows),
                   cubes=0, core_expanded_patterns=0, prime_expanded_patterns=0,
                   exhaustive_fault_pattern_pairs=len(rows) * (1 << len(order)),
                   core_care=0, prime_care=0, unsound_core_cubes=0,
                   unsound_prime_cubes=0, exact_cover_mismatches=0)
    details, seen = [], set()
    def cube_mask(cube):
        assert len(cube) == len(order) and set(cube) <= set('01X')
        value = mask
        for p, bit in zip(order, cube):
            if bit != 'X':
                value &= vectors[p] if bit == '1' else vectors[p] ^ mask
        assert value.bit_count() == 1 << cube.count('X')
        return value
    for path in covers:
        lines = path.read_text().splitlines()
        fault, stuck, n = lines[0].split()
        stuck, n = int(stuck), int(n)
        assert lines[1].split() == order and n == len(order)
        key = fault, stuck
        assert key in rows and key not in seen and fault in net.sites, key
        seen.add(key)
        faulty = net.simulate(inputs, mask, net.sites[fault], stuck)
        detection = 0
        for p in net.po:
            detection |= good[p] ^ faulty[p]
        cover, count, core_expanded, prime_expanded = 0, 0, 0, 0
        for line in lines[2:]:
            extracted, prime = line.split()
            assert all(a == b or b == 'X' for a, b in zip(extracted, prime))
            for stage, cube in [('core', extracted), ('prime', prime)]:
                expanded = cube_mask(cube)
                missed = expanded & (mask ^ detection)
                if missed:
                    witness = (missed & -missed).bit_length() - 1
                    assignment = ''.join(str((witness >> i) & 1) for i in range(n))
                    raise AssertionError((circuit, key, stage, cube, 'non-detecting expansion', assignment))
            core_expanded += 1 << extracted.count('X')
            prime_expanded += 1 << prime.count('X')
            summary['core_care'] += n - extracted.count('X')
            summary['prime_care'] += n - prime.count('X')
            cover |= cube_mask(prime)
            count += 1
        assert rows[key]['complete'] == '1' and cover == detection, (key, 'cover incomplete')
        assert count == int(rows[key]['cube_cnt'])
        expected = Decimal(detection.bit_count()) / Decimal(1 << n)
        assert abs(Decimal(rows[key]['fdp']) - expected) <= Decimal('5e-11'), (key, 'FDP mismatch')
        summary['cubes'] += count
        summary['core_expanded_patterns'] += core_expanded
        summary['prime_expanded_patterns'] += prime_expanded
        details.append(dict(fault=fault, stuck=stuck, cubes=count, detected_patterns=detection.bit_count(),
                            core_expanded_patterns=core_expanded, prime_expanded_patterns=prime_expanded,
                            all_expansions_detect=True, exact_cover=True))
    assert seen == rows.keys()
    stats = re.search(r'cubes=(\d+) solves=\d+ care: input=(\d+) core=(\d+) prime=(\d+)',
                      (directory / 'stderr.txt').read_text())
    cubes, total, core, prime = map(int, stats.groups())
    assert (cubes, total, core, prime) == (summary['cubes'], n * cubes, summary['core_care'], summary['prime_care'])
    (directory / 'simulation.json').write_text(json.dumps(dict(summary=summary, faults=details), indent=2) + '\n')
    return summary


def build_capture_binary(build):
    cmake = (ROOT / 'CMakeLists.txt').read_text()
    sources = re.search(r'set\(SOURCES\s+(.*?)\)', cmake, re.S).group(1).split()
    includes = [ROOT / p for p in ['src','src/fdp','src/fdp/cnf','src/lib','src/netlist','src/opt','src/fdp/xid',
                                  'external/cadical/src','external/cudd','external/cudd/cudd']]
    flags = ['gcc','-std=gnu11','-O3','-DNDEBUG','-fcommon'] + ['-I'+str(p) for p in includes]
    objects = []
    # Compile the current sources, rather than trusting previous build objects.
    for source in sources + ['verification/paper_core/capture_cubes.c']:
        obj = build / (source.replace('/', '_') + '.o')
        subprocess.run(flags + ['-c',str(ROOT/source),'-o',str(obj)],check=True)
        objects.append(str(obj))
    binary = build / 'main_capture'
    subprocess.run(['gcc'] + objects + ['-Wl,--wrap=PaperCoreBuildOracle',
        '-Wl,--wrap=PaperCoreGeneralize','-Wl,--wrap=ccadical_failed',
        str(ROOT/'external/cadical/build/libcadical.a'), str(ROOT/'external/cudd/cudd/.libs/libcudd.a'),
        '-lgmp','-lm','-lstdc++','-o',str(binary)],check=True)
    return binary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--historical',type=Path,help='Optional previous benchmark directory; require identical CSVs')
    parser.add_argument('--output',type=Path,default=ROOT/'output/paper_core_simulation')
    args = parser.parse_args()
    args.output = args.output.resolve()
    if args.historical is not None:
        args.historical = args.historical.resolve()
    args.output.mkdir(parents=True,exist_ok=True)
    build = args.output/'build'
    build.mkdir(exist_ok=True)
    binary = build_capture_binary(build)
    hooks = set()
    for source in (ROOT/'src').rglob('*.c'):
        hooks.update(re.findall(r'getenv\("([A-Z0-9_]+)"\)',source.read_text(errors='replace')))
    env = {k:v for k,v in os.environ.items() if k not in hooks}
    summaries = []
    for circuit in CIRCUITS:
        directory = args.output/circuit
        directory.mkdir(exist_ok=True)
        assert not list(directory.glob('*.cover')), 'Use a fresh output directory'
        settings = directory/'run.set'
        settings.write_text(f'-saf\n-net {ROOT}/input/circuit/{circuit}.v\n'
            f'-dc_method core\n-dom_reuse off\n-core_verify off\n'
            f'-fdp {directory}/fdp.csv\n-log {directory}/run.log\n')
        with (directory/'stdout.txt').open('w') as out, (directory/'stderr.txt').open('w') as err:
            subprocess.run([str(binary),'-set',str(settings)],cwd=ROOT,
                env=dict(env,PAPER_CORE_CAPTURE_DIR=str(directory)),stdout=out,stderr=err,check=True,timeout=120)
        result = verify(circuit,directory,args.historical)
        summaries.append(result)
        print(json.dumps(result),flush=True)
    result = dict(validation='exhaustive two-valued SAF simulation; every X expansion, no sampling',
                  binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                  instrumentation='GNU linker wrappers capture failed core and final cube; production source unchanged',
                  historical_csv_match=args.historical is not None,circuits=summaries)
    (args.output/'summary.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS: all core and prime X expansions detect; all full covers and FDP match simulation')


if __name__ == '__main__':
    main()
