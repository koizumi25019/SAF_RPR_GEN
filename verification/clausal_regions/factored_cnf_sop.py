"""Enumerate D as a SAT-certified factored cover H AND (Q1 OR ... OR Qn).

H is a conjunction of bounded clauses proved to be implicates of D.  Each Qi
is a predicate cube generalized under H, so H AND Qi implies D.  This keeps a
CNF-like condition factored instead of distributing it into every PI cube.
"""
from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys
import time
from collections import Counter

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / "predicate_sop"))
sys.path.insert(0, str(HERE.parent / "factored_sop"))
from clausal import (  # noqa: E402
    clauses_up_to_two,
    count_union,
    expression_clause,
    write_cnf,
)
from fast_basis import FastNet  # noqa: E402
from predicates import prepare, select  # noqa: E402
from probe import SAT  # noqa: E402


def remove_subsumed(clauses):
    units = {clause[0] for clause in clauses if len(clause) == 1}
    return [
        clause
        for clause in clauses
        if len(clause) == 1 or not any(literal in units for literal in clause)
    ]


def propagated_implicates(cnf, output_literal, predicates):
    if not predicates:
        return []
    command = [
        str(HERE / "build" / "propagate_implicates"),
        str(cnf),
        str(output_literal),
        *[str(atom) for atom, _, _ in predicates],
    ]
    process = subprocess.run(command, text=True, capture_output=True, check=True)
    lines = process.stdout.splitlines()
    if not lines:
        raise RuntimeError("Empty propagation implicate output")
    if lines[0] == "UNSAT":
        # An untestable fault has the empty cover. The detector below confirms
        # UNSAT; no common clauses are needed in this case.
        return []
    count = int(lines[0])
    clauses = []
    for line in lines[1:]:
        values = list(map(int, line.split()))
        if values[0] != len(values) - 1:
            raise RuntimeError("Malformed propagation implicate output")
        clauses.append(tuple(values[1:]))
    if len(clauses) != count:
        raise RuntimeError("Truncated propagation implicate output")
    return clauses


def run(netpath, fault, stuck, prefix, width=3, seconds=10, limit=100000,
        minimize=True, implicate_mode="all"):
    prefix = pathlib.Path(prefix)
    prefix.parent.mkdir(parents=True, exist_ok=True)
    total_start = time.monotonic()
    net = FastNet(netpath)
    ex, output, rows = prepare(net, fault, stuck)
    predicates = select(ex, output, width, frontier=True)
    candidate_count = 2 * len(predicates) * len(predicates)
    clauses, variables, output_literal = ex.cnf(output)
    cnf = prefix.with_suffix(".cnf")
    write_cnf(cnf, clauses, variables)

    implication_start = time.monotonic()
    on = SAT(cnf, variables)
    on.add([output_literal])
    implicates = []
    try:
        if implicate_mode == "all":
            candidates = clauses_up_to_two(len(predicates))
        elif implicate_mode == "propagation":
            candidates = propagated_implicates(cnf, output_literal, predicates)
        else:
            raise ValueError("implicate_mode must be all or propagation")
        # The propagation result is already sound; these explicit SAT checks
        # keep the same semantic certificate boundary as exhaustive learning.
        for clause in candidates:
            negated = [-literal for literal in expression_clause(clause, predicates)]
            if on.solve(negated)[0] == 20:
                implicates.append(clause)
    finally:
        on.close()
    implicates = remove_subsumed(implicates)
    implication_seconds = time.monotonic() - implication_start

    det = SAT(cnf, variables)
    off = SAT(cnf, variables)
    det.add([output_literal])
    off.add([-output_literal])
    for clause in implicates:
        off.add(expression_clause(clause, predicates))

    atom_to_predicate = {atom: index + 1 for index, (atom, _, _) in enumerate(predicates)}
    regions = []
    cubes = []
    core_hist = Counter()
    minimize_calls = 0
    complete = False
    timed_out = False
    enumeration_start = time.monotonic()
    try:
        while len(cubes) < limit:
            if time.monotonic() - enumeration_start >= seconds:
                timed_out = True
                break
            rc, model = det.solve()
            if rc == 20:
                complete = True
                break
            fixed = [
                atom if model[atom - 1] > 0 else -atom
                for atom, _, _ in predicates
            ]
            rc, core = off.solve(fixed)
            if rc != 20:
                raise RuntimeError("Frontier assignment did not determine detection")
            fixed_set = set(fixed)
            fixed = [literal for literal in core if literal in fixed_set]
            if minimize:
                for candidate in list(fixed):
                    if time.monotonic() - enumeration_start >= seconds:
                        timed_out = True
                        break
                    if candidate not in fixed:
                        continue
                    trial = [literal for literal in fixed if literal != candidate]
                    rc, smaller = off.solve(trial)
                    minimize_calls += 1
                    if rc == 20:
                        trial_set = set(trial)
                        fixed = [literal for literal in smaller if literal in trial_set]
                if timed_out:
                    break
            if off.solve(fixed)[0] != 20:
                raise RuntimeError("Factored region is not an implicant")
            predicate_cube = [
                atom_to_predicate[abs(literal)] * (1 if literal > 0 else -1)
                for literal in fixed
            ]
            cubes.append(predicate_cube)
            regions.append(list(implicates) + [(literal,) for literal in predicate_cube])
            # D implies H, hence blocking Q is exactly blocking H AND Q while
            # searching under D. No CNF encoding of NOT(H AND Q) is needed.
            det.add([-literal for literal in fixed])
            core_hist[len(fixed)] += 1
        if not complete and not timed_out:
            complete = det.solve()[0] == 20
    finally:
        det.close()
        off.close()
    enumeration_seconds = time.monotonic() - enumeration_start

    union = count_union(prefix, ex.n, predicates, regions) if complete else None
    prefix.with_suffix(".regions.json").write_text(json.dumps({
        "nvars": ex.n,
        "predicates": [
            {"expression_atom": atom, "group": group, "mask": mask}
            for atom, group, mask in predicates
        ],
        "common_cnf": implicates,
        "residual_cubes": cubes,
        "regions": regions,
    }))
    result = {
        "fault": fault,
        "stuck": stuck,
        "width": width,
        "implicate_mode": implicate_mode,
        "predicates": len(predicates),
        "candidate_clauses": candidate_count,
        "tested_clauses": len(candidates),
        "common_implicates": len(implicates),
        "regions": len(regions),
        "complete": complete,
        "timed_out": timed_out,
        "residual_care_hist": dict(sorted(core_hist.items())),
        "implication_sat_calls": on.calls,
        "enumeration_sat_calls": det.calls + off.calls,
        "minimize_calls": minimize_calls,
        "implication_seconds": implication_seconds,
        "enumeration_seconds": enumeration_seconds,
        "union": union,
        "total_seconds": time.monotonic() - total_start,
    }
    prefix.with_suffix(".metadata.json").write_text(json.dumps({
        "net": str(netpath),
        "fault": fault,
        "stuck": stuck,
        "ncoords": ex.n,
        "basis_rows": [hex(row) for row in rows],
        "pi_order": net.pi,
    }, indent=2))
    prefix.with_suffix(".result.json").write_text(json.dumps(result, indent=2))
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--net", required=True)
    parser.add_argument("--fault", required=True)
    parser.add_argument("--stuck", type=int, default=0)
    parser.add_argument("--prefix", required=True)
    parser.add_argument("--width", type=int, default=3)
    parser.add_argument("--seconds", type=float, default=10)
    parser.add_argument("--limit", type=int, default=100000)
    parser.add_argument("--core-only", action="store_true")
    parser.add_argument("--implicates", choices=("all", "propagation"), default="all")
    args = parser.parse_args()
    print(json.dumps(run(args.net, args.fault, args.stuck, args.prefix,
                         args.width, args.seconds, args.limit,
                         not args.core_only, args.implicates)))
