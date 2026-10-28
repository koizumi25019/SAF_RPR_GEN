#!/usr/bin/env python3
"""Exercise selected cube output and independently expand the saved covers."""
import argparse
import csv
from decimal import Decimal
import os
from pathlib import Path
from urllib.parse import quote
import subprocess
import tempfile

from tdf_check import reference_events, truth_vectors

ROOT = Path(__file__).resolve().parents[1]


def cube_files(directory):
    return sorted(path for path in directory.glob('*.txt')
                  if path.name.endswith(('_sa0_cube.txt', '_sa1_cube.txt', '_STR_cube.txt', '_STF_cube.txt')))


def cube_texts(directory):
    return {path.name: path.read_bytes() for path in cube_files(directory)}


def cube_metadata(directory):
    lines = (directory / '.fdp-cube-output').read_text().split('\n', 1)
    assert lines[0] == 'FDP per-fault cube output v2'
    return lines[1]


def read_cubes(directory, completion=None):
    header = cube_metadata(directory)
    assert header.startswith('# FDP test cube metadata v1\n')
    order = next(line.split()[2:] for line in header.splitlines() if line.startswith('# pi '))
    assert len(order) == int(next(line.split()[2] for line in header.splitlines() if line.startswith('# inputs ')))
    blocks = {}
    for file in cube_files(directory):
        lines = file.read_text().splitlines()
        assert len(lines) >= 3
        name, kind = lines[0].split('\t')
        assert kind in ('sa0', 'sa1', 'STR', 'STF')
        assert lines[1] in ('complete', 'incomplete')
        key = name, kind
        assert key not in blocks
        assert file.name == quote(name, safe='._-') + '_' + kind + '_cube.txt'
        if completion is not None:
            completion[key] = '1' if lines[1] == 'complete' else '0'
        cubes = lines[2:]
        assert all(len(cube) == len(order) and set(cube) <= set('01X') for cube in cubes)
        assert file.read_bytes().endswith(b'\n')
        blocks[key] = cubes
    return order, blocks


def verify(directory, circuit, tdf=False):
    rows = list(csv.DictReader((directory / 'fdp.csv').open()))
    limit = int(next(line.split()[2] for line in cube_metadata(directory / 'cubes').splitlines()
                     if line.startswith('# limit ')))
    maximum = next((Decimal(line.split()[2]) for line in cube_metadata(directory / 'cubes').splitlines()
                    if line.startswith('# cube_fdp_max ')), Decimal(1))
    include_incomplete = '# cube_include_incomplete on' in cube_metadata(directory / 'cubes').splitlines()
    expected = {(r['net_name'], r['f_type']): r for r in rows
                if (r['complete'] == '1' or (include_incomplete and r['complete'] == '0'))
                and Decimal(r['fdp']) > 0
                and (limit <= 0 or int(r['cube_cnt']) <= limit) and Decimal(r['fdp']) <= maximum}
    completion = {}
    order, blocks = read_cubes(directory / 'cubes', completion)
    assert set(blocks) == set(expected), (directory, set(blocks) ^ set(expected))
    mask, vectors = truth_vectors(order)
    if tdf:
        net, event_mask, _, event_vectors, events = reference_events(circuit)
        inverse = {net.alias[p]: p for p in net.pi}
        inverse.update({net.alias[q] + '_1t': q + '_1t' for q, _ in net.flops})
        assert set(order) == set(inverse)
        mask = event_mask
        vectors = {p: event_vectors[inverse[p]] for p in order}
    for key, cubes in blocks.items():
        assert len(cubes) == int(expected[key]['cube_cnt'])
        assert completion[key] == expected[key]['complete']
        union = 0
        for cube in cubes:
            expanded = mask
            for name, bit in zip(order, cube):
                if bit != 'X':
                    expanded &= vectors[name] if bit == '1' else mask ^ vectors[name]
            union |= expanded
        density = Decimal(union.bit_count()) / Decimal(mask.bit_count())
        assert abs(density - Decimal(expected[key]['fdp'])) <= Decimal('5e-11'), (directory, key)
        if tdf:
            assert not (union & (mask ^ events[key])), (directory, key, 'unsound detection cover')
            if completion[key] == '1':
                assert union == events[key], (directory, key, 'wrong detection cover')
    return rows, blocks


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--debug', type=Path, required=True)
    args = parser.parse_args()
    env = dict(os.environ)
    with tempfile.TemporaryDirectory(prefix='fdp-cube-') as temporary:
        root = Path(temporary)

        def run(label, circuit, extra='', cli=(), binary=args.binary, save=True, success=True):
            directory = root / label
            directory.mkdir(exist_ok=True)
            config = directory / 'run.set'
            config.write_text(f'-net {circuit}\n-fdp {directory}/fdp.csv\n-log {directory}/time.log\n'
                              + (f'-cube {directory}/cubes\n' if save else '') + extra)
            result = subprocess.run([str(binary.resolve()), '-set', str(config), *cli],
                                    cwd=ROOT / 'build', env=env, capture_output=True, text=True, timeout=60)
            assert (result.returncode == 0) == success, (label, result.stderr, result.stdout[-1000:])
            return directory

        for model, circuit in (('saf', ROOT / 'input/circuit/saf/c17a.v'),
                               ('tdf', ROOT / 'input/circuit/tdf/s27.v')):
            for limit in (0, 1, 30):
                extra = f'-{model}\n-limit {limit}\n'
                serial = run(f'{model}_{limit}_serial', circuit, extra)
                rows, blocks = verify(serial, circuit, model == 'tdf')
                parallel = run(f'{model}_{limit}_parallel', circuit, extra + '-jobs 4\n')
                debug = run(f'{model}_{limit}_debug', circuit, extra, binary=args.debug)
                disabled = run(f'{model}_{limit}_disabled', circuit, extra, save=False)
                for other in (parallel, debug, disabled):
                    assert (serial / 'fdp.csv').read_bytes() == (other / 'fdp.csv').read_bytes()
                for other in (parallel, debug):
                    assert cube_texts(serial / 'cubes') == cube_texts(other / 'cubes')
                assert not (disabled / 'cubes').exists()
                assert blocks
                if limit == 1:
                    assert any(r['complete'] == '0' for r in rows)
                    assert any(r['complete'] == '1' and r['cube_cnt'] == '1' for r in rows)
                if model == 'tdf':
                    assert any(r['complete'] == '1' and Decimal(r['fdp']) == 0 for r in rows)
            for maximum in ('1', '0.125'):
                extra = (f'-{model}\n-limit 1\n'
                         f'-cube_fdp_max {maximum}\n-cube_include_incomplete on\n')
                serial = run(f'{model}_incomplete_{maximum}', circuit, extra)
                rows, blocks = verify(serial, circuit, model == 'tdf')
                assert any(r['complete'] == '0' and (r['net_name'], r['f_type']) in blocks for r in rows)
                parallel = run(f'{model}_incomplete_{maximum}_parallel', circuit, extra + '-jobs 4\n')
                debug = run(f'{model}_incomplete_{maximum}_debug', circuit, extra, binary=args.debug)
                for other in (serial, parallel, debug):
                    assert (other / 'fdp.csv').read_bytes() == (root / f'{model}_1_serial/fdp.csv').read_bytes()
                    assert cube_texts(other / 'cubes') == cube_texts(serial / 'cubes')
            # An explicit off and a later CLI override both keep the original selection.
            off = run(model + '_incomplete_off', circuit,
                      f'-{model}\n-limit 1\n-cube_include_incomplete off\n')
            assert cube_texts(off / 'cubes') == cube_texts(root / f'{model}_1_serial/cubes')
            override = run(model + '_incomplete_override', circuit,
                           f'-{model}\n-limit 1\n-cube_include_incomplete on\n',
                           cli=('-cube_include_incomplete', 'off'))
            assert cube_texts(override / 'cubes') == cube_texts(off / 'cubes')
            cli_on = run(model + '_incomplete_cli_on', circuit,
                         f'-{model}\n-limit 1\n-cube_fdp_max 1\n'
                         '-cube_include_incomplete off\n', cli=('-cube_include_incomplete', 'on'))
            assert cube_texts(cli_on / 'cubes') == cube_texts(root / f'{model}_incomplete_1/cubes')
            # Inclusive thresholds, empty selection, scientific notation, and very small bounds.
            for maximum in ('0', '0.25', '1.25e-1', '1', '1e-1000'):
                extra = f'-{model}\n-limit 30\n-cube_fdp_max {maximum}\n'
                serial = run(f'{model}_max_{maximum}', circuit, extra)
                rows, blocks = verify(serial, circuit, model == 'tdf')
                parallel = run(f'{model}_max_{maximum}_parallel', circuit, extra + '-jobs 4\n')
                debug = run(f'{model}_max_{maximum}_debug', circuit, extra, binary=args.debug)
                for other in (serial, parallel, debug):
                    assert (other / 'fdp.csv').read_bytes() == (root / f'{model}_30_serial/fdp.csv').read_bytes()
                    assert cube_texts(other / 'cubes') == cube_texts(serial / 'cubes')
                if maximum == '1.25e-1' and model == 'saf':
                    assert any(Decimal(r['fdp']) == Decimal(maximum) and
                               (r['net_name'], r['f_type']) in blocks for r in rows)
                    assert len(blocks) < len(read_cubes(root / f'{model}_30_serial/cubes')[1])
                if maximum in ('0', '1e-1000'):
                    assert not blocks

        circuit = ROOT / 'input/circuit/saf/c17a.v'
        expected = list(csv.DictReader((ROOT / 'tests/expected/saf/c17a_result.csv').open()))
        assert sorted((r['net_name'], r['f_type'], r['fdp']) for r in expected) == sorted(
            (r['net_name'], r['f_type'], r['fdp'])
            for r in csv.DictReader((root / 'saf_0_serial/fdp.csv').open()))
        # CLI support and an empty fault set (header only).
        directory = root / 'cli'
        run('cli', circuit, save=False, cli=('-cube', str(directory / 'cubes')))
        verify(directory, circuit)
        overridden = run('max_cli_override', circuit, '-cube_fdp_max 0\n',
                         cli=('-cube_fdp_max', '0.25'))
        verify(overridden, circuit)
        assert read_cubes(overridden / 'cubes')[1]
        set_override = run('max_set_override', circuit, '-cube_fdp_max 1\n-cube_fdp_max 0\n')
        assert read_cubes(set_override / 'cubes')[1] == {}
        for index, value in enumerate(('-0.1', '1.01', '1.00000000000000000001', 'nan', 'inf', 'junk', '0.2junk')):
            run(f'max_invalid_set_{index}', circuit, f'-cube_fdp_max {value}\n', success=False)
            run(f'max_invalid_cli_{index}', circuit, cli=('-cube_fdp_max', value), success=False)
        run('max_missing_set', circuit, '-cube_fdp_max\n', success=False)
        run('max_missing_cli', circuit, cli=('-cube_fdp_max',), success=False)
        for index, value in enumerate(('yes', '2', '')):
            run(f'incomplete_invalid_set_{index}', circuit,
                f'-cube_include_incomplete {value}\n', success=False)
            run(f'incomplete_invalid_cli_{index}', circuit,
                cli=('-cube_include_incomplete', value), success=False)
        run('incomplete_missing_cli', circuit, cli=('-cube_include_incomplete',), success=False)
        empty = root / 'empty.faults'; empty.write_text('')
        directory = run('empty', circuit, f'-fault {empty}\n-jobs 4\n')
        assert read_cubes(directory / 'cubes')[1] == {}
        assert list((directory / 'cubes').glob('*.txt')) == []
        # Reusing a managed directory replaces the selection, including an empty result.
        rerun = run('rerun', circuit)
        assert cube_texts(rerun / 'cubes')
        note = rerun / 'cubes/notes.md'; note.write_text('keep this file')
        run('rerun', circuit, '-cube_fdp_max 0\n-jobs 4\n')
        assert cube_texts(rerun / 'cubes') == {} and note.read_text() == 'keep this file'
        run('rerun', circuit, '-jobs 4\n')
        verify(rerun, circuit)
        # Reusing the previous directory format removes the old fault_ filenames too.
        for version, prefix in ((1, 'fault_'), (2, '')):
            label = f'legacy_directory_{version}'
            legacy = run(label, circuit)
            for file in cube_files(legacy / 'cubes'):
                lines = file.read_text().splitlines()
                name, kind = lines[0].split('\t')
                old = legacy / 'cubes' / (prefix + quote(name, safe='._-') + '__' + kind + '.txt')
                old.write_text('# FDP test cubes v2\n'
                               f'fault {name} {kind} {len(lines)-2} complete=1\n'
                               + '\n'.join(lines[2:]) + '\nend\n\n')
                file.unlink()
            legacy_marker = legacy / 'cubes/.fdp-cube-output'
            legacy_marker.write_text(f'FDP per-fault cube output v{version}\n# FDP test cubes v2\n')
            run(label, circuit, '-cube_fdp_max 0\n-jobs 4\n')
            assert list((legacy / 'cubes').glob('*.txt')) == []
            assert cube_metadata(legacy / 'cubes').startswith('# FDP test cube metadata v1\n')
        # Existing unrelated data and the old single-file output are never overwritten.
        foreign = root / 'foreign'; foreign.mkdir()
        keep = foreign / 'fault_keep__sa0.txt'; keep.write_text('unrelated content')
        run('foreign_directory', circuit, cli=('-cube', str(foreign)), success=False)
        assert keep.read_text() == 'unrelated content'
        old_file = root / 'old_cubes.txt'; old_file.write_text('old aggregate output')
        run('legacy_file', circuit, cli=('-cube', str(old_file)), success=False)
        assert old_file.read_text() == 'old aggregate output'
        marker = rerun / 'cubes/.fdp-cube-output'
        marker.write_text('invalid marker')
        before = cube_texts(rerun / 'cubes')
        run('rerun', circuit, success=False)
        assert cube_texts(rerun / 'cubes') == before
        for index, (extra, cli) in enumerate((('-cube\n', ()), ('', ('-cube',)),
                                              ('', ('-cube', '-jobs', '4')))):
            run(f'invalid_{index}', circuit, extra, cli, success=False)
        for jobs in (1, 4):
            run(f'io_failure_{jobs}', circuit, f'-jobs {jobs}\n-cube /dev/full\n', success=False)
        print('Selected cube output, exact limit, redundant/equivalent exclusion, covers, '
              'serial/parallel/debug, FDP regression, settings and I/O failures: PASS')


if __name__ == '__main__':
    main()
