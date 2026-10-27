#!/usr/bin/env python3
"""Check fault/loop counts, live worker notifications, and progress-off behavior."""
import argparse
import csv
import fcntl
import os
from pathlib import Path
import pty
import re
import select
import struct
import subprocess
import termios
import time

from tdf_check import ROOT, run, equal

FAULT_PROGRESS = re.compile(
    r'(?:fault|worker) \d+: (\S+) (sa[01]|STR|STF) loops=(\d+) seeded=(\d+) \[([^]]+)\]')


def maximum_snapshots(directory):
    elapsed = re.search(r'//\s+Time\s+:\s+([\d.]+) sec', (directory / 'time.log').read_text())
    assert elapsed, directory
    return int(float(elapsed[1])) + 3  # one per second, plus initial/final and rounding


def check_counts(directory):
    text = (directory / 'stdout.txt').read_text()
    assert '\x1b' not in text, 'redirected output must not contain terminal controls'
    assert len([line for line in text.splitlines() if line.startswith('Progress >>')]) <= maximum_snapshots(directory)
    with (directory / 'fdp.csv').open() as stream:
        rows = [r for r in csv.DictReader(stream) if r['complete'] in ('0', '1')]
    expected = {(r['net_name'], r['f_type']):
                (int(r['cube_cnt']) - int(r['seeded_cnt']), int(r['seeded_cnt'])) for r in rows}
    observed = {}
    for name, kind, loops, seeds, phase in FAULT_PROGRESS.findall(text):
        if phase in ('BDD', 'done'):
            observed[name, kind] = (int(loops), int(seeds))
    # Display snapshots are sampled globally; short faults need not appear individually.
    assert observed and all(expected[key] == value for key, value in observed.items()), (
        directory, observed, expected)
    completed = [(int(a), int(b)) for a, b in re.findall(r'Progress >> (\d+)/(\d+)', text)]
    assert completed[0] == (0, len(rows)), (directory, completed)
    assert completed[-1] == (len(rows), len(rows)), (directory, completed)
    assert all(0 <= n <= total == len(rows) for n, total in completed), directory
    assert [n for n, _ in completed] == sorted(n for n, _ in completed), directory
    return rows


def check_controls(binary, output):
    source = ROOT / 'input/circuit/saf/c17a.v'
    cases = (
        ('both', '', (), True, True),
        ('fault_only', '-fault_progress on\n-cube_progress off\n', (), True, False),
        ('cube_only', '-fault_progress off\n-cube_progress on\n', (), False, True),
        ('neither', '-fault_progress off\n-cube_progress off\n', (), False, False),
        ('legacy_off', '', ('-progress', 'off'), False, False),
        ('legacy_then_fault', '-progress off\n-fault_progress on\n', (), True, False),
        ('legacy_then_cube', '', ('-progress', 'off', '-cube_progress', 'on'), False, True),
        ('legacy_last_off', '-fault_progress on\n-cube_progress on\n-progress off\n', (), False, False),
        ('legacy_last_on', '', ('-fault_progress', 'off', '-cube_progress', 'off', '-progress', 'on'), True, True),
        ('cli_override', '-fault_progress off\n-cube_progress on\n', ('-fault_progress', 'on', '-cube_progress', 'off'), True, False),
        ('numeric_aliases', '-fault_progress\t0\r\n-cube_progress 1\n', (), False, True),
    )
    for jobs in (1, 4):
        baseline = None
        for label, extra, cli, fault_visible, cube_visible in cases:
            directory = run(binary, output / f'{binary.name}_controls_{jobs}_{label}', source,
                            f'-jobs {jobs}\n-dom_reuse off\n' + extra, cli=cli, model='-saf\n')
            if baseline is None:
                baseline = directory
            else:
                equal(baseline, directory)
            text = (directory / 'stdout.txt').read_text()
            updates = [(int(a), int(b)) for a, b in re.findall(r'Progress >> (\d+)/(\d+)', text)]
            assert bool(updates) == fault_visible, directory
            assert ('loops=' in text) == cube_visible, directory
            assert ('Cube progress >>' in text) == (cube_visible and not fault_visible), directory
            if fault_visible and not cube_visible:
                # Fast completions are coalesced, but the final total must be exact.
                total = updates[-1][1]
                assert updates[-1] == (22, 22), directory
                assert all(t == total for _, t in updates), directory
                assert len(updates) <= maximum_snapshots(directory), directory
            if not fault_visible and not cube_visible:
                assert 'progress >>' not in text.lower(), directory
    for option in ('-fault_progress', '-cube_progress'):
        for mode in ('cli', 'set'):
            for index, value in enumerate((None, 'invalid', '2', 'ON')):
                tokens = (option,) if value is None else (option, value)
                directory = run(binary, output / f'{binary.name}_{option[1:]}_{mode}_{index}', source,
                                ' '.join(tokens) + '\n' if mode == 'set' else '',
                                cli=tokens if mode == 'cli' else (), success=False, model='-saf\n')
                assert f'{option} requires on|off' in (directory / 'stderr.txt').read_text(), directory


def check_live(binary, output, jobs, fault_progress=True):
    """Parity needs exponentially many cubes; observe it before terminating the run."""
    directory = output / (f'live_{jobs}' + ('' if fault_progress else '_cube_only'))
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
                      f'-limit 0\n-progress on\n-fault_progress {"on" if fault_progress else "off"}\n'
                      f'-fdp {directory}/fdp.csv\n-log {directory}/time.log\n')
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
                for snapshot in data.decode().splitlines():
                    generating = {(name, kind) for name, kind, loops, seeds, phase
                                  in FAULT_PROGRESS.findall(snapshot)
                                  if name == 'y' and int(loops) > 0 and phase == 'SAT/XID'}
                    prefix = 'Progress >> 1/3' if fault_progress else 'Cube progress >>'
                    if prefix in snapshot and len(generating) >= min(jobs, 2):
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
    if not fault_progress:
        assert not re.search(rb'Progress >> \d+/\d+', data)


def check_terminal(binary, output):
    """Reproduce the reported 15-worker layout in a real PTY, including resize."""
    directory = output / 'terminal_15'
    directory.mkdir(parents=True, exist_ok=True)
    circuit = output / 'live_2/parity.v'
    faults = directory / 'faults.txt'
    faults.write_text(''.join(f'{name} sa{kind}\n'
                             for name in [f'p{i}' for i in range(16, 23)] + ['y']
                             for kind in (0, 1)))
    config = directory / 'run.set'
    config.write_text(f'-net {circuit}\n-fault {faults}\n-jobs 15\n-dom_reuse off\n'
                      f'-progress on\n-fdp {directory}/fdp.csv\n-log {directory}/time.log\n')
    (directory / 'time.log').unlink(missing_ok=True)
    master, slave = pty.openpty()
    fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack('HHHH', 24, 80, 0, 0))
    data = bytearray()
    children = set()
    start = time.monotonic()
    with (directory / 'stderr.txt').open('wb') as err:
        process = subprocess.Popen([str(binary), '-set', str(config)], stdout=slave, stderr=err,
                                   cwd=ROOT / 'build', env=dict(os.environ, TERM='xterm'))
        os.close(slave)
        try:
            def collect(frames):
                while len(re.findall(rb'Progress >> \d+/\d+', data)) < frames:
                    assert time.monotonic() - start < 15, data[-4000:]
                    child_file = Path(f'/proc/{process.pid}/task/{process.pid}/children')
                    if child_file.exists():
                        children.update(int(p) for p in child_file.read_text().split())
                    ready, _, _ = select.select([master], [], [], 0.2)
                    if ready:
                        data.extend(os.read(master, 65536))

            collect(2)
            first = bytes(data)
            ansi = rb'\x1b\[[0-9;]*[A-Za-z]'
            plain = re.sub(ansi, b'', first)
            assert {int(w) for w in re.findall(rb'worker (\d+):', plain)} == set(range(1, 16))
            assert b'\x1b[16A' in first, 'worker panel did not reuse its 16 rows'
            assert all(len(line) <= 79 for line in plain.splitlines() if b'worker ' in line)
            offset = len(data)
            fcntl.ioctl(master, termios.TIOCSWINSZ, struct.pack('HHHH', 10, 48, 0, 0))
            collect(3)
            narrow = re.sub(ansi, b'', bytes(data[offset:]))
            assert b'Workers ' in narrow, 'small terminal did not page the worker list'
            assert all(len(line) <= 47 for line in narrow.splitlines())
            assert len([line for line in narrow.split(b'Progress >>')[-1].splitlines() if line]) <= 9
            assert process.poll() is None
            assert len(data) < 50000, 'progress output was not coalesced'
            assert len(re.findall(rb'Progress >> \d+/\d+', data)) <= int(time.monotonic() - start) + 3
        finally:
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=10)
            os.close(master)
            (directory / 'stdout.bin').write_bytes(data)
    assert all(not Path(f'/proc/{pid}').exists() for pid in children)
    assert not (directory / 'time.log').exists()


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
        check_live(args.binary.resolve(), output, jobs, fault_progress=False)
    check_terminal(args.binary.resolve(), output)
    print('Fault completion/loop/seed counts, SAF/TDF, limits, Debug/Release, progress off, '
          'CSV/golden equality, live serial/worker updates and cancellation: PASS')


if __name__ == '__main__':
    main()
