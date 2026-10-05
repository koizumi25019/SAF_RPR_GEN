"""Reproduce the earlier prototype on the fixed matched b19 subset."""
import pathlib
import sys

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent/'linear_scaling'))
from batch import batch


if __name__ == '__main__':
    batch(HERE.parents[1]/'input/circuit/b19_C.v', HERE/'runs/reproduce_b19_original16',
          fault_file=HERE/'cases/b19_C_16.faults', simulate=True, width=3, seconds=2)
