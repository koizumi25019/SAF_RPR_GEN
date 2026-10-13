"""Validate affected-output TFI and compare EA with the same structural scope.

Run from any directory. Outputs stay under runs/, separate from normal results.
Benchmark runs are sequential, one per condition, with validation hooks disabled.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
HOOKS = {"FDP_NORMAL_SCOPE", "PROFILE_CUBE_DIR"}
for source in (ROOT / "src").rglob("*.c"):
    HOOKS.update(re.findall(r'getenv\("([A-Z0-9_]+)"\)', source.read_text()))
BASE_ENV = {key: value for key, value in os.environ.items() if key not in HOOKS}


def run(label, circuit="c17a", ea="on", jobs=2, limit=0, method="xid",
        gt=True, tdf=False, power=None, fault=None, reference=False,
        binary=None, environment=None, cli=(), reuse="off"):
    directory = HERE / "runs" / label
    directory.mkdir(parents=True, exist_ok=True)
    (directory / "time.log").unlink(missing_ok=True)
    net = Path(circuit) if str(circuit).endswith(".v") else ROOT / f"input/circuit/{circuit}.v"
    directives = [f"-net {net}", f"-fdp {directory}/fdp.csv",
                  f"-log {directory}/time.log", f"-jobs {jobs}",
                  f"-limit {limit}", f"-dc_method {method}", f"-dom_reuse {reuse}"]
    if not reference:
        directives.append(f"-ea {ea}")
    if method == "core":
        directives.append("-core_verify " + ("on" if gt else "off"))
    if tdf:
        directives.append("-tdf")
    if power is not None:
        directives += ["-low_power on", f"-wsa_threshold {power}"]
    if fault:
        path = directory / "faults.txt"
        path.write_text("".join(f"{name}\t{kind}\n" for name, kind in fault))
        directives.append(f"-fault {path}")
    config = directory / "run.set"
    config.write_text("\n".join(directives) + "\n")
    env = dict(BASE_ENV, **(environment or {}))
    if reference:
        env["FDP_NORMAL_SCOPE"] = "1"
        if ea == "off":
            env["MDC_NOEA"] = "1"
    if gt:
        env.update(GT_BDD="1", FDP_NORMAL_SCOPE_VALIDATE="1")
    executable = binary or (HERE / "build/main_reference" if reference else ROOT / "build/main_release")
    start = time.monotonic()
    with (directory / "stdout.txt").open("w") as out, (directory / "stderr.txt").open("w") as err:
        subprocess.run([str(executable), "-set", str(config), *cli], cwd=ROOT / "build",
                       env=env, stdout=out, stderr=err, check=True, timeout=900)
    elapsed = time.monotonic() - start
    stderr = (directory / "stderr.txt").read_text()
    rows = list(csv.DictReader((directory / "fdp.csv").open()))
    representatives = [row for row in rows if row["complete"] in ("0", "1")]
    if gt:
        summaries = re.findall(r"\[GT\] summary: checked=(\d+)\s+UNSOUND=(\d+)\s+"
                               r"complete-but-NOT-exact=(\d+)\s+ALL VERIFIED", stderr)
        assert summaries and tuple(map(int, summaries[-1])) == (len(representatives), 0, 0), stderr[-3000:]
    log = (directory / "time.log").read_text()
    if not reference:
        assert "on (affected-output TFI)" in log
        expected_ea = cli[-1] if "-ea" in cli else ea
        assert re.search(r"Essential Assignment\s+: " + expected_ea, log), label
    scopes = re.findall(r"faults=(\d+) normal_gates=(\d+)/(\d+) outside_pi=(\d+)", stderr)
    scope_counts = [sum(int(row[i]) for row in scopes) for i in range(4)]
    record = dict(label=label, circuit=net.stem, ea=ea, jobs=jobs, limit=limit,
                  external_wall_seconds=elapsed, time_seconds=float(re.search(r"//  Time\s+: ([\d.]+)", log)[1]),
                  cpu_seconds=float(re.search(r"CPU Time\s+: ([\d.]+)", log)[1]),
                  cadical_cpu_seconds=float(re.search(r"CPU Time \(CaDiCaL\)\s+: ([\d.]+)", log)[1]),
                  bdd_cpu_seconds=float(re.search(r"CPU Time \(BDD\)\s+: ([\d.]+)", log)[1]),
                  xid_cpu_seconds=float(re.search(r"CPU Time \(Don't care\)\s+: ([\d.]+)", log)[1]),
                  faults=len(representatives), completed=sum(row["complete"] == "1" for row in representatives),
                  cubes=sum(int(row["cube_cnt"]) for row in representatives),
                  normal_definitions_kept=scope_counts[1], normal_definitions_full=scope_counts[2],
                  outside_pi_total=scope_counts[3], gt_verified=gt)
    return record, rows


def signature(rows):
    return sorted((row["net_name"], row["f_type"], row["fdp"], row["complete"]) for row in rows)


def smoke(record):
    for name in ("c17a", "s27_C", "s208_C", "s298_C", "s344_C", "s510_C", "s641_C", "s713_C", "s1494_C"):
        on, a = run(name + "_ea_on", circuit=name)
        off, b = run(name + "_ea_off", circuit=name, ea="off")
        _, reference = run(name + "_reference", circuit=name, reference=True)
        assert signature(a) == signature(b) == signature(reference), name
        assert on["normal_definitions_kept"] == off["normal_definitions_kept"]
        assert all(row["complete"] == "1" for row in a if row["complete"] in ("0", "1"))
        if name == "c17a":
            golden = list(csv.DictReader((ROOT / "expected/c17a_result.csv").open()))
            assert sorted((r["net_name"], r["f_type"], r["fdp"]) for r in a) == sorted((r["net_name"], r["f_type"], r["fdp"]) for r in golden)
        record(dict(check=name, gt="ALL VERIFIED", full_FDP_equal=True, scope_equal_with_EA_on_off=True))
    for fault, expected in (("i_k_B", 9), ("i_l_A", 8)):
        for ea in ("on", "off"):
            data, _ = run("branch_" + fault + "_" + ea, fault=[(fault, "sa0")], ea=ea)
            assert data["normal_definitions_kept"] == expected, data
        record(dict(check=fault, retained_definitions=expected, scope_equal_with_EA_on_off=True))
    for name, method, power in (("c17a", "core", None), ("s208_C", "core", None),
                              ("s27", "xid", None), ("s208", "xid", None),
                              ("s27", "core", None), ("s208", "core", 20)):
        tdf = not name.endswith("_C") and name != "c17a"
        label = f"compat_{name}_{method}_{power}"
        _, a = run(label + "_on", circuit=name, method=method, power=power, tdf=tdf)
        _, b = run(label + "_off", circuit=name, method=method, power=power, tdf=tdf, ea="off")
        _, reference = run(label + "_reference", circuit=name, method=method, power=power, tdf=tdf, reference=True)
        assert signature(a) == signature(b) == signature(reference), label
        record(dict(check=label, gt="ALL VERIFIED", full_FDP_equal=True))
    # A fanout may terminate internally without being an observable PO.
    fixture = HERE / "runs/dangling.v"
    fixture.write_text("""module dangling(a,b,c,d,y,z);
input a,b,c,d;
output y,z;
wire a,b,c,d,f,dead,y,z;
BUF G0(.A(a),.Z(f));
AND2 G1(.A(f),.B(b),.Z(y));
BUF G2(.A(f),.Z(dead));
AND2 G3(.A(c),.B(d),.Z(z));
endmodule
""")
    # Independent enumeration of all 16 PI vectors, observing only y and z.
    detection_count = sum((a and b) for a in (0, 1) for b in (0, 1)
                          for c in (0, 1) for d in (0, 1))
    for ea in ("on", "off"):
        _, rows = run("dangling_" + ea, circuit=fixture, ea=ea,
                      fault=[("f", "sa0"), ("dead", "sa0"), ("dead", "sa1")])
        measured = {(r["net_name"], r["f_type"]): float(r["fdp"]) for r in rows}
        assert measured["f", "sa0"] == detection_count / 16
        assert measured["dead", "sa0"] == measured["dead", "sa1"] == 0
    record(dict(check="dangling_fanout", independent_full_PI_enumeration=True))
    for ea in ("on", "off"):
        run("debug_" + ea, ea=ea, jobs=1, binary=ROOT / "build/main_debug")
    # Explicit settings override the legacy environment; CLI overrides .set.
    run("ea_set_precedence", environment={"MDC_NOEA": "1"})
    run("ea_cli_precedence", cli=("-ea", "off"))
    config = HERE / "runs/ea_cli_precedence/run.set"
    for value in ("maybe", "", "2"):
        args = [str(ROOT / "build/main_release"), "-set", str(config), "-ea"]
        if value:
            args.append(value)
        result = subprocess.run(args, env=BASE_ENV, capture_output=True, text=True)
        assert result.returncode and "-ea requires" in result.stderr
    record(dict(check="settings_debug_release", passed=True))


def large(record, circuits):
    for circuit in circuits:
        label = circuit.removesuffix("_C")
        on, a = run(label + "_gt_on", circuit=circuit, limit=30)
        off, b = run(label + "_gt_off", circuit=circuit, limit=30, ea="off")
        assert on["faults"] == off["faults"]
        assert on["normal_definitions_kept"] == off["normal_definitions_kept"]
        record(dict(check=label + "_all", gt="ALL VERIFIED", faults=on["faults"], ea_on=on, ea_off=off))


def benchmark(record, circuits, jobs):
    for circuit in circuits:
        outputs, measurements = {}, []
        for ea in ("on", "off"):
            measurement, rows = run(circuit + "_timing_" + ea, circuit=circuit,
                                    limit=30, jobs=jobs, ea=ea, gt=False)
            outputs[ea] = {(r["net_name"], r["f_type"]): r for r in rows if r["complete"] in ("0", "1")}
            measurements.append(measurement)
            print(json.dumps(measurement), flush=True)
        assert outputs["on"].keys() == outputs["off"].keys()
        shared = [key for key in outputs["on"] if outputs["on"][key]["complete"] == outputs["off"][key]["complete"] == "1"]
        assert all(outputs["on"][key]["fdp"] == outputs["off"][key]["fdp"] for key in shared)
        assert measurements[0]["normal_definitions_kept"] == measurements[1]["normal_definitions_kept"]
        record(dict(circuit=circuit, measurements=measurements, shared_completed_faults=len(shared),
                    shared_complete_FDP_mismatches=0, scope_equal_with_EA_on_off=True))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("smoke", "large", "benchmark"), default="smoke")
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--circuits", nargs="+")
    args = parser.parse_args()
    report = dict(reference_commit="576ee07", measurements_per_condition=1,
                  scope="TFI of affected observable outputs; EA units only inside that scope",
                  binary_sha256=hashlib.sha256((ROOT / "build/main_release").read_bytes()).hexdigest(),
                  cpu_quota=Path("/sys/fs/cgroup/cpu.max").read_text().strip() if Path("/sys/fs/cgroup/cpu.max").exists() else None,
                  checks=[])
    result_name = args.phase
    if args.phase == "large" and args.circuits:
        result_name += "_" + "_".join(args.circuits)
    result_path = HERE / (result_name + ".json")
    def record(item):
        report["checks"].append(item)
        result_path.write_text(json.dumps(report, indent=2) + "\n")
        print(json.dumps(item), flush=True)
    if args.phase == "smoke":
        smoke(record)
    elif args.phase == "large":
        large(record, args.circuits or ("s5378_C",))
    else:
        benchmark(record, args.circuits or ("s5378_C", "s9234_C"), args.jobs)
    report["passed"] = True
    result_path.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS " + args.phase, flush=True)


if __name__ == "__main__":
    main()
