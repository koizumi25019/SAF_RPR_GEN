#!/usr/bin/env python3
"""Compare SAF CSV rows and saved cubes with exhaustive gate simulation."""
import argparse
import csv
from decimal import Decimal
from itertools import product
import os
from pathlib import Path
import subprocess
import tempfile

from cube_output_check import read_cubes, cube_texts


def evaluate(inputs, gates, vector, fault=None):
    values = dict(zip(inputs, vector))
    if fault and fault[0] in values:
        values[fault[0]] = int(fault[1][-1])
    for name, kind, sources in gates:
        bits = [values[source] for source in sources]
        if kind in ('AND', 'NAND'):
            value = int(all(bits))
        elif kind in ('OR', 'NOR'):
            value = int(any(bits))
        else:
            value = bits[0]
        if kind in ('NAND', 'NOR', 'INV'):
            value = 1 - value
        values[name] = int(fault[1][-1]) if fault and fault[0] == name else value
    return values[gates[-1][0]]


def detection_events(inputs, gates):
    vectors = list(product((0, 1), repeat=len(inputs)))
    sites = [*inputs, *(name for name, _, _ in gates)]
    return {
        (site, kind): {vector for vector in vectors
                       if evaluate(inputs, gates, vector) != evaluate(inputs, gates, vector, (site, kind))}
        for site in sites for kind in ('sa0', 'sa1')
    }


def write_circuit(path, inputs, gates):
    output = gates[-1][0]
    lines = [f'module fixture({",".join([*inputs, output])});',
             f'input {",".join(inputs)};', f'output {output};',
             f'wire {",".join([*inputs, *(name for name, _, _ in gates)])};']
    for index, (name, kind, sources) in enumerate(gates):
        cell = kind if kind in ('BUF', 'INV') else kind + str(len(sources))
        ports = [f'.{pin}({source})' for pin, source in zip('ABC', sources)]
        lines.append(f'{cell} G{index}({",".join([*ports, f".Z({name})"])});')
    path.write_text('\n'.join([*lines, 'endmodule', '']))


def run(binary, directory, circuit, extra='', cli=(), env_extra=None, success=True):
    directory.mkdir()
    config = directory / 'run.set'
    config.write_text(f'-saf\n-net {circuit}\n-fdp {directory}/fdp.csv\n'
                      f'-log {directory}/time.log\n-cube {directory}/cubes\n' + extra)
    env = {key: value for key, value in os.environ.items()
           if key not in ('MDC_NODOM', 'MDC_NOPROP')}
    env.update(env_extra or {})
    result = subprocess.run([str(binary), '-set', str(config), *cli],
                            env=env, capture_output=True, text=True, timeout=20)
    assert (result.returncode == 0) == success, (directory, result.stdout, result.stderr)
    return result


def verify(directory, inputs, events):
    with (directory / 'fdp.csv').open() as stream:
        reader = csv.DictReader(stream)
        assert reader.fieldnames == ['net_name', 'f_type', 'cube_cnt', 'complete', 'fdp'], directory
        rows = list(reader)
    assert all(None not in row and None not in row.values() for row in rows), directory
    assert {(row['net_name'], row['f_type']) for row in rows} == set(events), directory
    representatives = {}
    for row in rows:
        key = row['net_name'], row['f_type']
        expected = Decimal(len(events[key])) / (2 ** len(inputs))
        assert abs(Decimal(row['fdp']) - expected) < Decimal('5e-11'), (directory, key, row['fdp'], expected)
        if row['complete']:
            assert row['complete'] == '1', (directory, row)
            representatives[key] = row
    order, blocks = read_cubes(directory / 'cubes')
    assert set(order) == set(inputs), (directory, order)
    assert set(blocks) == {key for key in representatives if events[key]}, directory
    for key, cubes in blocks.items():
        covered = set()
        for vector in product((0, 1), repeat=len(inputs)):
            assignment = dict(zip(inputs, vector))
            for cube in cubes:
                if all(bit == 'X' or bit == str(assignment[name]) for name, bit in zip(order, cube)):
                    covered.add(vector)
                    break
        assert covered == events[key], (directory, key, covered, events[key])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--debug', type=Path, required=True)
    args = parser.parse_args()
    cases = [(kind.lower() + str(arity), tuple('abc'[:arity]), [('y', kind, tuple('abc'[:arity]))])
             for kind, arity in (('AND', 2), ('NAND', 2), ('OR', 2), ('NOR', 2),
                                 ('NAND', 3), ('NOR', 3), ('BUF', 1), ('INV', 1))]
    cases += [
        ('nand_inv', ('a', 'b'), [('n', 'NAND', ('a', 'b')), ('y', 'INV', ('n',))]),
        ('nor_buf_inv', ('a', 'b'), [('n', 'NOR', ('a', 'b')), ('m', 'BUF', ('n',)),
                                   ('y', 'INV', ('m',))]),
        ('nand_and', ('a', 'b', 'c'), [('n', 'NAND', ('a', 'b')), ('y', 'AND', ('n', 'c'))]),
        ('nor_or', ('a', 'b', 'c'), [('n', 'NOR', ('a', 'b')), ('y', 'OR', ('n', 'c'))]),
    ]
    with tempfile.TemporaryDirectory(prefix='fdp-equivalence-') as temporary:
        root = Path(temporary)
        for binary in dict.fromkeys((args.binary.resolve(), args.debug.resolve())):
            for label, inputs, gates in cases:
                circuit = root / (label + '.v')
                write_circuit(circuit, inputs, gates)
                events = detection_events(inputs, gates)
                baseline = None
                for jobs in (1, 4):
                    directory = root / f'{binary.name}_{label}_{jobs}'
                    run(binary, directory, circuit, f'-jobs {jobs}\n')
                    verify(directory, inputs, events)
                    if baseline is None:
                        baseline = directory
                    else:
                        assert (directory / 'fdp.csv').read_bytes() == (baseline / 'fdp.csv').read_bytes()
                        assert cube_texts(directory / 'cubes') == cube_texts(baseline / 'cubes')
                if label in ('nand2', 'nor2', 'nand_and', 'nor_or'):
                    faults = root / (label + '.fault')
                    faults.write_text(''.join(f'{name} {kind}\n' for name, kind in events))
                    for jobs in (1, 4):
                        directory = root / f'{binary.name}_{label}_explicit_{jobs}'
                        run(binary, directory, circuit, f'-jobs {jobs}\n-fault {faults}\n')
                        verify(directory, inputs, events)
                # The removed environment switch cannot change the serial calculation.
                if label == 'nand2':
                    for value in ('0', '1'):
                        directory = root / f'{binary.name}_legacy_env_{value}'
                        run(binary, directory, circuit, env_extra={'MDC_NODOM': value})
                        assert (directory / 'fdp.csv').read_bytes() == (baseline / 'fdp.csv').read_bytes()
                    for mode in ('cli', 'set'):
                        for value in ('on', 'off'):
                            directory = root / f'{binary.name}_removed_{mode}_{value}'
                            result = run(binary, directory, circuit,
                                         extra=f'-dom_reuse {value}\n' if mode == 'set' else '',
                                         cli=('-dom_reuse', value) if mode == 'cli' else (), success=False)
                            assert '-dom_reuse' in result.stdout and 'not expected' in result.stdout
    print('SAF equivalence: 12 circuits, all input assignments and faults, CSV/cubes, '
          'serial/parallel, Debug/Release, removed reuse settings: PASS')


if __name__ == '__main__':
    main()
