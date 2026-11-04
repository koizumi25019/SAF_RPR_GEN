#!/usr/bin/env python3
"""Verify imported SAF equivalence with exhaustive simulation, including fanout branches."""
import argparse
from collections import Counter
import csv
from decimal import Decimal
from itertools import product
from pathlib import Path
import re
import subprocess
import tempfile

from cube_output_check import read_cubes, cube_texts

INPUTS = ('a', 'b')
# Three independently implemented copies of a XOR b (one inverted). Their faults
# have equivalences that gate-local structural collapsing cannot discover.
GATES = [('p', 'EXOR', ('a', 'b')), ('q', 'EXNOR', ('a', 'b')),
         ('t', 'EXOR', ('a', 'b')), ('buffered', 'BUF', ('p',)),
         ('u', 'EXOR', ('buffered', 'q')), ('y', 'EXOR', ('u', 't'))]
COLUMNS = ['net_name', 'f_type', 'cube_cnt', 'complete', 'fdp', 'rep_net_name', 'rep_f_type']
PAIRS = [('p/0', 'q/1'), ('q/1', 't/0'), ('p/1', 'q/0'), ('q/0', 't/1')]


def write_circuit(path):
    lines = ['module fixture(a,b,y);', 'input a,b;', 'output y;',
             'wire a,b,p,q,t,buffered,u,y;']
    for index, (name, kind, sources) in enumerate(GATES):
        pins = ','.join(f'.{pin}({source})' for pin, source in zip('AB', sources))
        cell = kind if kind == 'BUF' else kind + '2'
        lines.append(f'{cell} G{index}({pins},.Z({name}));')
    path.write_text('\n'.join([*lines, 'endmodule', '']))


def exhaustive_events():
    uses = Counter(source for _, _, sources in GATES for source in sources)
    branch_sites = {f'{source}_{output}_{pin}': (output, index)
                    for output, _, sources in GATES
                    for index, (pin, source) in enumerate(zip('AB', sources)) if uses[source] > 1}
    sites = [*INPUTS, *(output for output, _, _ in GATES), *branch_sites]

    def evaluate(vector, fault=None):
        site, stuck = fault if fault else (None, None)
        values = dict(zip(INPUTS, vector))
        if site in values:
            values[site] = stuck
        for output, kind, sources in GATES:
            bits = [stuck if branch_sites.get(site) == (output, index) else values[source]
                    for index, source in enumerate(sources)]
            value = bits[0] if kind == 'BUF' else bits[0] ^ bits[1]
            if kind == 'EXNOR':
                value ^= 1
            values[output] = stuck if site == output else value
        return values['y']

    vectors = list(product((0, 1), repeat=len(INPUTS)))
    return {(site, f'sa{stuck}'): {vector for vector in vectors
                                  if evaluate(vector) != evaluate(vector, (site, stuck))}
            for site in sites for stuck in (0, 1)}


def run(binary, root, label, circuit, settings='', cli=(), success=True):
    directory = root / label
    directory.mkdir()
    config = directory / 'run.set'
    config.write_text(f'-saf\n-net {circuit}\n-fdp {directory}/fdp.csv\n'
                      f'-log {directory}/time.log\n-cube {directory}/cubes\n' + settings)
    result = subprocess.run([str(binary), '-set', str(config), *map(str, cli)],
                            capture_output=True, text=True, timeout=20)
    assert (result.returncode == 0) == success, (label, result.stdout, result.stderr)
    return directory, result


def verify(directory, events, requested=None, limited=False):
    with (directory / 'fdp.csv').open() as stream:
        reader = csv.DictReader(stream)
        assert reader.fieldnames == COLUMNS, directory
        rows = list(reader)
    requested = set(events) if requested is None else set(requested)
    by_fault = {(row['net_name'], row['f_type']): row for row in rows}
    assert set(by_fault) == requested and len(rows) == len(requested), directory
    representatives = {key: row for key, row in by_fault.items() if row['complete']}
    for key, row in by_fault.items():
        assert None not in row and None not in row.values(), (directory, row)
        representative = row['rep_net_name'], row['rep_f_type']
        assert representative in representatives, (directory, row)
        computed = representatives[representative]
        assert (computed['rep_net_name'], computed['rep_f_type']) == representative
        assert row['fdp'] == computed['fdp'] and events[key] == events[representative]
        exact = Decimal(len(events[key])) / 4
        if computed['complete'] == '1':
            assert abs(Decimal(row['fdp']) - exact) < Decimal('5e-11'), (directory, key, row)
        else:
            assert limited and computed['complete'] == '0'
            assert Decimal(0) <= Decimal(row['fdp']) <= exact, (directory, row)
        if key != representative:
            assert row['cube_cnt'] == row['complete'] == '', (directory, row)
    order, blocks = read_cubes(directory / 'cubes')
    assert set(order) == set(INPUTS)
    assert set(blocks) == {key for key, row in representatives.items() if Decimal(row['fdp']) > 0}
    for key, cubes in blocks.items():
        covered = {vector for vector in product((0, 1), repeat=len(INPUTS))
                   if any(all(bit == 'X' or bit == str(dict(zip(INPUTS, vector))[name])
                              for name, bit in zip(order, cube)) for cube in cubes)}
        assert covered <= events[key], (directory, key)
        if representatives[key]['complete'] == '1':
            assert covered == events[key], (directory, key)
        assert Decimal(len(covered)) / 4 == Decimal(representatives[key]['fdp'])
    return by_fault, representatives


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--debug', type=Path, required=True)
    args = parser.parse_args()
    events = exhaustive_events()
    for left, right in PAIRS:
        def key(reference):
            name, stuck = reference.rsplit('/', 1)
            return name, 'sa' + stuck
        assert events[key(left)] == events[key(right)] and events[key(left)]
    with tempfile.TemporaryDirectory(prefix='fdp-external-equivalence-') as temporary:
        root = Path(temporary)
        circuit = root / 'fixture.v'
        write_circuit(circuit)
        pairs = root / 'pairs.csv'
        pairs.write_bytes(('# additional equivalences\r\n\r\n' +
                           ''.join(f' {left} , {right} \r\n' for left, right in PAIRS) +
                           't/0,p/0\r\np/0,q/1\r\np/0,p/0').encode())
        reversed_pairs = root / 'reversed.csv'
        reversed_pairs.write_text(''.join(f'{right},{left}\n' for left, right in reversed(PAIRS)))
        empty = root / 'empty.csv'
        empty.write_text('# no additional relations\n')
        for binary in dict.fromkeys((args.binary.resolve(), args.debug.resolve())):
            prefix = binary.name
            baseline, _ = run(binary, root, prefix + '_baseline', circuit)
            _, base_reps = verify(baseline, events)
            unchanged, _ = run(binary, root, prefix + '_empty', circuit, f'-equiv {empty}\n')
            assert (unchanged / 'fdp.csv').read_bytes() == (baseline / 'fdp.csv').read_bytes()
            grouped = None
            for jobs in (1, 4):
                directory, result = run(binary, root, prefix + f'_grouped_{jobs}', circuit,
                                        f'-equiv {pairs}\n-jobs {jobs}\n')
                rows, reps = verify(directory, events)
                assert len(reps) == len(base_reps) - 4
                for kind in ('sa0', 'sa1'):
                    assert rows['p', kind]['complete'] == ''  # excluded by BUF structure
                    rep = rows['p', kind]['rep_net_name'], rows['p', kind]['rep_f_type']
                    assert rep == (rows['t', kind]['rep_net_name'], rows['t', kind]['rep_f_type'])
                log = (directory / 'time.log').read_text()
                assert re.search(r'External Equivalence Pairs\s*:\s*7\b', log)
                assert re.search(r'Targets Before External Equivalence\s*:\s*' + str(len(base_reps)), log)
                assert re.search(r'Targets After External Equivalence\s*:\s*' + str(len(reps)), log)
                assert f'Progress >> {len(reps)}/{len(reps)}' in result.stdout
                if grouped:
                    assert (directory / 'fdp.csv').read_bytes() == (grouped / 'fdp.csv').read_bytes()
                    assert cube_texts(directory / 'cubes') == cube_texts(grouped / 'cubes')
                grouped = directory
            reordered, _ = run(binary, root, prefix + '_reordered', circuit,
                               f'-equiv {empty}\n', cli=('-equiv', reversed_pairs))
            assert (reordered / 'fdp.csv').read_bytes() == (grouped / 'fdp.csv').read_bytes()
            for jobs in (1, 4):
                directory, _ = run(binary, root, prefix + f'_limited_{jobs}', circuit,
                                   f'-equiv {pairs}\n-limit 1\n-jobs {jobs}\n-cube_include_incomplete on\n')
                rows, reps = verify(directory, events, limited=True)
                representative = rows['p', 'sa0']['rep_net_name'], rows['p', 'sa0']['rep_f_type']
                assert reps[representative]['complete'] == '0'
                if jobs == 1:
                    limited = directory
                else:
                    assert (directory / 'fdp.csv').read_bytes() == (limited / 'fdp.csv').read_bytes()
                    assert cube_texts(directory / 'cubes') == cube_texts(limited / 'cubes')
            requested = [('p', 'sa0'), ('t', 'sa0'), ('q', 'sa0')]
            faults = root / 'selected.fault'
            faults.write_text(''.join(f'{name} {kind}\n' for name, kind in requested))
            for jobs in (1, 4):
                directory, _ = run(binary, root, prefix + f'_selected_{jobs}', circuit,
                                   f'-equiv {pairs}\n-fault {faults}\n-jobs {jobs}\n')
                _, reps = verify(directory, events, requested)
                assert len(reps) == 2
            faults.write_text(''.join(f'{name} {kind}\n' for name, kind in events))
            directory, _ = run(binary, root, prefix + '_explicit_all', circuit,
                               f'-equiv {pairs}\n-fault {faults}\n')
            _, reps = verify(directory, events)
            assert len(reps) == len(base_reps) - 4
            faults.write_text('')
            directory, _ = run(binary, root, prefix + '_no_targets', circuit,
                               f'-equiv {pairs}\n-fault {faults}\n-jobs 4\n')
            verify(directory, events, requested=[])

            invalid = [b'p/2,q/1', b'p/0,missing/1', b'p/0', b'p/0,q/1,t/0',
                       b',q/1', b'p/0,', b'p/STR,q/1', b'p/0,q/1\0,trailing',
                       b'p/0,q/1\nq/0,t/3', b'"p/0",q/1', b'/0,q/1']
            for index, contents in enumerate(invalid):
                bad = root / f'invalid_{index}.csv'
                bad.write_bytes(contents)
                _, result = run(binary, root, prefix + f'_invalid_{index}', circuit,
                                f'-equiv {bad}\n', success=False)
                line = 2 if index == 8 else 1
                assert f'{bad}:{line}:' in result.stderr, result.stderr
            _, result = run(binary, root, prefix + '_missing_file', circuit,
                            f'-equiv {root / "missing.csv"}\n', success=False)
            assert 'cannot open equivalence file' in result.stderr
            _, result = run(binary, root, prefix + '_unreadable_input', circuit,
                            f'-equiv {root}\n', success=False)
            assert 'failed to read equivalence file' in result.stderr
            _, result = run(binary, root, prefix + '_reject_tdf', circuit,
                            f'-equiv {pairs}\n-tdf\n', success=False)
            assert '-equiv supports SAF only' in result.stderr
    print('External SAF equivalence: exhaustive events/cubes, transitive and opposite-polarity pairs, '
          'structurally excluded sites, representative metadata, lower bounds, explicit scope, '
          'empty/malformed input, serial/parallel and Debug/Release: PASS')


if __name__ == '__main__':
    main()
