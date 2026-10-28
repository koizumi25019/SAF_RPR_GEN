"""Runtime XID/SB-XID selection, settings, parallelism and complete FDP."""
import argparse
import csv
from decimal import Decimal
import json
import os
from pathlib import Path
import re
import subprocess


def signature(path):
    with path.open() as stream:
        rows = list(csv.DictReader(stream))
    assert rows and all(r["complete"] != "0" for r in rows), path
    # Include cube counts so XID must match the actual baseline algorithm,
    # rather than simply reporting a different method name for SB-XID.
    return sorted((r["net_name"], r["f_type"], r["cube_cnt"],
                   r["complete"], Decimal(r["fdp"])) for r in rows)


def check(args):
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, FDP_NORMAL_SCOPE_VALIDATE="1", FDP_SBXID_VERIFY="1")
    reports = []

    def run(binary, label, circuit, method, jobs=1, reuse="off", extra=()):
        result, log = output / f"{label}.csv", output / f"{label}.log"
        config = output / f"{label}.set"
        config.write_text(
            f"-saf\n-net input/circuit/saf/{circuit}.v\n"
            f"-fdp {result}\n-log {log}\n-limit 0\n"
            f"-jobs {jobs}\n-dom_reuse {reuse}\n"
            + (f"-dc_method {method}\n" if method else ""))
        with (output / f"{label}.stdout").open("w") as stdout, \
                (output / f"{label}.stderr").open("w") as stderr:
            subprocess.run([str(binary), "-set", str(config), *extra], cwd=root,
                           env=env, stdout=stdout, stderr=stderr, check=True, timeout=30)
        return signature(result), log.read_text()

    for binary in (args.binary.resolve(), args.debug.resolve()):
        for options in (("-dc_method",), ("-dc_method", "bad")):
            result = subprocess.run([str(binary), *options], env=env,
                                    capture_output=True, text=True, timeout=10)
            assert result.returncode != 0 and "requires xid|sbxid" in result.stderr
        for text in ("-dc_method\n", "-dc_method bad\n"):
            invalid = output / "invalid.set"
            invalid.write_text(text)
            result = subprocess.run([str(binary), "-set", str(invalid)], env=env,
                                    capture_output=True, text=True, timeout=10)
            assert result.returncode != 0 and "requires xid|sbxid" in result.stderr

        for circuit in ("c17a", "s27_C"):
            complete_fdp = None
            for method in ("xid", "sbxid"):
                serial = None
                for jobs, reuse in ((1, "off"), (2, "off"), (1, "on")):
                    label = f"{binary.name}_{circuit}_{method}_{jobs}_{reuse}"
                    actual, log = run(binary, label, circuit, method, jobs, reuse)
                    expected_method = "XID (circuit-structure based)" if method == "xid" else "SB-XID v1 (no CEGAR)"
                    assert re.search(r"Don't-care Method\s*:\s*" + re.escape(expected_method), log)
                    assert re.search(r"CPU Time \(Don't care\)\s*:\s*[\d.]+ sec", log)
                    assert re.search(r"Total Cubes / Reused Cubes\s*:\s*\d+ / \d+", log)
                    if method == "xid":
                        assert "Clasp" not in log and "SB-XID PI Objective" not in log
                    else:
                        assert re.search(r"Clasp Search Limit\s*:\s*unlimited \(until optimal\)", log)
                        assert re.search(r"Clasp Optimal / Feasible\s*:\s*[1-9]\d* / 0", log)
                        assert re.search(r"Clasp Timeouts / UNKNOWN Fallbacks\s*:\s*0 / 0", log)
                        assert re.search(r"SB-XID Verified / Failed\s*:\s*\d+ / 0", log)
                    fdp = [(name, kind, density) for name, kind, _, _, density in actual]
                    if complete_fdp is None:
                        complete_fdp = fdp
                    assert fdp == complete_fdp, label
                    if reuse == "off":
                        if serial is None:
                            serial = actual
                        assert actual == serial, label
                    reports.append(dict(binary=binary.name, circuit=circuit,
                                        method=method, jobs=jobs, reuse=reuse,
                                        rows=len(actual), complete_fdp_equal=True))
                reference = os.environ.get("FDP_TEST_XID_REFERENCE" if method == "xid" else "FDP_TEST_SBXID_REFERENCE")
                if reference:
                    original, _ = run(Path(reference).resolve(), f"reference_{binary.name}_{circuit}_{method}",
                                      circuit, None)
                    assert original == serial, (circuit, method, "reference mismatch")
            if circuit == "c17a":
                golden = signature(root / "expected/c17a_result.csv")
                assert complete_fdp == [(name, kind, density) for name, kind, _, _, density in golden]

            default, log = run(binary, f"default_{binary.name}_{circuit}", circuit, None)
            explicit, _ = run(binary, f"explicit_{binary.name}_{circuit}", circuit, "sbxid")
            override, log_override = run(binary, f"override_{binary.name}_{circuit}", circuit, "xid",
                                         extra=("-dc_method", "sbxid"))
            assert default == explicit == override
            assert "SB-XID v1" in log and "SB-XID v1" in log_override

    report = dict(runs=reports, invalid_cli_and_set_rejected=True,
                  default_sbxid_preserved=True, cli_override_works=True,
                  xid_reference_checked=bool(os.environ.get("FDP_TEST_XID_REFERENCE")),
                  sbxid_reference_checked=bool(os.environ.get("FDP_TEST_SBXID_REFERENCE")))
    (output / "summary.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"XID/SB-XID switching passed: {len(reports)} complete-FDP cases, settings and defaults")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--debug", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    check(parser.parse_args())
