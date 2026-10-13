"""Build current Debug/Release and the pre-change verification reference."""
import importlib.util
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
REFERENCE_COMMIT = "576ee07"


def main():
    specification = importlib.util.spec_from_file_location(
        "fault_pool_builder", ROOT / "verification/fault_pool/build.py")
    builder = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(builder)
    reference = HERE / "build/reference"
    reference.mkdir(parents=True, exist_ok=True)
    archive = subprocess.check_output(
        ["git", "archive", REFERENCE_COMMIT, "src", "CMakeLists.txt"], cwd=ROOT)
    subprocess.run(["tar", "-x", "-C", str(reference)], input=archive, check=True)
    builder.compile_program(ROOT, ROOT / "build/main_release")
    builder.compile_program(ROOT, ROOT / "build/main_debug", True)
    builder.compile_program(reference, HERE / "build/main_reference")


if __name__ == "__main__":
    main()
