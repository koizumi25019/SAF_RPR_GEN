"""Exercise real fault pipelines, output order, validation, and process failures."""
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
from build import HERE, ROOT, REFERENCE_COMMIT

RUNS = HERE / 'runs'
RESULTS = HERE / 'results'
RELEASE = ROOT / 'build/main_release'
REFERENCE = HERE / 'build/main_reference'
HOOKS = set()
for source in (ROOT / 'src').rglob('*.c'):
    HOOKS.update(re.findall(r'getenv\("([A-Z0-9_]+)"\)', source.read_text()))
BASE_ENV = {k: v for k, v in os.environ.items() if k not in HOOKS}


def settings(label, circuit='c17a', jobs=1, method='xid', limit=0,
             tdf=False, power=None, analysis=False, extra='', reuse='off'):
    p = RUNS / label
    p.mkdir(parents=True, exist_ok=True)
    (p / 'time.log').unlink(missing_ok=True)
    text = (f'-net {ROOT}/input/circuit/{circuit}.v\n-fdp {p}/fdp.csv\n'
            f'-log {p}/time.log\n-dc_method {method}\n-dom_reuse {reuse}\n-limit {limit}\n')
    if jobs is not None:
        text += f'-jobs {jobs}\n'
    if tdf:
        text += '-tdf\n'
    if power is not None:
        text += f'-low_power on\n-wsa_threshold {power}\n'
    if method == 'core':
        text += '-core_verify on\n'
    if analysis:
        text += f'-cube_analysis {p}/series.csv\n'
    path = p / 'run.set'
    path.write_text(text + extra)
    return p, path


def run(label, scope=True, gt=True, binary=RELEASE, **options):
    p, config = settings(label, **options)
    env = dict(BASE_ENV, FDP_NORMAL_SCOPE=str(int(scope)))
    if gt:
        env.update(GT_BDD='1', FDP_NORMAL_SCOPE_VALIDATE='1')
    start = time.monotonic()
    with (p / 'stdout.txt').open('w') as out, (p / 'stderr.txt').open('w') as err:
        subprocess.run([str(binary), '-set', str(config)], cwd=ROOT / 'build',
                       env=env, stdout=out, stderr=err, check=True, timeout=360)
    wall = time.monotonic() - start
    rows = list(csv.DictReader((p / 'fdp.csv').open()))
    representatives = [r for r in rows if r['complete'] in ('0', '1')]
    if gt:
        summaries = re.findall(r'\[GT\] summary: checked=(\d+)\s+UNSOUND=(\d+)\s+'
                               r'complete-but-NOT-exact=(\d+)\s+ALL VERIFIED',
                               (p / 'stderr.txt').read_text())
        assert summaries == [(str(len(representatives)), '0', '0')], (label, summaries)
    return p, rows, wall


def equal(a, b, analysis=False):
    assert (a / 'fdp.csv').read_bytes() == (b / 'fdp.csv').read_bytes(), (a, b)
    if analysis:
        assert (a / 'series.csv').read_bytes() == (b / 'series.csv').read_bytes(), (a, b)


def smoke(record):
    for name in ('c17a', 's27_C', 's208_C', 's298_C', 's344_C',
                 's510_C', 's641_C', 's713_C', 's1494_C'):
        serial, rows, _ = run(name + '_serial', circuit=name)
        parallel, _, _ = run(name + '_parallel', circuit=name, jobs=4)
        reference, _, _ = run(name + '_reference', circuit=name, jobs=None, binary=REFERENCE)
        equal(serial, parallel); equal(serial, reference)
        if name == 'c17a':
            golden = list(csv.DictReader((ROOT / 'expected/c17a_result.csv').open()))
            sig = lambda rs: sorted((r['net_name'], r['f_type'], r['fdp']) for r in rs)
            assert sig(rows) == sig(golden)
            debug, _, _ = run('c17a_debug', jobs=3, binary=ROOT / 'build/main_debug')
            equal(serial, debug)
        record(name, serial_parallel_reference_byte_identical=True, gt='ALL VERIFIED')
    for scope in (False, True):
        serial, _, _ = run(f's208_scope{scope}_serial', circuit='s208_C', scope=scope, limit=2, analysis=True)
        parallel, _, _ = run(f's208_scope{scope}_parallel', circuit='s208_C', scope=scope, limit=2, analysis=True, jobs=4)
        reference, _, _ = run(f's208_scope{scope}_reference', circuit='s208_C', scope=scope, limit=2, analysis=True, jobs=None, binary=REFERENCE)
        equal(serial, parallel, True); equal(serial, reference, True)
        record('limited_analysis_scope' + str(scope), csv_and_series_byte_identical=True)
    for name in ('c17a', 's208_C', 's298_C'):
        serial, _, _ = run(name + '_core_serial', circuit=name, method='core')
        parallel, _, _ = run(name + '_core_parallel', circuit=name, method='core', jobs=4)
        reference, _, _ = run(name + '_core_reference', circuit=name, method='core', jobs=None, binary=REFERENCE)
        equal(serial, parallel); equal(serial, reference)
        record(name + '_core', byte_identical=True, core_verify=True, gt='ALL VERIFIED')
    for name, method, power in (('s27', 'xid', None), ('s208', 'xid', None),
                                 ('s27', 'core', None), ('s27', 'core', 0),
                                 ('s27', 'core', 20), ('s27', 'core', 100),
                                 ('s208', 'core', 20)):
        label = f'{name}_tdf_{method}_{power}'
        options = dict(circuit=name, method=method, power=power, tdf=True)
        serial, _, _ = run(label + '_serial', **options)
        parallel, _, _ = run(label + '_parallel', jobs=4, **options)
        reference, _, _ = run(label + '_reference', jobs=None, binary=REFERENCE, **options)
        equal(serial, parallel); equal(serial, reference)
        record(label, byte_identical=True, gt='ALL VERIFIED')
    for reuse in ('on', 'off'):
        current, _, _ = run('reuse_' + reuse, circuit='s208_C', reuse=reuse)
        old, _, _ = run('reuse_' + reuse + '_reference', circuit='s208_C', reuse=reuse, jobs=None, binary=REFERENCE)
        equal(current, old)
        record('serial_reuse_' + reuse, pre_pool_byte_identical=True)
    one, rows, _ = run('clamp_base')
    faults = RUNS / 'few_faults.txt'
    representatives = [r for r in rows if r['complete'] in ('0', '1')]
    faults.write_text(''.join(f"{r['net_name']}\t{r['f_type']}\n" for r in representatives[:2]))
    a, _, _ = run('few_serial', extra=f'-fault {faults}\n')
    b, _, _ = run('few_parallel', jobs=64, extra=f'-fault {faults}\n')
    equal(a, b)
    empty = RUNS / 'empty_faults.txt'; empty.write_text('')
    a, _, _ = run('empty_serial', extra=f'-fault {empty}\n', gt=False)
    b, _, _ = run('empty_parallel', jobs=4, extra=f'-fault {empty}\n', gt=False)
    equal(a, b)
    record('few_and_empty', same_output=True)
    rejection_tests(record)
    process_failure('worker_killed', signal.SIGKILL, True, record)
    process_failure('parent_cancelled', signal.SIGTERM, False, record)


def rejection_tests(record):
    p, config = settings('settings_checks', jobs=None)
    for value in ('0', '-1', '257', 'abc', '1.5'):
        result = subprocess.run([str(RELEASE), '-set', str(config), '-jobs', value],
                                env=BASE_ENV, capture_output=True, text=True)
        assert result.returncode != 0 and '-jobs requires' in result.stderr, value
    config.write_text(config.read_text() + '-jobs\n')
    result = subprocess.run([str(RELEASE), '-set', str(config)], env=BASE_ENV, capture_output=True, text=True)
    assert result.returncode != 0 and '-jobs requires' in result.stderr
    p, config = settings('reuse_rejected', jobs=4, reuse='on')
    result = subprocess.run([str(RELEASE), '-set', str(config)], env=BASE_ENV, capture_output=True, text=True)
    assert result.returncode != 0 and '-dom_reuse off' in result.stderr
    p, config = settings('hooks_rejected', jobs=4)
    for hook in ('AIG_DUMP', 'AIG_DUMP_DIR', 'DUMP_CNF', 'XID_EXTERNAL',
                 'CUBE_TREND_CSV', 'MAXDC', 'MAXHAM', 'DUAL', 'SPLIT',
                 'PCOUNT', 'BDD_EXACT', 'GT_ISOP'):
        result = subprocess.run([str(RELEASE), '-set', str(config)],
                                env=dict(BASE_ENV, **{hook: '1'}), capture_output=True, text=True)
        assert result.returncode != 0 and hook in result.stderr, hook
    # -jobs can follow -set; omitted reuse becomes off in parallel mode.
    p, config = settings('cli_jobs', jobs=None)
    text = config.read_text().replace('-dom_reuse off\n', '')
    config.write_text(text)
    subprocess.run([str(RELEASE), '-set', str(config), '-jobs', '3'], env=BASE_ENV,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
    assert re.search(r'Fault Workers\s+: 3', (p / 'time.log').read_text())
    assert re.search(r'Dominance Cube Reuse\s+: off', (p / 'time.log').read_text())
    for jobs in (1, 4):
        p, config = settings('write_failure_' + str(jobs), jobs=jobs, extra='-fdp /dev/full\n')
        result = subprocess.run([str(RELEASE), '-set', str(config)], env=BASE_ENV,
                                stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, timeout=20)
        assert result.returncode != 0
    record('settings_and_output_errors', invalid_values_rejected=True, cli_after_set=True,
           parallel_default_reuse_off=True, shared_file_hooks_rejected=True, io_failure_nonzero=True)


def process_failure(label, signum, child, record):
    p, config = settings(label, circuit='s5378_C', jobs=2, limit=30)
    env = dict(BASE_ENV, GT_BDD='1', FDP_NORMAL_SCOPE='1')
    with (p / 'stdout.txt').open('w') as out, (p / 'stderr.txt').open('w') as err:
        process = subprocess.Popen([str(RELEASE), '-set', str(config)], env=env, stdout=out, stderr=err)
        children = []
        try:
            deadline = time.monotonic() + 10
            while time.monotonic() < deadline and process.poll() is None:
                # The task/children file is absent on some kernels; PPid is portable
                # across those Linux configurations and identifies our own workers.
                children = []
                for entry in Path('/proc').iterdir():
                    if not entry.name.isdigit():
                        continue
                    try:
                        status = (entry / 'status').read_text()
                    except (FileNotFoundError, ProcessLookupError):
                        continue
                    match = re.search(r'^PPid:\s+(\d+)$', status, re.M)
                    if match and int(match.group(1)) == process.pid:
                        children.append(int(entry.name))
                if len(children) == 2:
                    break
                time.sleep(0.01)
            assert len(children) == 2, children
            os.kill(children[0] if child else process.pid, signum)
            assert process.wait(timeout=10) != 0
        finally:
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=10)
    assert all(not Path(f'/proc/{pid}').exists() for pid in children), children
    text = (p / 'stderr.txt').read_text()
    assert '[PARALLEL] failed' in text and 'ALL VERIFIED' not in text
    assert not (p / 'time.log').exists()
    record(label, nonzero_exit=True, all_children_reaped=True, no_success_summary=True)


def large(record):
    serial, rows, _ = run('s5378_serial_gt', circuit='s5378_C', limit=30)
    parallel, other, _ = run('s5378_parallel_gt', circuit='s5378_C', limit=30, jobs=4)
    equal(serial, parallel)
    assert sum(r['complete'] in ('0', '1') for r in other) == 4551
    record('s5378_all', faults=4551, byte_identical=True, gt='ALL VERIFIED',
           completed=sum(r['complete'] == '1' for r in rows))


def benchmark(record):
    reference = None
    first_wall = None
    for jobs in (1, 2, 4, 8):
        p, rows, wall = run(f's5378_timing_jobs{jobs}', circuit='s5378_C',
                            limit=30, jobs=jobs, gt=False)
        if reference is None:
            reference, first_wall = p, wall
        equal(reference, p)
        log = (p / 'time.log').read_text()
        cpu = float(re.search(r'CPU Time\s+: ([0-9.]+)', log).group(1))
        record('s5378_jobs' + str(jobs), jobs=jobs, wall_seconds=wall,
               total_cpu_seconds=cpu, speedup=first_wall / wall, byte_identical=True,
               completed=sum(r['complete'] == '1' for r in rows))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--phase', choices=('smoke', 'large', 'benchmark', 'all'), default='smoke')
    args = parser.parse_args()
    RESULTS.mkdir(parents=True, exist_ok=True)
    report = {'reference_commit': REFERENCE_COMMIT, 'repeats_per_timing': 1,
              'binary_sha256': hashlib.sha256(RELEASE.read_bytes()).hexdigest(),
              'cpu_quota': (Path('/sys/fs/cgroup/cpu.max').read_text().strip()
                            if Path('/sys/fs/cgroup/cpu.max').exists() else None), 'checks': []}
    def record(label, **data):
        report['checks'].append(dict(label=label, **data))
        (RESULTS / (args.phase + '.json')).write_text(json.dumps(report, indent=2) + '\n')
        print(json.dumps(report['checks'][-1]), flush=True)
    for phase in ('smoke', 'large', 'benchmark'):
        if args.phase in (phase, 'all'):
            globals()[phase](record)
    print('PASS fault pool', args.phase, flush=True)


if __name__ == '__main__':
    main()
