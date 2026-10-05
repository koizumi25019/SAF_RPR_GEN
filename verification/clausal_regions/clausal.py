"""SAT/DC enumeration whose regions are CNFs over bounded local predicates.

The ordinary PI cube language is the special case in which every clause is a
unit clause.  Here a model is generalized by an UNSAT core of candidate
clauses, so one reported region may contain alternatives such as
``(x1 or x2) and (x3 or x4)`` without distributing them into four cubes.
Only the emitted region union is sent to CUDD.
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
ROOT = HERE.parent.parent
sys.path.insert(0, str(HERE.parent / "predicate_sop"))
sys.path.insert(0, str(HERE.parent / "factored_sop"))
from fast_basis import FastNet  # noqa: E402
from predicates import prepare, select  # noqa: E402
from probe import SAT  # noqa: E402


def write_cnf(path: pathlib.Path, clauses: list[list[int]], variables: int) -> None:
    path.write_text(
        f"p cnf {variables} {len(clauses)}\n"
        + "".join(" ".join(map(str, clause)) + " 0\n" for clause in clauses)
    )


def clauses_up_to_two(count: int) -> list[tuple[int, ...]]:
    result = [(sign * atom,) for atom in range(1, count + 1) for sign in (1, -1)]
    for left in range(1, count + 1):
        for right in range(left + 1, count + 1):
            for left_sign in (1, -1):
                for right_sign in (1, -1):
                    result.append((left_sign * left, right_sign * right))
    return result


def expression_clause(clause: tuple[int, ...], predicates) -> list[int]:
    return [
        predicates[abs(literal) - 1][0] * (1 if literal > 0 else -1)
        for literal in clause
    ]


def clause_is_true(clause: tuple[int, ...], model: list[int], predicates) -> bool:
    for literal in clause:
        atom = predicates[abs(literal) - 1][0]
        value = model[atom - 1] > 0
        if value == (literal > 0):
            return True
    return False


def add_region_blocker(det: SAT, region, predicates, fresh: int) -> int:
    """Add NOT(AND clauses) to the detecting solver with Tseitin variables."""
    violations = []
    for clause in region:
        fresh += 1
        violation = fresh
        literals = expression_clause(clause, predicates)
        # violation <=> every literal of this clause is false.
        for literal in literals:
            det.add([-violation, -literal])
        det.add([violation] + literals)
        violations.append(violation)
    # An empty conjunction is true and therefore blocks the whole space.
    det.add(violations)
    return fresh


def count_union(prefix: pathlib.Path, nvars: int, predicates, regions, timeout=60):
    lines = [f"{nvars} {len(predicates)} {len(regions)}"]
    for _, group, mask in predicates:
        lines.append(f"{len(group)} {' '.join(map(str, group))} {mask}")
    for region in regions:
        lines.append(str(len(region)))
        for clause in region:
            lines.append(f"{len(clause)} {' '.join(map(str, clause))}")
    run = subprocess.run(
        [str(HERE / "build" / "formula_union")],
        input="\n".join(lines) + "\n",
        text=True,
        capture_output=True,
        check=True,
        timeout=timeout,
    )
    result = json.loads(run.stdout)
    prefix.with_suffix(".union.json").write_text(json.dumps(result, indent=2))
    return result


def enumerate_regions(
    ex,
    output: int,
    predicates,
    prefix: pathlib.Path,
    seconds: float,
    limit: int,
    minimize: bool,
):
    clauses, variables, output_literal = ex.cnf(output)
    # Reserve blocker variables before the first solve. CaDiCaL may otherwise
    # allocate the same external IDs for internal extension variables.
    blocker_reserve = max(100000, limit * (len(predicates) + 1))
    declared_variables = variables + blocker_reserve
    cnf = prefix.with_suffix(".cnf")
    write_cnf(cnf, clauses, declared_variables)
    det = SAT(cnf, variables)
    off = SAT(cnf, variables)
    det.add([output_literal])

    candidates = clauses_up_to_two(len(predicates))
    selector_first = declared_variables + 1
    for index, clause in enumerate(candidates):
        selector = selector_first + index
        off.add([-selector] + expression_clause(clause, predicates))
    selector_to_candidate = {
        selector_first + index: clause for index, clause in enumerate(candidates)
    }
    candidate_to_selector = {
        clause: selector for selector, clause in selector_to_candidate.items()
    }

    start = time.monotonic()
    fresh = variables
    regions = []
    complete = False
    timed_out = False
    core_sizes = []
    minimize_calls = 0
    try:
        while len(regions) < limit:
            if time.monotonic() - start >= seconds:
                timed_out = True
                break
            rc, model = det.solve()
            if rc == 20:
                complete = True
                break
            active = [
                selector_first + index
                for index, clause in enumerate(candidates)
                if clause_is_true(clause, model, predicates)
            ]
            rc, core = off.solve([-output_literal] + active)
            if rc != 20:
                raise RuntimeError(
                    "Predicate frontier does not determine detection for this model"
                )
            active_set = set(active)
            core = [literal for literal in core if literal in active_set]
            if not core:
                # NOT D itself is UNSAT, so D is a tautology.
                region = []
            else:
                if minimize:
                    initial = list(core)
                    for candidate in initial:
                        if time.monotonic() - start >= seconds:
                            timed_out = True
                            break
                        if candidate not in core:
                            continue
                        trial = [x for x in core if x != candidate]
                        rc, reduced = off.solve([-output_literal] + trial)
                        minimize_calls += 1
                        if rc == 20:
                            reduced_set = set(trial)
                            core = [x for x in reduced if x in reduced_set]
                    if timed_out:
                        break
                region = [selector_to_candidate[selector] for selector in core]
            # Certify the exact region used after core/minimization.
            assumptions = [-output_literal] + [
                candidate_to_selector[clause] for clause in region
            ]
            rc, _ = off.solve(assumptions)
            if rc != 20:
                raise RuntimeError("Generated clausal region is unsound")
            regions.append(region)
            core_sizes.append(len(region))
            fresh = add_region_blocker(det, region, predicates, fresh)
        if not complete and not timed_out:
            complete = det.solve()[0] == 20
    finally:
        det.close()
        off.close()

    elapsed = time.monotonic() - start
    result = {
        "regions": len(regions),
        "complete": complete,
        "timed_out": timed_out,
        "seconds": elapsed,
        "predicates": len(predicates),
        "candidate_clauses": len(candidates),
        "detector_calls": det.calls,
        "oracle_calls": off.calls,
        "minimize_calls": minimize_calls,
        "clause_count": sum(core_sizes),
        "clause_hist": dict(sorted(Counter(core_sizes).items())),
    }
    prefix.with_suffix(".regions.json").write_text(json.dumps({
        "nvars": ex.n,
        "predicates": [
            {"expression_atom": atom, "group": group, "mask": mask}
            for atom, group, mask in predicates
        ],
        "regions": regions,
    }))
    prefix.with_suffix(".result.json").write_text(json.dumps(result, indent=2))
    return result, regions


def run(
    netpath,
    fault,
    stuck,
    prefix,
    width=1,
    seconds=10,
    limit=10000,
    minimize=True,
):
    prefix = pathlib.Path(prefix)
    prefix.parent.mkdir(parents=True, exist_ok=True)
    total_start = time.monotonic()
    net = FastNet(netpath)
    ex, output, rows = prepare(net, fault, stuck)
    predicates = select(ex, output, width, frontier=True)
    if len(predicates) < 2:
        raise ValueError("The binary-clause prototype needs at least two frontier predicates")
    result, regions = enumerate_regions(
        ex, output, predicates, prefix, seconds, limit, minimize
    )
    union = count_union(prefix, ex.n, predicates, regions) if result["complete"] else None
    metadata = {
        "net": str(netpath),
        "fault": fault,
        "stuck": stuck,
        "width": width,
        "ncoords": ex.n,
        "basis_rows": [hex(row) for row in rows],
        "pi_order": net.pi,
        "total_seconds": time.monotonic() - total_start,
    }
    prefix.with_suffix(".metadata.json").write_text(json.dumps(metadata, indent=2))
    combined = {**result, "union": union, "total_seconds": metadata["total_seconds"]}
    prefix.with_suffix(".combined.json").write_text(json.dumps(combined, indent=2))
    return combined


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--net", required=True)
    parser.add_argument("--fault", required=True)
    parser.add_argument("--stuck", type=int, default=0)
    parser.add_argument("--prefix", required=True)
    parser.add_argument("--width", type=int, default=1)
    parser.add_argument("--seconds", type=float, default=10)
    parser.add_argument("--limit", type=int, default=10000)
    parser.add_argument("--core-only", action="store_true")
    args = parser.parse_args()
    print(json.dumps(run(args.net, args.fault, args.stuck, args.prefix,
                         args.width, args.seconds, args.limit,
                         not args.core_only)))
