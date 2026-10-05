"""Independently prove clausal covers against the original gate-level CNF."""
from __future__ import annotations

import argparse
import collections
import importlib.util
import json
import pathlib
import sys
import time

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "factored_sop"))
from fast_basis import probe  # noqa: E402

spec = importlib.util.spec_from_file_location(
    "clausal_raw", HERE.parent / "linear_coordinates" / "verify.py"
)
raw = importlib.util.module_from_spec(spec)
spec.loader.exec_module(raw)


def write_cnf(path, clauses, variables):
    path.write_text(
        f"p cnf {variables} {len(clauses)}\n"
        + "".join(" ".join(map(str, clause)) + " 0\n" for clause in clauses)
    )


def verify(netpath, prefix, complete=True):
    start = time.monotonic()
    prefix = pathlib.Path(prefix)
    metadata = json.loads(prefix.with_suffix(".metadata.json").read_text())
    cover = json.loads(prefix.with_suffix(".regions.json").read_text())
    net = probe.Netlist(netpath)
    observed = len(net.po)
    if len(net.g) > 10000:
        fanout = collections.defaultdict(list)
        for output, (_, inputs) in net.g.items():
            for source in inputs:
                fanout[source].append(output)
        affected = {metadata["fault"]}
        stack = [metadata["fault"]]
        while stack:
            for output in fanout[stack.pop()]:
                if output not in affected:
                    affected.add(output)
                    stack.append(output)
        net.po = [output for output in net.po if output in affected]

    rows = [int(row, 16) for row in metadata["basis_rows"]]
    pivots = {}
    for row in rows:
        value = row
        while value:
            pivot = value.bit_length() - 1
            if pivot not in pivots:
                pivots[pivot] = value
                break
            value ^= pivots[pivot]
        assert value, "Dependent coordinate rows"

    clauses, variables, detection = raw.raw_cnf(
        net, metadata["fault"], metadata["stuck"], rows
    )
    variables += 1
    one = variables
    clauses.append([one])
    cache = {}

    def and_(literals):
        nonlocal variables
        literals = set(literals)
        if -one in literals or any(-literal in literals for literal in literals):
            return -one
        literals.discard(one)
        if not literals:
            return one
        if len(literals) == 1:
            return next(iter(literals))
        key = ("and", tuple(sorted(literals)))
        if key not in cache:
            variables += 1
            result = variables
            clauses.extend([[-result, literal] for literal in key[1]])
            clauses.append([result] + [-literal for literal in key[1]])
            cache[key] = result
        return cache[key]

    def or_(literals):
        return -and_([-literal for literal in literals])

    predicate_literals = []
    for predicate in cover["predicates"]:
        group, mask = predicate["group"], predicate["mask"]
        states = []
        for state in range(1 << len(group)):
            if mask >> state & 1:
                states.append(and_([
                    var if state >> index & 1 else -var
                    for index, var in enumerate(group)
                ]))
        predicate_literals.append(or_(states))

    regions = []
    for region in cover["regions"]:
        region_clauses = []
        for clause in region:
            region_clauses.append(or_([
                predicate_literals[abs(literal) - 1]
                * (1 if literal > 0 else -1)
                for literal in clause
            ]))
        regions.append(and_(region_clauses))
    covered = or_(regions)

    cnf = prefix.with_suffix(".raw.cnf")
    write_cnf(cnf, clauses, variables)
    solver = probe.SAT(cnf, 0)
    try:
        sound_rc, _ = solver.solve([covered, -detection])
        if sound_rc != 20:
            raise AssertionError("Unsound clausal cover")
        exact_rc, _ = solver.solve([-covered, detection])
        if complete and exact_rc != 20:
            raise AssertionError("Incomplete clausal cover claimed complete")
    finally:
        solver.close()
    result = {
        "fault": metadata["fault"],
        "stuck": metadata["stuck"],
        "regions": len(regions),
        "sound": True,
        "exact": exact_rc == 20,
        "coordinate_rank": len(rows),
        "original_outputs": observed,
        "affected_outputs_verified": len(net.po),
        "seconds": time.monotonic() - start,
    }
    prefix.with_suffix(".verified.json").write_text(json.dumps(result, indent=2))
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--net", required=True)
    parser.add_argument("--prefix", required=True)
    parser.add_argument("--partial", action="store_true")
    args = parser.parse_args()
    print(json.dumps(verify(args.net, args.prefix, not args.partial)))
