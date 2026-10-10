"""Build exact pre-pool reference and isolated cube-capture binary for checks.
The production Debug/Release binaries are built with CMake.
"""
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build/fault_pool_checks'
EXTERNAL = Path(os.environ.get('FDP_EXTERNAL_DIR', ROOT / 'external')).resolve()


def compile_program(root, binary):
    sources = re.search(r'set\(SOURCES\s+(.*?)\)',
                        (root / 'CMakeLists.txt').read_text(), re.S).group(1).split()
    includes = [root / part for part in ('src', 'src/fdp', 'src/fdp/cnf', 'src/lib',
                                         'src/netlist', 'src/opt', 'src/fdp/xid')]
    includes += [EXTERNAL / part for part in ('cadical/src', 'cudd', 'cudd/cudd')]
    command = ['gcc', '-std=gnu11', '-fcommon', '-O3', '-DNDEBUG']
    command += ['-I' + str(path) for path in includes] + [str(root / source) for source in sources]
    command += [str(EXTERNAL / 'cadical/build/libcadical.a'),
                str(EXTERNAL / 'cudd/cudd/.libs/libcudd.a'), '-lgmp', '-lm', '-lstdc++', '-o', str(binary)]
    with binary.with_suffix('.build.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=log, check=True)
    print('Built', binary, flush=True)


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    reference = BUILD / 'reference'
    reference.mkdir(exist_ok=True)
    archive = subprocess.check_output(['git', 'archive', 'ac45287', 'src', 'CMakeLists.txt'], cwd=ROOT)
    subprocess.run(['tar', '-x', '-C', str(reference)], input=archive, check=True)
    compile_program(reference, BUILD / 'main_reference')

    # 実生成キューブの保存処理はコピーにだけ挿入する。本体・出荷バイナリには入れない。
    instrumented = BUILD / 'instrumented'
    instrumented.mkdir(exist_ok=True)
    shutil.copytree(ROOT / 'src', instrumented / 'src', dirs_exist_ok=True)
    source = instrumented / 'src/fdp/fault_detection_prob.c'
    text = source.read_text()
    text = text.replace('/* 各プロセス専用。',
                        'void CaptureFaultCubes(FNODE*, CubeSet*, int);\n\n/* 各プロセス専用。', 1)
    assert text.count('            FaultResult result = {') == 1
    text = text.replace('            FaultResult result = {',
                        '            CaptureFaultCubes(target, &cubes, !limit_hit);\n            FaultResult result = {', 1)
    source.write_text(text)
    (instrumented / 'capture.c').write_text('''
#include <stdio.h>
#include <stdlib.h>
#include "fdp/read.h"
#include "fdp/cube_set.h"
void CaptureFaultCubes(FNODE* fault, CubeSet* cubes, int complete) {
    const char* directory = getenv("BASELINE_COVER_DIR");
    if (!directory) return;
    char path[4096];
    snprintf(path, sizeof(path), "%s/%s_sa%d.cover", directory, fault->name, fault->type == SF1);
    FILE* output = fopen(path, "w");
    if (!output) { perror(path); exit(1); }
    fprintf(output, "%s %d %d %d %d\\n", fault->name, fault->type == SF1, complete, n_pi, cubes->n);
    for (int index = 0; index < n_pi; index++)
        fprintf(output, "%s%c", pi[index]->name, index + 1 == n_pi ? '\\n' : ' ');
    for (int index = 0; index < cubes->n; index++) fprintf(output, "%s\\n", cubes->data[index]);
    if (fclose(output) != 0) exit(1);
}
''')
    (instrumented / 'CMakeLists.txt').write_text(
        (ROOT / 'CMakeLists.txt').read_text().replace('set(SOURCES\n', 'set(SOURCES\n    capture.c\n', 1))
    compile_program(instrumented, BUILD / 'main_capture')

if __name__ == '__main__':
    main()
