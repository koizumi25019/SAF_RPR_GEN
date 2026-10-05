"""Learn a bounded CNF for a detection function with SAT equivalence proofs.

For every candidate local clause C, SAT proves whether D implies C.  The
conjunction H of retained implicates over-approximates D.  A second SAT query
proves H implies D.  If so, H is one exact clausal cover region; no DNF/SOP
model or implicant enumeration is performed.
"""
from __future__ import annotations

import argparse
import json
import pathlib
import sys
import time

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


def learn(netpath, fault, stuck, prefix, width=1, minimize=True):
    prefix = pathlib.Path(prefix)
    prefix.parent.mkdir(parents=True, exist_ok=True)
    total_start = time.monotonic()
    net = FastNet(netpath)
    ex, output, rows = prepare(net, fault, stuck)
    predicates = select(ex, output, width, frontier=True)
    candidates = clauses_up_to_two(len(predicates))
    clauses, variables, output_literal = ex.cnf(output)
    cnf = prefix.with_suffix(".cnf")
    write_cnf(cnf, clauses, variables)

    on = SAT(cnf, variables)
    on.add([output_literal])
    implicates = []
    implication_start = time.monotonic()
    try:
        for clause in candidates:
            assumptions = [-literal for literal in expression_clause(clause, predicates)]
            if on.solve(assumptions)[0] == 20:
                implicates.append(clause)
    finally:
        on.close()
    implication_seconds = time.monotonic() - implication_start

    off = SAT(cnf, variables)
    off.add([-output_literal])
    selector_first = variables + 1
    selector_to_clause = {}
    for index, clause in enumerate(implicates):
        selector = selector_first + index
        selector_to_clause[selector] = clause
        off.add([-selector] + expression_clause(clause, predicates))
    exact = False
    reduced = []
    minimize_calls = 0
    equivalence_start = time.monotonic()
    try:
        selectors = list(selector_to_clause)
        rc, core = off.solve(selectors)
        exact = rc == 20
        if exact:
            selected = set(selectors)
            reduced = [literal for literal in core if literal in selected]
            if minimize:
                for candidate in list(reduced):
                    if candidate not in reduced:
                        continue
                    trial = [x for x in reduced if x != candidate]
                    rc, smaller = off.solve(trial)
                    minimize_calls += 1
                    if rc == 20:
                        trial_set = set(trial)
                        reduced = [x for x in smaller if x in trial_set]
            if off.solve(reduced)[0] != 20:
                raise RuntimeError("Reduced learned CNF failed its equivalence proof")
    finally:
        off.close()
    equivalence_seconds = time.monotonic() - equivalence_start

    region = [selector_to_clause[selector] for selector in reduced] if exact else []
    union = count_union(prefix, ex.n, predicates, [region]) if exact else None
    artifact = {
        "nvars": ex.n,
        "predicates": [
            {"expression_atom": atom, "group": group, "mask": mask}
            for atom, group, mask in predicates
        ],
        "regions": [region] if exact else [],
    }
    prefix.with_suffix(".regions.json").write_text(json.dumps(artifact))
    result = {
        "fault": fault,
        "stuck": stuck,
        "width": width,
        "predicates": len(predicates),
        "candidate_clauses": len(candidates),
        "entailed_clauses": len(implicates),
        "exact": exact,
        "learned_clauses": len(region),
        "implication_sat_calls": on.calls,
        "equivalence_sat_calls": off.calls,
        "minimize_calls": minimize_calls,
        "implication_seconds": implication_seconds,
        "equivalence_seconds": equivalence_seconds,
        "union": union,
        "total_seconds": time.monotonic() - total_start,
    }
    metadata = {
        "net": str(netpath),
        "fault": fault,
        "stuck": stuck,
        "ncoords": ex.n,
        "basis_rows": [hex(row) for row in rows],
        "pi_order": net.pi,
    }
    prefix.with_suffix(".metadata.json").write_text(json.dumps(metadata, indent=2))
    prefix.with_suffix(".result.json").write_text(json.dumps(result, indent=2))
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--net", required=True)
    parser.add_argument("--fault", required=True)
    parser.add_argument("--stuck", type=int, default=0)
    parser.add_argument("--prefix", required=True)
    parser.add_argument("--width", type=int, default=1)
    parser.add_argument("--no-minimize", action="store_true")
    args = parser.parse_args()
    print(json.dumps(learn(args.net, args.fault, args.stuck, args.prefix,
                           args.width, not args.no_minimize)))
