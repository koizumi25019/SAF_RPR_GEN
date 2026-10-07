"""Build current Debug/Release and the exact pre-pool verification reference."""
import os
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
EXT = Path(os.environ.get('FDP_EXTERNAL_DIR', ROOT / 'external')).resolve()
REFERENCE_COMMIT = '476f8ba'


def compile_program(root, binary, debug=False):
    sources = re.search(r'set\(SOURCES\s+(.*?)\)',
                        (root / 'CMakeLists.txt').read_text(), re.S).group(1).split()
    includes = [root / p for p in ('src', 'src/fdp', 'src/fdp/cnf', 'src/lib',
                                  'src/netlist', 'src/opt', 'src/fdp/xid')]
    includes += [EXT / p for p in ('cadical/src', 'cudd', 'cudd/cudd')]
    flags = ['-g', '-O0', '-DDEBUG'] if debug else ['-O3', '-DNDEBUG']
    command = ['gcc', '-std=gnu11', '-fcommon'] + flags
    command += ['-I' + str(p) for p in includes] + [str(root / p) for p in sources]
    command += [str(EXT / 'cadical/build/libcadical.a'),
                str(EXT / 'cudd/cudd/.libs/libcudd.a'), '-lgmp', '-lm', '-lstdc++',
                '-o', str(binary)]
    binary.parent.mkdir(parents=True, exist_ok=True)
    with binary.with_suffix('.build.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=log, check=True)
    print('Built', binary, flush=True)


def main():
    compile_program(ROOT, ROOT / 'build/main_release')
    compile_program(ROOT, ROOT / 'build/main_debug', True)
    reference = HERE / 'build/reference'
    reference.mkdir(parents=True, exist_ok=True)
    archive = subprocess.check_output(['git', 'archive', REFERENCE_COMMIT,
                                       'src', 'CMakeLists.txt'], cwd=ROOT)
    subprocess.run(['tar', '-x', '-C', str(reference)], input=archive, check=True)
    compile_program(reference, HERE / 'build/main_reference')


if __name__ == '__main__':
    main()
