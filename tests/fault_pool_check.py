"""Baseline integration: compare full CSVs, settings, worker cleanup and s5378.
Run after the CMake build; --reference points to a build of baseline ac45287.
Optional --capture/--checker verify actual generated covers independently.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
RUNS = ROOT / 'build/fault_pool_checks/runs'
RELEASE = ROOT / 'build/main_release'
HOOKS = {'MDC_NODOM', 'FDP_NORMAL_SCOPE', 'BASELINE_COVER_DIR', 'GT_BDD'}
for source in (ROOT / 'src').rglob('*.c'):
    HOOKS.update(re.findall(r'getenv\("([A-Z0-9_]+)"\)', source.read_text()))
ENV = {key: value for key, value in os.environ.items() if key not in HOOKS}


def settings(label, circuit='c17a', jobs=1, limit=0, reuse='off', extra=''):
    directory = RUNS / label
    directory.mkdir(parents=True, exist_ok=True)
    (directory / 'time.log').unlink(missing_ok=True)
    text = (f'-net {ROOT}/input/circuit/{circuit}.v\n'
            f'-fdp {directory}/fdp.csv\n-log {directory}/time.log\n-limit {limit}\n')
    if jobs is not None:
        text += f'-jobs {jobs}\n-dom_reuse {reuse}\n'
    config = directory / 'run.set'
    config.write_text(text + extra)
    return directory, config


def run(label, binary=RELEASE, env_extra=None, **options):
    directory, config = settings(label, **options)
    env = dict(ENV, **(env_extra or {}))
    if options.get('jobs', 1) is None and options.get('reuse', 'off') == 'off':
        env['MDC_NODOM'] = '1'
    start = time.monotonic()
    with (directory / 'stdout.txt').open('w') as out, (directory / 'stderr.txt').open('w') as err:
        subprocess.run([str(binary), '-set', str(config)], cwd=ROOT / 'build',
                       env=env, stdout=out, stderr=err, check=True, timeout=360)
    return directory, time.monotonic() - start


def equal(*directories):
    expected = (directories[0] / 'fdp.csv').read_bytes()
    assert all((item / 'fdp.csv').read_bytes() == expected for item in directories[1:])


def child_pids(parent_pid):
    children = []
    for entry in Path('/proc').iterdir():
        if not entry.name.isdigit():
            continue
        try:
            status = (entry / 'status').read_text()
        except (FileNotFoundError, ProcessLookupError):
            continue
        match = re.search(r'^PPid:\s+(\d+)$', status, re.M)
        if match and int(match.group(1)) == parent_pid:
            children.append(int(entry.name))
    return children


def process_failure(label, kill_child):
    directory, config = settings(label, circuit='s5378_C', jobs=2, limit=30)
    with (directory / 'stdout.txt').open('w') as out, (directory / 'stderr.txt').open('w') as err:
        process = subprocess.Popen([str(RELEASE), '-set', str(config)], env=ENV,
                                   stdout=out, stderr=err)
        children = []
        try:
            deadline = time.monotonic() + 10
            while time.monotonic() < deadline and process.poll() is None:
                children = child_pids(process.pid)
                if len(children) == 2:
                    break
                time.sleep(0.01)
            assert len(children) == 2, children
            os.kill(children[0] if kill_child else process.pid,
                    signal.SIGKILL if kill_child else signal.SIGTERM)
            assert process.wait(timeout=10) != 0
        finally:
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=10)
    assert all(not Path(f'/proc/{pid}').exists() for pid in children)
    assert '[PARALLEL] failed' in (directory / 'stderr.txt').read_text()
    assert not (directory / 'time.log').exists()


def check_settings():
    directory, config = settings('settings', jobs=None)
    for value in ('0', '-1', '257', 'abc', '1.5'):
        result = subprocess.run([str(RELEASE), '-set', str(config), '-jobs', value],
                                env=ENV, capture_output=True, text=True)
        assert result.returncode != 0 and '-jobs requires' in result.stderr
    for extra, message in (('-jobs\n', '-jobs requires'),
                           ('-jobs 4\n-dom_reuse on\n', '-dom_reuse off')):
        config.write_text(config.read_text().split('-jobs')[0] + extra)
        result = subprocess.run([str(RELEASE), '-set', str(config)], env=ENV,
                                capture_output=True, text=True)
        assert result.returncode != 0 and message in result.stderr
    directory, config = settings('cli_override', jobs=2)
    subprocess.run([str(RELEASE), '-set', str(config), '-jobs', '4'], env=ENV,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
    assert re.search(r'Fault Workers\s+: 4', (directory / 'time.log').read_text())
    directory, config = settings('implicit_reuse', jobs=None, extra='-jobs 3\n')
    subprocess.run([str(RELEASE), '-set', str(config)], env=ENV,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
    assert re.search(r'Dominance Cube Reuse\s+: off', (directory / 'time.log').read_text())
    for jobs in (1, 4):
        for option in ('fdp', 'log'):
            _, config = settings(f'write_failure_{option}_{jobs}', jobs=jobs,
                                 extra=f'-{option} /dev/full\n')
            result = subprocess.run([str(RELEASE), '-set', str(config)], env=ENV,
                                    stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, timeout=20)
            assert result.returncode != 0


def independent_gt(capture, checker, circuit, limit):
    label = circuit + '_capture'
    directory, _ = settings(label, circuit=circuit, jobs=4, limit=limit)
    for old in directory.glob('*.cover'):
        old.unlink()
    actual, _ = run(label, binary=capture, circuit=circuit, jobs=4, limit=limit,
                    env_extra={'BASELINE_COVER_DIR': str(directory), 'FDP_NORMAL_SCOPE_VALIDATE': '1'})
    equal(actual, RUNS / (circuit + '_parallel'))
    rows = list(csv.DictReader((actual / 'fdp.csv').open()))
    representatives = [row for row in rows if row['complete'] in ('0', '1')]
    faults = actual / 'faults.txt'
    faults.write_text(''.join(f"{row['net_name']}\t{row['f_type']}\n" for row in representatives))
    gt_config = actual / 'gt.set'
    gt_config.write_text(f'-net {ROOT}/input/circuit/{circuit}.v\n-fault {faults}\n'
                         f'-fdp {actual}/unused.csv\n-log {actual}/gt.log\n'
                         '-dc_method xid\n-dom_reuse off\n')
    result = subprocess.run([str(checker), '-set', str(gt_config)],
                            env=dict(ENV, BASELINE_COVER_DIR=str(actual), GT_BDD='1'),
                            capture_output=True, text=True, check=True, timeout=360)
    (actual / 'gt.stderr.txt').write_text(result.stderr)
    summaries = re.findall(r'checked=(\d+)\s+UNSOUND=(\d+)\s+complete-but-NOT-exact=(\d+)\s+ALL VERIFIED',
                           result.stderr)
    assert summaries == [(str(len(representatives)), '0', '0')], summaries
    return len(representatives)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--capture', type=Path)
    parser.add_argument('--checker', type=Path)
    args = parser.parse_args()
    args.reference = args.reference.resolve()
    if args.capture: args.capture = args.capture.resolve()
    if args.checker: args.checker = args.checker.resolve()
    assert bool(args.capture) == bool(args.checker)
    report = {'reference_commit': 'ac45287', 'normal_cnf_scope': 'always on',
              'binary_sha256': hashlib.sha256(RELEASE.read_bytes()).hexdigest(),
              'timing_repeats': 1,
              'cpu_quota': (Path('/sys/fs/cgroup/cpu.max').read_text().strip()
                            if Path('/sys/fs/cgroup/cpu.max').exists() else None),
              'checks': []}
    output = ROOT / 'docs/fault_parallel_results.json'
    def record(label, **data):
        report['checks'].append(dict(label=label, **data))
        output.parent.mkdir(exist_ok=True)
        output.write_text(json.dumps(report, indent=2) + '\n')
        print(json.dumps(report['checks'][-1]), flush=True)

    for circuit in ('c17a', 's27_C', 's208_C', 's298_C', 's344_C', 's510_C', 's641_C', 's713_C', 's1494_C'):
        serial, _ = run(circuit + '_serial', circuit=circuit)
        parallel, _ = run(circuit + '_parallel', circuit=circuit, jobs=4)
        old, _ = run(circuit + '_old', binary=args.reference, circuit=circuit, jobs=None)
        equal(serial, parallel, old)
        record(circuit, serial_parallel_previous_csv_identical=True)
        if circuit == 'c17a':
            rows = list(csv.DictReader((serial / 'fdp.csv').open()))
            golden = list(csv.DictReader((ROOT / 'expected/c17a_result.csv').open()))
            sig = lambda rs: sorted((row['net_name'], row['f_type'], row['fdp']) for row in rs)
            assert sig(rows) == sig(golden)
            debug, _ = run('c17a_debug', binary=ROOT / 'build/main_debug', jobs=4)
            no_toggle, _ = run('c17a_old_toggle', env_extra={'FDP_NORMAL_SCOPE': '0'})
            equal(serial, debug, no_toggle)
            assert re.search(r'Normal CNF Scope\s+: on', (no_toggle / 'time.log').read_text())
            record('golden_debug_always_scoped', passed=True)
    for reuse in ('on', 'off'):
        new, _ = run('reuse_' + reuse, circuit='s208_C', reuse=reuse)
        old, _ = run('reuse_old_' + reuse, binary=args.reference, circuit='s208_C', jobs=None, reuse=reuse)
        equal(new, old)
        record('serial_reuse_' + reuse, previous_csv_identical=True)
    empty = RUNS / 'empty.txt'; empty.write_text('')
    first, _ = run('empty_serial', extra=f'-fault {empty}\n')
    second, _ = run('empty_parallel', jobs=4, extra=f'-fault {empty}\n')
    equal(first, second)
    few = RUNS / 'few.txt'
    rows = list(csv.DictReader((RUNS / 'c17a_serial/fdp.csv').open()))
    few.write_text(''.join(f"{row['net_name']}\t{row['f_type']}\n" for row in rows if row['complete'] in ('0','1'))[:0])
    representatives = [row for row in rows if row['complete'] in ('0','1')][:2]
    few.write_text(''.join(f"{row['net_name']}\t{row['f_type']}\n" for row in representatives))
    first, _ = run('few_serial', extra=f'-fault {few}\n')
    second, _ = run('few_parallel', jobs=64, extra=f'-fault {few}\n')
    equal(first, second)
    assert re.search(r'Fault Workers\s+: 2', (second / 'time.log').read_text())
    record('empty_and_worker_clamp', passed=True)
    check_settings(); record('settings_and_io_failure', passed=True)
    for label, child in (('worker_killed', True), ('parent_cancelled', False)):
        process_failure(label, child); record(label, nonzero_exit=True, all_workers_reaped=True)
    serial, wall = run('s5378_C_serial', circuit='s5378_C', limit=30)
    parallel, parallel_wall = run('s5378_C_parallel', circuit='s5378_C', jobs=4, limit=30)
    old, _ = run('s5378_C_old', binary=args.reference, circuit='s5378_C', jobs=None, limit=30)
    equal(serial, parallel, old)
    rows = list(csv.DictReader((parallel / 'fdp.csv').open()))
    assert sum(row['complete'] in ('0','1') for row in rows) == 4551
    record('s5378_C_all', faults=4551, limit=30, serial_parallel_previous_csv_identical=True,
           completed=sum(row['complete']=='1' for row in rows), single_serial_wall_seconds=wall,
           single_parallel4_wall_seconds=parallel_wall)
    if args.checker:
        for circuit, limit in (('c17a',0),('s208_C',0),('s5378_C',30)):
            count = independent_gt(args.capture, args.checker, circuit, limit)
            record(circuit + '_independent_gt', faults=count, gt='ALL VERIFIED', actual_csv_matches_capture=True)
    report['passed'] = True
    output.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS baseline fault parallelization', flush=True)

if __name__ == '__main__':
    main()
