"""Reject TDF from both CLI and .set before reading a netlist or writing FDP."""
import argparse
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True)
parser.add_argument('--debug', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
for binary in (args.binary, args.debug):
    for mode, method in ((mode, method) for mode in ('cli', 'set')
                         for method in ('xid', 'sbxid')):
        prefix = args.output / (binary.name + '_' + mode + '_' + method)
        output = prefix.with_suffix('.csv')
        output.unlink(missing_ok=True)
        # Deliberately missing netlist: rejection must precede circuit reading.
        options = ['-tdf', '-dc_method', method, '-net', str(prefix.with_suffix('.missing.v')),
                   '-fdp', str(output), '-log', str(prefix.with_suffix('.log'))]
        if mode == 'set':
            config = prefix.with_suffix('.set')
            config.write_text(options[0] + '\n' + '\n'.join(
                ' '.join(options[i:i + 2]) for i in range(1, len(options), 2)) + '\n')
            options = ['-set', str(config)]
        result = subprocess.run([str(binary), *options], capture_output=True,
                                text=True, timeout=10)
        assert result.returncode != 0, (binary, mode)
        assert 'TDF is disabled in this SB-XID build' in result.stderr, result
        assert not output.exists(), output
print('TDF rejected for both methods, CLI/.set, Debug and Release')
