#!/usr/bin/env python3
"""Check shipped settings and batch execution in an isolated repository layout."""
import argparse
import csv
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def options(path):
    result = {}
    for line in path.read_text().splitlines():
        tokens = line.split('#', 1)[0].split()
        if tokens:
            result[tokens[0]] = tokens[1] if len(tokens) > 1 else None
    return result


def projection(path):
    with path.open() as stream:
        return sorted((r['net_name'], r['f_type'], r['fdp'])
                      for r in csv.DictReader(stream))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    args = parser.parse_args()
    binary = args.binary.resolve()
    configs = list((ROOT / 'input/script').rglob('*.set'))
    assert configs
    for config in configs:
        model = config.parent.name
        settings = options(config)
        assert model in ('saf', 'tdf'), config
        if model == 'tdf':
            assert '-tdf' in settings, config
        for key, kind in (('-net', 'circuit'), ('-fault', 'fault')):
            if key in settings:
                target = (ROOT / 'build' / settings[key]).resolve()
                assert target.is_file(), (config, target)
                assert target.parent == ROOT / 'input' / kind / model, (config, target)
        for key in ('-fdp', '-log'):
            target = (ROOT / 'build' / settings[key]).resolve()
            target.relative_to(ROOT / 'output' / model)

    with tempfile.TemporaryDirectory(prefix='fdp batch checks ') as temporary:
        root = Path(temporary)
        shutil.copytree(ROOT / 'scripts', root / 'scripts')
        (root / 'build').mkdir()
        (root / 'build/main_release').symlink_to(binary)
        (root / 'input').mkdir()
        for kind in ('circuit', 'fault'):
            (root / 'input' / kind).symlink_to(ROOT / 'input' / kind, target_is_directory=True)
        shutil.copytree(ROOT / 'input/script', root / 'input/script')
        env = dict(os.environ, NO_DISCORD='1')

        def run(script, *arguments, status=0):
            result = subprocess.run([str(root / 'scripts' / script), *arguments],
                                    cwd=root.parent, env=env, text=True,
                                    capture_output=True, timeout=60)
            assert result.returncode == status, (script, arguments, result.stdout, result.stderr)
            return result

        run('run.sh', '--help')
        run('run.sh', status=2)
        run('run.sh', 'unknown', 'c17a', status=2)
        run('run.sh', 'saf', '../tdf/s27_tdf', status=1)
        run('run.sh', 'saf', 'c17a.set')
        assert projection(root / 'output/saf/full/fdp/c17a.csv') == projection(
            ROOT / 'tests/expected/c17a_result.csv')
        config = root / 'input/script/saf/c17a.set'
        with config.open('a') as stream:
            stream.write('\n-cube ../output/saf/full/cube/c17a\n')
        run('run.sh', 'saf', 'c17a')
        assert list((root / 'output/saf/full/cube/c17a').glob('*_sa0_cube.txt'))
        run('run_experiments.sh', 'c17a_test')
        run('run_itc99.sh', 'c17a')
        run('run_tdf.sh', 's27', 's27_tdf_list.set', 's208_tdf_list')
        for name in ('s27_tdf', 's27_tdf_list', 's208_tdf_list'):
            assert (root / f'output/tdf/full/fdp/{name}.csv').stat().st_size > 0
        run('run.sh', 'saf', 'missing', 'c17a', status=1)
        bad = root / 'input/script/saf/bad.set'
        bad.write_text('-net ../input/circuit/saf/missing.v\n'
                       '-fdp ../output/saf/full/fdp/bad.csv\n'
                       '-log ../output/saf/full/log/bad.txt\n')
        (root / 'output/saf/full/fdp/c17a.csv').unlink()
        run('run.sh', 'saf', 'bad', 'c17a', status=1)
        assert (root / 'output/saf/full/fdp/c17a.csv').is_file()

        # Validate default batch dispatch without executing large benchmark circuits.
        (root / 'build/main_release').unlink()
        fake = root / 'build/main_release'
        fake.write_text('#!/usr/bin/env python3\nimport json, sys\n'
                        'with open("calls.jsonl", "a") as f:\n'
                        '    f.write(json.dumps(sys.argv[1:]) + "\\n")\n')
        fake.chmod(0o755)
        defaults = {
            'run_b15.sh': ('saf', ['b15']),
            'run_itc99.sh': ('saf', ['b14', 'b17']),
            'run_experiments.sh': ('saf', ['s5378_C', 's9234_C', 's13207_C',
                                        's15850_C', 's35932_C', 's38417_C', 's38584_C']),
            'run_tdf.sh': ('tdf', ['s9234_tdf', 's35932_tdf', 's38417_tdf', 'b14_tdf', 'b17_tdf']),
        }
        calls = root / 'build/calls.jsonl'
        for script, (model, names) in defaults.items():
            calls.unlink(missing_ok=True)
            run(script)
            assert [json.loads(line) for line in calls.read_text().splitlines()] == [
                ['-set', f'../input/script/{model}/{name}.set'] for name in names]
    print(f'{len(configs)} settings, golden FDP, SAF/TDF batch execution, defaults and failure handling: PASS')


if __name__ == '__main__':
    main()
