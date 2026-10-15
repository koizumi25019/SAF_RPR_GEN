"""Build production Debug/Release, pre-scope reference and isolated checks.
Pass --checker to build the independent verification oracle in a copied tree.
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build/fault_pool_checks'
EXTERNAL = Path(os.environ.get('FDP_EXTERNAL_DIR', ROOT / 'external')).resolve()
REFERENCE_COMMIT = '52bb7ac'


def compile_program(root, binary, debug=False):
    sources = re.search(r'set\(SOURCES\s+(.*?)\)',
                        (root / 'CMakeLists.txt').read_text(), re.S).group(1).split()
    includes = [root / 'src'] + sorted(p for p in (root / 'src').rglob('*') if p.is_dir())
    includes += [EXTERNAL / part for part in ('cadical/src', 'cudd', 'cudd/cudd')]
    flags = ['-g', '-O0', '-DDEBUG'] if debug else ['-O3', '-DNDEBUG']
    command = ['gcc', '-std=gnu11', '-fcommon'] + flags
    command += ['-I' + str(path) for path in includes] + [str(root / source) for source in sources]
    command += [str(EXTERNAL / 'cadical/build/libcadical.a'),
                str(EXTERNAL / 'cudd/cudd/.libs/libcudd.a'), '-lgmp', '-lm', '-lstdc++', '-o', str(binary)]
    with binary.with_suffix('.build.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=log, check=True)
    print('Built', binary, flush=True)

def build_checker():
    reference = BUILD / 'checker_source'
    reference.mkdir(exist_ok=True)
    archive = subprocess.check_output(['git', 'archive', 'bf8faa8', 'src', 'CMakeLists.txt'], cwd=ROOT)
    subprocess.run(['tar', '-x', '-C', str(reference)], input=archive, check=True)
    checker = subprocess.check_output(['git', 'show',
        'bf8faa8:verification/normal_scope/baseline_migration/checker.c'], cwd=ROOT)
    (reference / 'checker.c').write_bytes(checker)
    source = reference / 'src/fdp/fault_detection_prob.c'
    text = source.read_text()
    at = text.index('static void AddBlockingClauseFromCube')
    text = text[:at] + 'void CheckBaselineCovers(void);\n' + text[at:]
    needle = '\tif (CreateConsGC() != true) return AFD_ERROR;'
    assert text.count(needle) == 1
    text = text.replace(needle, needle + '''
    if (getenv("BASELINE_COVER_DIR")) {
        CheckBaselineCovers();
        *out_time_read = time_read;
        return AFD_OKAY;
    }
''')
    source.write_text(text)
    cmake = reference / 'CMakeLists.txt'
    cmake.write_text(cmake.read_text().replace('set(SOURCES\n', 'set(SOURCES\n    checker.c\n', 1))
    compile_program(reference, BUILD / 'check_covers')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--checker', action='store_true')
    args = parser.parse_args()
    BUILD.mkdir(parents=True, exist_ok=True)
    compile_program(ROOT, ROOT / 'build/main_release')
    compile_program(ROOT, ROOT / 'build/main_debug', True)
    reference = BUILD / 'reference'
    reference.mkdir(exist_ok=True)
    archive = subprocess.check_output(['git', 'archive', REFERENCE_COMMIT, 'src', 'CMakeLists.txt'], cwd=ROOT)
    subprocess.run(['tar', '-x', '-C', str(reference)], input=archive, check=True)
    compile_program(reference, BUILD / 'main_reference')

    # 実生成キューブの保存処理はコピーにだけ挿入する。本体・出荷バイナリには入れない。
    instrumented = BUILD / 'instrumented'
    instrumented.mkdir(exist_ok=True)
    shutil.copytree(ROOT / 'src', instrumented / 'src', dirs_exist_ok=True)
    source = instrumented / 'src/fdp/analysis.c'
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
#include "fault/fault_list.h"
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
    if args.checker:
        build_checker()

if __name__ == '__main__':
    main()
