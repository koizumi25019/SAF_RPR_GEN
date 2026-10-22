"""Compare all-PI and normal-scope PI objectives on identical reference seeds.

Build sbxid_pi_objective_benchmark explicitly first. Each native invocation uses
one clasp solve thread, a one-second search limit, and post-solve verification.
"""
import argparse
import csv
import json
from pathlib import Path
import statistics
import subprocess


def compare(args):
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    reference = root / "SB-XID/tp/1f1p"
    faults = reference / "s5378_C_1f1p_fault.txt"
    patterns = reference / "s5378_C_1f1p_tp.txt"
    rows = len(faults.read_text().splitlines())
    assert rows == len(patterns.read_text().splitlines())
    assert 2 <= args.samples <= rows and args.trials >= 1
    reports = []
    for trial in range(1, args.trials + 1):
        order = ("all", "scope") if trial % 2 else ("scope", "all")
        data = {}
        for mode in order:
            prefix = output / f"{mode}_{trial}"
            command = [str(args.binary.resolve()),
                       str(root / "input/circuit/saf/s5378_C.v"),
                       str(reference / "pin_s5378_C.txt"), str(faults),
                       str(patterns), mode, str(args.samples), str(rows),
                       str(prefix.with_suffix(".csv"))]
            with prefix.with_suffix(".stdout").open("w") as stdout, \
                    prefix.with_suffix(".stderr").open("w") as stderr:
                subprocess.run(command, cwd=root, stdout=stdout, stderr=stderr,
                               check=True, timeout=args.samples * 2 + 60)
            with prefix.with_suffix(".csv").open() as stream:
                data[mode] = list(csv.DictReader(stream))
            assert len(data[mode]) == args.samples
            print(f"trial {trial} {mode}: "
                  f"{sum(float(r['total_seconds']) for r in data[mode]):.3f}s", flush=True)
        a, b = data["all"], data["scope"]
        quality = dict(scope_better=0, equal=0, all_better=0,
                       both_optimal=0, both_optimal_care_equal=0, cube_equal=0)
        for x, y in zip(a, b):
            for key in ("index", "name", "stuck", "n_pi", "scope_pis",
                        "variables", "constraints"):
                assert x[key] == y[key], (key, x, y)
            cx, cy = int(x["care"]), int(y["care"])
            quality["scope_better" if cy < cx else "all_better" if cx < cy else "equal"] += 1
            quality["cube_equal"] += x["cube"] == y["cube"]
            if x["status"] == y["status"] == "1":
                quality["both_optimal"] += 1
                quality["both_optimal_care_equal"] += cx == cy
                assert cx == cy, (x, y)
        summary = {}
        for mode in ("all", "scope"):
            cases = data[mode]
            summary[mode] = {
                "search_seconds": sum(float(r["search_seconds"]) for r in cases),
                "total_seconds": sum(float(r["total_seconds"]) for r in cases),
                "optimal": sum(r["status"] == "1" for r in cases),
                "feasible": sum(r["status"] == "2" for r in cases),
                "unknown": sum(r["status"] == "0" for r in cases),
                "timeouts": sum(int(r["timed_out"]) for r in cases),
                "care_mean": statistics.mean(int(r["care"]) for r in cases),
                "care_median": statistics.median(int(r["care"]) for r in cases),
                "care_max": max(int(r["care"]) for r in cases),
            }
        reports.append(dict(trial=trial, order=order, **summary, quality=quality))
    result = dict(
        circuit="s5378_C", cases=args.samples, available_reference_cases=rows,
        sampling="floor(k*(rows-1)/(samples-1)), k=0..samples-1",
        clasp_search_limit_seconds=1.0, workers=1, clasp_threads=1,
        post_solve_verification=True, verification_failures=0,
        outside_scope_pi_terms_removed=True,
        variable_allocation_and_constraints_equal=True,
        n_pi=int(a[0]["n_pi"]),
        scope_pis_mean=statistics.mean(int(r["scope_pis"]) for r in a),
        scope_pis_median=statistics.median(int(r["scope_pis"]) for r in a),
        scope_pis_min=min(int(r["scope_pis"]) for r in a),
        scope_pis_max=max(int(r["scope_pis"]) for r in a),
        trials=reports,
    )
    result["median_total_seconds"] = {
        mode: statistics.median(r[mode]["total_seconds"] for r in reports)
        for mode in ("all", "scope")}
    result["median_search_seconds"] = {
        mode: statistics.median(r[mode]["search_seconds"] for r in reports)
        for mode in ("all", "scope")}
    (output / "summary.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--samples", type=int, default=4511)
    parser.add_argument("--trials", type=int, default=3)
    compare(parser.parse_args())
