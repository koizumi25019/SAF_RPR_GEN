#!/usr/bin/env python3
"""Small SAF feasibility check; does not implement an ASG production mode.

Build a four-bit symbolic generator in front of the supplied s27_C CUT,
then compare the existing SAT/XID/BDD pipeline with scalar exhaustive
simulation. Generated netlists and native logs live in a temporary directory.
"""
import argparse
import csv
import io
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
from fractions import Fraction
from functools import reduce
from operator import xor

ROOT = Path(__file__).resolve().parents[2]


def parse_cut(path):
    source = path.read_text()
    ports = lambda kind: re.search(rf'\b{kind}\s+([^;]+);', source)[1].replace(' ', '').split(',')
    gates = []
    for typ, name, body in re.findall(r'(NAND2|NOR2|AND2|OR2|INV)\s+(\w+)\s*\((.*?)\);', source):
        pins = dict(re.findall(r'\.(\w+)\((\w+)\)', body))
        gates.append((typ, name, [pins[k] for k in ('A', 'B') if k in pins], pins['Z']))
    return source, ports('input'), ports('output'), gates


def simulate_cut(values, outputs, gates, fault=None):
    values = dict(values)
    if fault and fault[0] in values:
        values[fault[0]] = fault[1]
    pending = list(gates)
    while pending:
        ready = [g for g in pending if all(x in values for x in g[2])]
        if not ready:
            raise ValueError('CUT has missing inputs or a cycle')
        for gate in ready:
            typ, _, ins, out = gate
            bits = [values[x] for x in ins]
            if typ in ('AND2', 'NAND2'):
                value = int(all(bits)) ^ int(typ == 'NAND2')
            elif typ in ('OR2', 'NOR2'):
                value = int(any(bits)) ^ int(typ == 'NOR2')
            else:
                value = 1 ^ bits[0]
            values[out] = fault[1] if fault and fault[0] == out else value
            pending.remove(gate)
    return tuple(values[x] for x in outputs)


def rank(masks):
    basis = {}
    for value in masks:
        while value:
            pivot = value.bit_length() - 1
            if pivot not in basis:
                basis[pivot] = value
                break
            value ^= basis[pivot]
    return len(basis)


def generator_masks(taps, layout, inputs):
    # Matches ASG polynomial.h case 4 (0xc), shift toward higher indices.
    state = [1 << i for i in range(4)]
    mapping = {}
    for row in layout:
        for channel, net in enumerate(row):
            if net in inputs:
                mapping[net] = reduce(xor, (state[t - 1] for t in taps[channel]), 0)
        state = [state[2] ^ state[3], *state[:-1]]
    if set(mapping) != set(inputs):
        raise ValueError('Every CUT input must be driven')
    return mapping


def scalar_pattern(seed, taps, layout, inputs):
    # Separate concrete simulation, independent of the symbolic XOR masks.
    state = [(seed >> i) & 1 for i in range(4)]
    values = {}
    for row in layout:
        for channel, net in enumerate(row):
            if net in inputs:
                values[net] = sum(state[t - 1] for t in taps[channel]) % 2
        state = [state[2] ^ state[3], state[0], state[1], state[2]]
    return values


def composed_netlist(source, inputs, outputs, mapping):
    seed_names = [f'S{i}' for i in range(4)]
    generator = []
    temps = []
    for net in inputs:
        literals = [seed_names[i] for i in range(4) if mapping[net] & (1 << i)]
        if not literals:
            raise ValueError('This diagnostic uses nonconstant generator outputs')
        acc = literals[0]
        if len(literals) == 1:
            generator.append(f'BUF gen_{net} (.A({acc}), .Z({net}));')
        for i, literal in enumerate(literals[1:]):
            out = net if i == len(literals) - 2 else f'gen_{net}_{i}'
            if out != net:
                temps.append(out)
            generator.append(f'EXOR2 gen_{net}_{i} (.A({acc}), .B({literal}), .Z({out}));')
            acc = out
    source = re.sub(r'module\s+\w+\s*\([^;]+;', 'module asg_probe (' + ', '.join(seed_names + outputs) + ');', source)
    source = re.sub(r'\binput\s+[^;]+;', 'input ' + ', '.join(seed_names) + ';\nwire ' + ', '.join(inputs + temps) + ';', source)
    return source.replace('endmodule', '\n'.join(generator) + '\nendmodule')


def run_case(directory, binary, name, source, inputs, outputs, gates, taps, layout):
    mapping = generator_masks(taps, layout, inputs)
    faults = [(net, stuck) for net in inputs + [g[3] for g in gates] for stuck in (0, 1)]
    populations = {}
    for seed in range(16):
        values = scalar_pattern(seed, taps, layout, inputs)
        symbolic = {net: (mask & seed).bit_count() % 2 for net, mask in mapping.items()}
        assert values == symbolic, (seed, values, symbolic)
        populations[seed] = values
    expected = {}
    for fault in faults:
        detecting = [seed for seed, values in populations.items()
                     if simulate_cut(values, outputs, gates) != simulate_cut(values, outputs, gates, fault)]
        uniform = sum(simulate_cut(dict(zip(inputs, ((word >> i) & 1 for i in range(len(inputs))))), outputs, gates)
                      != simulate_cut(dict(zip(inputs, ((word >> i) & 1 for i in range(len(inputs))))), outputs, gates, fault)
                      for word in range(1 << len(inputs)))
        expected[fault] = {'detecting_seed_count': len(detecting), 'zero_seed_detects': 0 in detecting,
                           'seed_fdp_all': str(Fraction(len(detecting), 16)),
                           'seed_fdp_nonzero': str(Fraction(len(detecting) - int(0 in detecting), 15)),
                           'free_input_fdp': str(Fraction(uniform, 1 << len(inputs)))}
    netfile, faultfile, csvfile = (directory / f'{name}.{suffix}' for suffix in ('v', 'faults', 'csv'))
    netfile.write_text(composed_netlist(source, inputs, outputs, mapping))
    faultfile.write_text(''.join(f'{net}\tsa{stuck}\n' for net, stuck in faults))
    setfile = directory / f'{name}.set'
    setfile.write_text(f'-net {netfile}\n-fault {faultfile}\n-fdp {csvfile}\n-log {directory / (name + ".log")}\n')
    env = {k: v for k, v in os.environ.items() if not k.startswith(('GT_', 'MAXDC', 'MAXHAM', 'DUAL', 'PCOUNT', 'SPLIT', 'MDC_', 'AIG_', 'XID_', 'CUBE_', 'BDD_', 'DUMP_', 'TDF_'))}
    env.update(GT_BDD='1', MDC_NODOM='1')
    proc = subprocess.run([str(binary), '-set', str(setfile)], cwd=ROOT / 'build', env=env,
                          capture_output=True, text=True, timeout=30)
    if proc.returncode:
        raise RuntimeError(f'Native FDP failed ({proc.returncode}):\n{proc.stdout}\n{proc.stderr}')
    summary = [line for line in proc.stderr.splitlines() if '[GT] summary' in line]
    assert summary and 'ALL VERIFIED' in summary[-1], proc.stderr
    actual = {(row['net_name'], int(row['f_type'][-1])): row
              for row in csv.DictReader(io.StringIO(csvfile.read_text())) if row['complete']}
    assert set(actual) == set(expected), (set(actual), set(expected))
    for fault, result in expected.items():
        row = actual[fault]
        assert row['complete'] == '1', row
        assert abs(Fraction(row['fdp']) - Fraction(result['seed_fdp_all'])) < Fraction(1, 10**10), (fault, row, result)
    return {'name': name, 'seed_bits': 4, 'feedback_zero_based_taps': [2, 3], 'phase_shifter_one_based_taps': taps,
            'scan_layout_time_rows': layout, 'input_masks': mapping, 'mapping_rank': rank(mapping.values()),
            'reachable_patterns': len({tuple(v[x] for x in inputs) for v in populations.values()}),
            'verified_faults': len(faults), 'gt_summary': summary[-1],
            'faults': {f'{net}/sa{stuck}': value for (net, stuck), value in expected.items()}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build/main_debug')
    parser.add_argument('--output', type=Path, default=Path(__file__).with_name('results.json'))
    args = parser.parse_args()
    source, inputs, outputs, gates = parse_cut(ROOT / 'ASG/ASG/input/circuit/s27_C.v')
    layout = [re.split(r',\s*', row) for row in (ROOT / 'ASG/ASG/input/scan_chain/s27_C_sc.txt').read_text().splitlines() if row.strip()]
    text = (ROOT / 'ASG/ASG/input/xor_tap/tap_4.txt').read_bytes().decode('cp932')
    taps = [[int(x) for x in row.split(':')[1].split('_') if x] for row in text.splitlines() if row.strip()]
    with tempfile.TemporaryDirectory(prefix='fdp_asg_probe_') as tmp:
        # Existing init.c takes the fourth slash-delimited path component.
        # /tmp/<temporary-dir>/cases/<netlist> satisfies that legacy assumption.
        directory = Path(tmp) / 'cases'
        directory.mkdir()
        results = [run_case(directory, args.binary.resolve(), name, source, inputs, outputs, gates, ps, layout)
                   for name, ps in [('asg_tap4', taps), ('rank_deficient_ps', [[1], [1], [1, 2], [1, 2]])]]
    args.output.write_text(json.dumps({'scope': 'SAF, single expanded pattern, uniform four-bit seeds; nonzero values from scalar enumeration only', 'cases': results}, indent=2) + '\n')
    for case in results:
        print(case['name'], 'rank=', case['mapping_rank'], 'faults=', case['verified_faults'], case['gt_summary'])
        print('G17/sa1:', case['faults']['G17/sa1'])


if __name__ == '__main__':
    main()
