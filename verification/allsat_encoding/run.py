"""Compare encodings and DC starts on identical prepared fault functions."""
import argparse
import csv
import importlib.util
import json
import pathlib
import subprocess
import sys
import time
from fractions import Fraction

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE.parent/'factored_sop'))
from fast_basis import FastNet, build_fast, compact, probe
from experiment import run_fault, write_cnf, groups_for, state_encoding
from encoding import Encoded


def prepare(net, fault, stuck):
    old, out, rows, _, npar, _ = build_fast(net, fault, stuck, difference=True)
    ex, out, active = compact(old, out)
    return ex, out, [rows[i-1] for i in active], [], npar, set()


def run(net, fault, stuck, prefix, prepared, encoding='nnf_pg',
        method='greedy', seed=False, width=1, seconds=3, limit=100000):
    start = time.monotonic()
    ex, *rest = prepared
    wrapped = Encoded(ex, encoding)
    clauses, nv, _ = wrapped.cnf(rest[0])
    options = ['--witness-seed', str(int(seed))]
    if encoding == 'nnf_pg_split':
        negative = list(wrapped.off_clauses)
        off_nv = nv
        if width > 1:
            extra, off_nv, _ = state_encoding(groups_for(ex, rest[0], width), nv)
            negative += extra
        prefix.parent.mkdir(parents=True, exist_ok=True)
        off_path = prefix.with_suffix('.off.cnf')
        write_cnf(off_path, negative, off_nv)
        options += ['--off-cnf', str(off_path)]
    try:
        r = run_fault(net, fault, stuck, prefix, prepared=(wrapped, *rest),
                      width=width, method=method, seconds=seconds, limit=limit,
                      simulate=method != 'witness', native_binary=HERE/'build/enumerator',
                      native_options=options,
                      count_partial=False, union_timeout=10, union_order='large')
    except subprocess.TimeoutExpired as e:
        if not str(e.cmd[0]).endswith(('region_union', 'cube_union')):
            raise
        r = json.loads(prefix.with_suffix('.result.json').read_text())
        r.update(enumeration_complete=r['complete'], complete=False, union=None,
                 cause='bdd_timeout')
    r.update(fault=fault, stuck=stuck, encoding=encoding, seed=seed, width=width,
             cnf_vars=nv, cnf_clauses=len(clauses), total_seconds=time.monotonic()-start)
    prefix.with_suffix('.comparison.json').write_text(json.dumps(r, indent=2))
    return r


def certificate(netpath, prefix):
    # Load the existing independent raw-gate verifier without changing it.
    spec = importlib.util.spec_from_file_location('encoding_raw_verify', HERE.parent/'factored_sop/verify.py')
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    meta = json.loads(prefix.with_suffix('.metadata.json').read_text())
    result = json.loads(prefix.with_suffix('.result.json').read_text())
    union = json.loads(prefix.with_suffix('.union.json').read_text())
    n = len(meta['basis_rows'])
    data = {'fault': meta['fault'], 'stuck': meta['stuck'], 'basis_rows': meta['basis_rows'],
            'ncoords': n, 'tree': {'kind': 'cover', 'prefix': str(prefix),
                'map': {i:i for i in range(1, n+1)}, 'complete': result['complete'],
                'probability': union['probability']}}
    manifest = prefix.with_suffix('.cover.json')
    manifest.write_text(json.dumps(data))
    return mod.verify(netpath, manifest)


def pilot():
    cases = [('s5378_C','n673gat',0), ('s5378_C','n1592gat',0),
             ('s5378_C','n291gat',1), ('s38584_C','g16349',1),
             ('b19_C','P2_P1_P1_U3002',0)]
    nets, results = {}, []
    for circuit, fault, stuck in cases:
        netpath = ROOT/'input/circuit'/f'{circuit}.v'
        if circuit not in nets:
            nets[circuit] = FastNet(netpath)
        net = nets[circuit]
        prepared = prepare(net, fault, stuck)
        expected = None
        for encoding in ['tseitin', 'nnf_pg']:
            for method, seed, width in [('greedy',False,1), ('greedy',True,1),
                                         ('witness',True,1), ('greedy',False,3)]:
                label = f'{circuit}_{fault}_{stuck}_{encoding}_{method}_{int(seed)}_w{width}'
                prefix = HERE/'runs/pilot'/label
                r = run(net, fault, stuck, prefix, prepared, encoding, method, seed, width)
                if r['complete']:
                    p = Fraction(r['union']['probability'])
                    if expected is not None:
                        assert p == expected
                    expected = p
                    r['certificate'] = certificate(netpath, prefix)
                results.append(r)
                print(json.dumps({k:r[k] for k in ['fault','encoding','method','seed','width','complete','cubes','seconds','total_seconds']}), flush=True)
                (HERE/'results/pilot.json').write_text(json.dumps(results, indent=2))
    fields = ['fault','stuck','encoding','method','seed','width','complete','cubes','seconds',
              'sat_calls','cnf_vars','cnf_clauses','witness_seconds','total_seconds']
    with (HERE/'results/pilot.csv').open('w') as f:
        w = csv.DictWriter(f, fields, extrasaction='ignore')
        w.writeheader(); w.writerows(results)


if __name__ == '__main__':
    (HERE/'results').mkdir(exist_ok=True)
    pilot()
