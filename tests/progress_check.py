#!/usr/bin/env python3
"""Check completion-only progress, CLI/.set controls, and unchanged result files."""
import argparse
import csv
from pathlib import Path
import re

from tdf_check import ROOT, run, equal


def check_counts(directory, visible=True):
    text = (directory / 'stdout.txt').read_text()
    assert '\x1b' not in text and 'loops=' not in text and 'Cube progress >>' not in text, directory
    with (directory / 'fdp.csv').open() as stream:
        rows = [r for r in csv.DictReader(stream) if r['complete'] in ('0', '1')]
    updates = [(int(a), int(b)) for a, b in re.findall(r'Progress >> (\d+)/(\d+)', text)]
    expected = [(n, len(rows)) for n in range(1, len(rows) + 1)] if visible else []
    assert updates == expected, (directory, updates, expected)
    return rows


def check_controls(binary, output):
    source = ROOT / 'input/circuit/saf/c17a.v'
    cases = (
        ('default', '', (), True),
        ('set_off', '-progress off\n', (), False),
        ('cli_off', '', ('-progress', 'off'), False),
        ('cli_on_override', '-progress off\n', ('-progress', 'on'), True),
        ('cli_off_override', '-progress on\n', ('-progress', 'off'), False),
        ('set_last', '-progress off\n-progress on\n', (), True),
        ('cli_last', '', ('-progress', 'on', '-progress', 'off'), False),
        ('numeric_on', '-progress\t1\r\n', (), True),
        ('numeric_off', '', ('-progress', '0'), False),
    )
    for jobs in (1, 15):
        baseline = None
        for label, extra, cli, visible in cases:
            directory = run(binary, output / f'{binary.name}_simple_controls_{jobs}_{label}', source,
                            f'-jobs {jobs}\n-dom_reuse off\n' + extra, cli=cli, model='-saf\n')
            check_counts(directory, visible)
            if baseline is None:
                baseline = directory
            else:
                equal(baseline, directory)
    for mode in ('cli', 'set'):
        for index, value in enumerate((None, 'invalid', '2', 'ON')):
            tokens = ('-progress',) if value is None else ('-progress', value)
            directory = run(binary, output / f'{binary.name}_simple_invalid_{mode}_{index}', source,
                            ' '.join(tokens) + '\n' if mode == 'set' else '',
                            cli=tokens if mode == 'cli' else (), success=False, model='-saf\n')
            assert '-progress requires on|off' in (directory / 'stderr.txt').read_text()
        for option, value in (('-fault_progress', 'on'), ('-cube_progress', 'on'),
                              ('-progress_interval', '0.1')):
            tokens = (option, value)
            directory = run(binary, output / f'{binary.name}_removed_{mode}_{option[1:]}', source,
                            ' '.join(tokens) + '\n' if mode == 'set' else '',
                            cli=tokens if mode == 'cli' else (), success=False, model='-saf\n')
            assert option in (directory / 'stdout.txt').read_text()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--debug', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    for binary in (args.binary.resolve(), args.debug.resolve()):
        check_controls(binary, output)
        for model, circuit in (('saf', 'c17a'), ('tdf', 's27')):
            modes = [(1, 'off'), (4, 'off'), (15, 'off')]
            if model == 'saf':
                modes.append((1, 'on'))
            for jobs, reuse in modes:
                for limit in (0, 2, 100000):
                    label = f'{binary.name}_{model}_{jobs}_{reuse}_{limit}'
                    extra = f'-jobs {jobs}\n-dom_reuse {reuse}\n-limit {limit}\n'
                    source = ROOT / f'input/circuit/{model}/{circuit}.v'
                    on_dir = output / (label + '_on')
                    off_dir = output / (label + '_off')
                    cube_options = lambda directory: (
                        '-cube', str(directory / 'cubes'), '-cube_include_incomplete', 'on'
                    ) if limit == 2 else ()
                    on = run(binary, on_dir, source, extra, cli=cube_options(on_dir), model=f'-{model}\n')
                    off = run(binary, off_dir, source, extra + '-progress off\n',
                              cli=cube_options(off_dir), model=f'-{model}\n')
                    equal(on, off)
                    check_counts(on)
                    check_counts(off, visible=False)
                    if limit == 2:
                        saved = lambda directory: {
                            p.name: p.read_bytes() for p in (directory / 'cubes').glob('*_cube.txt')
                        }
                        assert saved(on) == saved(off), (on, off)
                    if model == 'saf' and limit == 0:
                        projection = lambda p: sorted((r['net_name'], r['f_type'], r['fdp'])
                                                       for r in csv.DictReader(p.open()))
                        assert projection(on / 'fdp.csv') == projection(ROOT / 'tests/expected/c17a_result.csv')
    print('Completion-only progress, on/off, CLI/.set validation, removed options, '
          'serial/4/15 workers, SAF/TDF, Debug/Release, CSV/cube/golden equality: PASS')


if __name__ == '__main__':
    main()
