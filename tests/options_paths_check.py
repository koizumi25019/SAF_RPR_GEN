#!/usr/bin/env python3
"""Check option values, required settings and circuit names through the executable."""
import argparse
import csv
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CIRCUIT = ROOT / 'input/circuit/saf/c17a.v'
EXPECTED = ROOT / 'tests/expected/saf/c17a_result.csv'


def projection(path):
    with path.open() as file:
        return sorted((row['net_name'], row['f_type'], row['fdp'])
                      for row in csv.DictReader(file))


def check_binary(binary, output):
    output.mkdir(parents=True)
    env = dict(os.environ)
    env.pop('MDC_NOPROP', None)

    def run(label, extra=(), missing=None, error=None, settings=None,
            netlist=CIRCUIT, cwd=ROOT, circuit_name=None, complete=False):
        directory = output / label
        directory.mkdir()
        arguments = []
        for option, value in (('-net', netlist), ('-fdp', directory / 'fdp.csv'),
                              ('-log', directory / 'time.log')):
            if option != missing:
                arguments.extend((option, str(value)))
        if settings is not None:
            config = directory / 'run.set'
            config.write_bytes(settings.encode())
            arguments.extend(('-set', str(config)))
        result = subprocess.run([str(binary), *arguments, *extra], cwd=cwd, env=env,
                                capture_output=True, text=True, timeout=20)
        diagnostics = result.stdout + result.stderr
        assert 'AddressSanitizer' not in diagnostics and 'runtime error:' not in diagnostics, diagnostics
        if error:
            assert result.returncode == 1, (label, result.returncode, diagnostics)
            assert 'COMMAND ERROR' in diagnostics and error in diagnostics, (label, diagnostics)
            assert not (directory / 'fdp.csv').exists(), label
            return
        assert result.returncode == 0, (label, result.returncode, diagnostics)
        if circuit_name:
            report = (directory / 'time.log').read_text()
            actual = re.search(r'Target Circuit\s*:\s*([^\n]+)', report).group(1)
            assert actual == circuit_name, (label, actual, circuit_name)
        if complete:
            assert projection(directory / 'fdp.csv') == projection(EXPECTED), label

    result = subprocess.run([str(binary)], env=env, capture_output=True, text=True, timeout=5)
    assert result.returncode == 1 and 'no netlist file' in result.stdout + result.stderr

    for option in ('-net', '-fault', '-log', '-fdp', '-cube', '-set', '-limit'):
        name = option[1:]
        run(name + '_missing_cli', (option,), error=option)
        run(name + '_empty_cli', (option, ''), error=option)
        run(name + '_next_option_cli', (option, '-progress', 'off'), error=option)
        if option != '-set':
            run(name + '_missing_set', settings=option + '\n', error=option)
            run(name + '_blank_set', settings=option + ' \t\r\n', error=option)
            run(name + '_comment_set', settings=option + ' # no value\n', error=option)

    for option, message in (('-net', 'no netlist file'), ('-log', 'no log file'),
                             ('-fdp', 'no fdp result file')):
        run('required_' + option[1:], missing=option, error=message)

    for index, value in enumerate(('invalid', '1.5', '30junk', str(2**100), str(-(2**100)))):
        run(f'limit_invalid_cli_{index}', ('-limit', value), error='-limit')
        run(f'limit_invalid_set_{index}', settings=f'-limit {value}\n', error='-limit')
    for value in ('0', '-1', '-100'):
        run('unlimited_cli_' + value, ('-limit', value), complete=True)
        run('unlimited_set_' + value, settings=f'-limit\t{value}\r\n', complete=True)
    run('positive_limit', ('-limit', '1'))
    run('explicit_faults', ('-fault', str(ROOT / 'input/fault/saf/c17a_rep.txt')))

    fixture = output / 'fixtures'
    fixture.mkdir()
    paths = [('c17a.v', 'c17a'), ('c17a', 'c17a'), ('.c17a', '.c17a'),
             ('nested/deeper/c17a.v', 'c17a'),
             ('directory.with.dots/c17a.test.v', 'c17a.test')]
    for index, (relative, name) in enumerate(paths):
        path = fixture / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(CIRCUIT, path)
        run(f'path_relative_{index}', netlist=relative, cwd=fixture,
            circuit_name=name, complete=True)
        run(f'path_absolute_{index}', netlist=path, circuit_name=name, complete=True)
    run('path_dot_prefix', netlist='./c17a.v', cwd=fixture,
        circuit_name='c17a', complete=True)
    run('set_crlf_path', settings=f'-net\t{fixture / "c17a.v"}\r\n-limit\t0\r\n',
        circuit_name='c17a', complete=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--debug', type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='fdp_options_paths_') as temporary:
        output = Path(temporary)
        check_binary(args.binary.resolve(), output / 'release')
        if args.debug:
            check_binary(args.debug.resolve(), output / 'debug')
    print('Missing values, required settings, optional faults, limits and circuit names: PASS')


if __name__ == '__main__':
    main()
