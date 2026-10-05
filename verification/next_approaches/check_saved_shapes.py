"""Recheck the proposed cross-fault cache key from saved weighted runs.

This performs no SAT enumeration.  It rebuilds the already used expression
decomposition, compares the full canonical keys in memory, and records only
compact diagnostics under this verification directory.
"""

from __future__ import annotations

import hashlib
import json
from fractions import Fraction
from pathlib import Path
import sys


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
WEIGHTED = ROOT / "verification" / "weighted_decomposition"
sys.path.insert(0, str(WEIGHTED))

import decompose  # noqa: E402
import weighted  # noqa: E402


CASES = (
    ("g16349", 1),
    ("II17661", 0),
    ("g13329", 1),
    ("II15893", 0),
)


def canonical_weights(probabilities, mapping):
    result = [None] * len(probabilities)
    for variable, (label, phase) in mapping.items():
        probability = probabilities[variable - 1]
        result[label] = 1 - probability if phase else probability
    assert all(value is not None for value in result)
    return tuple(result)


def main():
    circuit = ROOT / "input" / "circuit" / "s38584_C.v"
    net = decompose.FastNet(circuit)
    keys = []
    weight_rows = []
    cases = []

    for fault, stuck in CASES:
        expression, output, *_ = decompose.prepare(net, fault, stuck)
        top, root, sources, stats = decompose.select(expression, output, 12)
        key, mapping = weighted.base.shape_key(top, root)

        prefix = WEIGHTED / "runs" / "comparison" / f"{fault}_{stuck}_weighted_0"
        manifest = json.loads(prefix.with_suffix(".cover.json").read_text())
        result = json.loads(prefix.with_suffix(".result.json").read_text())
        probabilities = tuple(Fraction(source["probability"]) for source in manifest["sources"])
        weights = canonical_weights(probabilities, mapping)

        keys.append(key)
        weight_rows.append(weights)
        cases.append(
            {
                "fault": fault,
                "stuck": stuck,
                "original_inputs": stats["original_inputs"],
                "top_inputs": top.n,
                "selected_nonlinear_cuts": stats["selected"],
                "top_regions": result["top_cubes"],
                "probability": result["probability"],
                "shape_sha256": hashlib.sha256(repr(key).encode()).hexdigest(),
                "shape_serialized_characters": len(repr(key)),
                "canonical_weight_sha256": hashlib.sha256(
                    repr(tuple(map(str, weights))).encode()
                ).hexdigest(),
            }
        )

    output = {
        "scope": "saved-result structural check; no SAT/DC enumeration or timing",
        "circuit": str(circuit.relative_to(ROOT)),
        "exact_shape_keys_equal": all(key == keys[0] for key in keys[1:]),
        "canonical_weight_rows_equal": all(row == weight_rows[0] for row in weight_rows[1:]),
        "canonical_weight_count": len(weight_rows[0]),
        "distinct_canonical_weights": sorted({str(value) for value in weight_rows[0]}),
        "cases": cases,
    }
    destination = HERE / "results" / "shape_check.json"
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(output, indent=2) + "\n")
    print(json.dumps(output, indent=2))


if __name__ == "__main__":
    main()
