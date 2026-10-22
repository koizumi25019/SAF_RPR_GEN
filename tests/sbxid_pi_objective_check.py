"""CLI/.set PI-objective selection and complete FDP with native clasp."""
import argparse
import csv
from decimal import Decimal
import os
from pathlib import Path
import re
import subprocess


def check(args):
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, FDP_NORMAL_SCOPE_VALIDATE="1", FDP_SBXID_VERIFY="1")
    for binary in (args.binary.resolve(), args.debug.resolve()):
        for mode in (None, "bad"):
            options = ["-sbxid_pi_objective"] + ([] if mode is None else [mode])
            result = subprocess.run([str(binary), *options], capture_output=True,
                                    text=True, env=env, timeout=10)
            assert result.returncode != 0 and "requires all|scope" in result.stderr
        for circuit in ("c17a", "s27_C"):
            reference = None
            for mode, jobs in (("all", 1), ("scope", 1), ("scope", 2)):
                label = f"{binary.name}_{circuit}_{mode}_{jobs}"
                csv_path, log_path = output / f"{label}.csv", output / f"{label}.log"
                # Exercise .set followed by a CLI override, including the
                # default scope mode when no explicit option is present.
                config = output / f"{label}.set"
                config.write_text(
                    f"-net input/circuit/saf/{circuit}.v\n"
                    f"-fdp {csv_path}\n-log {log_path}\n-limit 0\n"
                    f"-jobs {jobs}\n-dom_reuse off\n"
                    + ("-sbxid_pi_objective all\n" if mode == "all" else ""))
                options = ["-set", str(config)]
                if jobs == 2:
                    with config.open("a") as stream:
                        stream.write("-sbxid_pi_objective all\n")
                    options += ["-sbxid_pi_objective", "scope"]
                with (output / f"{label}.stdout").open("w") as stdout, \
                        (output / f"{label}.stderr").open("w") as stderr:
                    subprocess.run([str(binary), *options], cwd=root, env=env,
                                   stdout=stdout, stderr=stderr, check=True, timeout=30)
                log = log_path.read_text()
                assert re.search(r"SB-XID PI Objective\s*:\s*" + mode, log), log
                assert re.search(r"SB-XID Verified / Failed\s*:\s*\d+ / 0", log), log
                with csv_path.open() as stream:
                    rows = list(csv.DictReader(stream))
                assert rows and all(r["complete"] != "0" for r in rows)
                signature = sorted((r["net_name"], r["f_type"], Decimal(r["fdp"])) for r in rows)
                if reference is None:
                    reference = signature
                else:
                    assert signature == reference, label
                if circuit == "c17a":
                    with (root / "expected/c17a_result.csv").open() as stream:
                        expected = list(csv.DictReader(stream))
                    assert signature == sorted((r["net_name"], r["f_type"], Decimal(r["fdp"]))
                                               for r in expected)
    print("PI objective CLI/.set selection and complete FDP passed in Debug/Release, jobs 1/2")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--debug", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    check(parser.parse_args())
