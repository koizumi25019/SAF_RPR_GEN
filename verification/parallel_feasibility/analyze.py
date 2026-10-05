"""Analyze measured fault durations and reuse dependency scheduling bounds."""
from pathlib import Path
import collections
import csv
import heapq
import json
import hashlib
import os
import re

HERE = Path(__file__).resolve().parent

def load_graph():
    rows = list(csv.DictReader((HERE / 'runs/s5378_C/snapshot/snapshot.csv').open()))
    dependencies = {r['name']+'/'+r['type']: r['dependencies'].split(';') if r['dependencies'] else [] for r in rows}
    order = {r['name']+'/'+r['type']: i for i, r in enumerate(sorted(rows, key=lambda r: int(r['level'])))}
    return rows, dependencies, order

def components(dependencies):
    parents = {k: k for k in dependencies}
    def find(key):
        while key != parents[key]:
            parents[key] = parents[parents[key]]
            key = parents[key]
        return key
    for key, deps in dependencies.items():
        for dep in deps:
            parents[find(key)] = find(dep)
    groups = collections.defaultdict(list)
    for key in dependencies:
        groups[find(key)].append(key)
    return list(groups.values())

def measured_times(label):
    pattern = re.compile(r'^\[PF_FAULT\] (\S+) (\S+) (\S+)$', re.MULTILINE)
    return {name+'/'+kind: float(seconds) for name, kind, seconds in
            pattern.findall((HERE / 'runs' / label / 'stderr.log').read_text())}

def schedule(dependencies, order, durations, workers):
    children = collections.defaultdict(list)
    remaining = {key: len(deps) for key, deps in dependencies.items()}
    ready = []
    for key, deps in dependencies.items():
        if not deps:
            heapq.heappush(ready, (order[key], key))
        for dep in deps:
            children[dep].append(key)
    running = []
    now, done = 0.0, 0
    while ready or running:
        while ready and len(running) < workers:
            _, key = heapq.heappop(ready)
            heapq.heappush(running, (now+durations[key], key))
        now, key = heapq.heappop(running)
        done += 1
        for child in children[key]:
            remaining[child] -= 1
            if remaining[child] == 0:
                heapq.heappush(ready, (order[child], child))
    assert done == len(dependencies)
    return now

def main():
    rows, dependencies, order = load_graph()
    durations = measured_times('s5378_C/r0/baseline')
    assert set(durations) == set(dependencies)
    # Actual serial sequence gives the tie-break order used by SetTarget.
    order = {key: i for i, key in enumerate(durations)}
    finish = {}
    for key in sorted(dependencies, key=order.get):
        finish[key] = durations[key] + max((finish[d] for d in dependencies[key]), default=0.0)
    total = sum(durations.values())
    span = max(finish.values())
    groups = components(dependencies)
    report = {'faults': len(rows), 'edges': sum(map(len, dependencies.values())),
              'dependent_faults': sum(bool(d) for d in dependencies.values()),
              'weak_components': len(groups), 'largest_component': max(map(len, groups)),
              'component_size_histogram': dict(collections.Counter(map(len, groups))),
              'fault_work_seconds': total, 'critical_path_seconds': span,
              'heaviest_faults': sorted(durations.items(), key=lambda p: -p[1])[:10],
              'model': 'Fixed serial fault costs, ideal workers, no cache/bandwidth/IPC/initialization costs',
              'scheduling': {}}
    def representative_rows(label):
        return {(r['net_name'], r['f_type']): r for r in
                csv.DictReader((HERE / 'runs' / label / 'fdp.csv').open()) if r['complete']}
    baseline = representative_rows('s5378_C/r0/baseline')
    nodom = representative_rows('s5378_C/r0/nodom')
    assert baseline.keys() == nodom.keys()
    common_exact = [k for k in baseline if baseline[k]['complete'] == nodom[k]['complete'] == '1']
    assert all(baseline[k]['fdp'] == nodom[k]['fdp'] for k in common_exact)
    report['removing_reuse'] = {
        'complete_transitions': dict(collections.Counter(
            baseline[k]['complete']+'->'+nodom[k]['complete'] for k in baseline)),
        'common_complete_equal_fdp': len(common_exact),
        'changed_fdp_rows': sum(baseline[k]['fdp'] != nodom[k]['fdp'] for k in baseline),
        'changed_representative_rows': sum(baseline[k] != nodom[k] for k in baseline)}
    manifest = json.loads((HERE / 'build/source_hashes.json').read_text())
    root = HERE.parent.parent
    assert all(hashlib.sha256((root / name).read_bytes()).hexdigest() == sha for name, sha in manifest.items())
    (HERE / 'results/source_hashes.json').write_text(json.dumps(manifest, indent=2)+'\n')
    report['production_sources_unchanged'] = True
    report['available_logical_cpus'] = os.cpu_count()
    for workers in (1, 2, 4, 8, 16):
        scheduled = schedule(dependencies, order, durations, workers)
        report['scheduling'][workers] = {'lower_bound_seconds': max(total/workers, span),
                                         'ready_queue_seconds': scheduled,
                                         'work_speedup': total/scheduled}
    (HERE / 'results/dependencies.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))

if __name__ == '__main__':
    main()
