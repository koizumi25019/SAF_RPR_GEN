#!/usr/bin/env python3
"""Check fault/loop counts, live worker notifications, and progress-off behavior."""
import argparse
import csv
import os
from pathlib import Path
import re
import select
import subprocess
import time

from tdf_check import ROOT, run, equal

FAULT_PROGRESS = re.compile(
    r'(?:fault|worker) \d+: (\S+) (sa[01]|STR|STF) loops=(\d+) seeded=(\d+) \[([^]]+)\]')


def check_counts(directory):
    text = (directory / 'stdout.txt').read_text()
    with (directory / 'fdp.csv').open() as stream:
        rows = [r for r in csv.DictReader(stream) if r['complete'] in ('0', '1')]
    expected = {(r['net_name'], r['f_type']):
                (int(r['cube_cnt']) - int(r['seeded_cnt']), int(r['seeded_cnt'])) for r in rows}
    observed = {}
    for name, kind, loops, seeds, phase in FAULT_PROGRESS.findall(text):
        if phase in ('BDD', 'done'):
            observed[name, kind] = (int(loops), int(seeds))
    assert observed == expected, (directory, observed, expected)
    completed = [(int(a), int(b)) for a, b in re.findall(r'Progress >> (\d+)/(\d+)', text)]
    assert completed[0] == (0, len(rows)), (directory, completed)
    assert completed[-1] == (len(rows), len(rows)), (directory, completed)
    assert all(0 <= n <= total == len(rows) for n, total in completed), directory
    assert [n for n, _ in completed] == sorted(n for n, _ in completed), directory
    return rows


def check_live(binary, output, jobs):
    """Parity needs exponentially many cubes; observe it before terminating the run."""
    directory = output / f'live_{jobs}'
    directory.mkdir(parents=True, exist_ok=True)
    circuit = directory / 'parity.v'
    inputs = [f'a{i}' for i in range(24)]
    gates = []
    previous = inputs[0]
    for index, signal in enumerate(inputs[1:], 1):
        target = 'y' if index == len(inputs) - 1 else f'p{index}'
        gates.append(f'EXOR2 g{index}(.A({previous}),.B({signal}),.Z({target}));')
        previous = target
    circuit.write_text('module parity(' + ','.join(inputs + ['b', 'y', 'z']) + ');\ninput ' +
                       ','.join(inputs + ['b']) + ';\noutput y,z;\n' + '\n'.join(gates) +
                       '\nBUF fast(.A(b),.Z(z));\nendmodule\n')
    faults = directory / 'faults.txt'
    faults.write_text('z sa0\ny sa0\ny sa1\n')
    config = directory / 'run.set'
    config.write_text(f'-net {circuit}\n-fault {faults}\n-jobs {jobs}\n-dom_reuse off\n'
                      f'-limit 0\n-progress on\n-fdp {directory}/fdp.csv\n-log {directory}/time.log\n')
    (directory / 'time.log').unlink(missing_ok=True)
    env = {k: v for k, v in os.environ.items() if k not in ('MDC_NODOM', 'MDC_NOPROP')}
    data = bytearray()
    children = set()
    found = False
    with (directory / 'stderr.txt').open('wb') as err:
        process = subprocess.Popen([str(binary), '-set', str(config)], stdout=subprocess.PIPE,
                                   stderr=err, env=env, cwd=ROOT / 'build')
        try:
            deadline = time.monotonic() + 15
            while time.monotonic() < deadline:
                children_file = Path(f'/proc/{process.pid}/task/{process.pid}/children')
                if children_file.exists():
                    children.update(int(pid) for pid in children_file.read_text().split())
                ready, _, _ = select.select([process.stdout], [], [], 0.2)
                if ready:
                    chunk = os.read(process.stdout.fileno(), 65536)
                    if not chunk:
                        break
                    data.extend(chunk)
                for snapshot in data.decode().split('\r'):
                    generating = {(name, kind) for name, kind, loops, seeds, phase
                                  in FAULT_PROGRESS.findall(snapshot)
                                  if name == 'y' and int(loops) > 0 and phase == 'SAT/XID'}
                    if 'Progress >> 1/3' in snapshot and len(generating) >= min(jobs, 2):
                        found = True
                        break
                if found:
                    assert process.poll() is None, 'progress only arrived after exit'
                    break
            assert found, (jobs, data.decode()[-4000:])
        finally:
            if process.poll() is None:
                process.terminate()
            remaining, _ = process.communicate(timeout=10)
            data.extend(remaining)
            (directory / 'stdout.txt').write_bytes(data)
    assert process.returncode != 0
    assert all(not Path(f'/proc/{pid}').exists() for pid in children), children
    assert not (directory / 'time.log').exists(), 'cancelled run recorded as successful'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--debug', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    for binary in (args.binary.resolve(), args.debug.resolve()):
        for model, circuit in (('saf', 'c17a'), ('tdf', 's27')):
            modes = [(1, 'off'), (4, 'off')]
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
                    if limit == 2:
                        saved_cubes = lambda directory: {
                            path.name: path.read_bytes() for path in (directory / 'cubes').glob('*_cube.txt')
                        }
                        assert saved_cubes(on) == saved_cubes(off), (on, off)
                    rows = check_counts(on)
                    assert 'Progress >>' not in (off / 'stdout.txt').read_text()
                    if model == 'saf' and limit == 0:
                        with (ROOT / 'tests/expected/c17a_result.csv').open() as stream:
                            golden = list(csv.DictReader(stream))
                        with (on / 'fdp.csv').open() as stream:
                            actual = list(csv.DictReader(stream))
                        projection = lambda rs: sorted((r['net_name'], r['f_type'], r['fdp']) for r in rs)
                        assert projection(actual) == projection(golden)
                        if reuse == 'on':
                            assert any(int(r['seeded_cnt']) > 0 for r in rows)
    for jobs in (1, 2):
        check_live(args.binary.resolve(), output, jobs)
    print('Fault completion/loop/seed counts, SAF/TDF, limits, Debug/Release, progress off, '
          'CSV/golden equality, live serial/worker updates and cancellation: PASS')


if __name__ == '__main__':
    main()
