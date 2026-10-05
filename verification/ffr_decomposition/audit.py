"""Structural FFR size audit; no probabilities or performance extrapolation."""
import collections
import json
from ffr import HERE, ROOT, Netlist

rows = []
for circuit in ['s5378_C', 's38584_C', 'b19_C']:
    net = Netlist(ROOT/'input/circuit'/f'{circuit}.v')
    fanout = collections.defaultdict(list)
    for output, (_, inputs) in net.g.items():
        for signal in inputs:
            fanout[signal].append(output)
    observed = set(net.po)
    root, depth = {}, {}
    for signal in net.pi+list(net.g):
        chain = []
        current = signal
        while current not in root and current not in observed and len(fanout[current]) == 1:
            chain.append(current)
            current = fanout[current][0]
        if current not in root:
            root[current], depth[current] = current, 0
        for member in reversed(chain):
            root[member] = root[current]
            depth[member] = depth[current]+1
            current = member
    signals = net.pi+list(net.g)
    sizes = collections.Counter(root[s] for s in signals)
    row = {'circuit': circuit, 'signals': len(signals), 'gates': len(net.g),
           'ffr_roots': len(sizes), 'max_signals_per_ffr': max(sizes.values()),
           'path_depth_hist': dict(sorted(collections.Counter(depth[s] for s in signals).items()))}
    target = 'P2_P1_P1_U3002'
    if target in root:
        row['previous_hard_fault'] = {'signal': target, 'root': root[target], 'path_gates': depth[target]}
    rows.append(row)
    print(json.dumps(row), flush=True)
(HERE/'results').mkdir(exist_ok=True)
(HERE/'results/structural_audit.json').write_text(json.dumps(rows, indent=2))
