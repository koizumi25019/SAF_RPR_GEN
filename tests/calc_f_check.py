#!/usr/bin/env python3
"""calc F（-calc_f）を実行ファイル経由で確認する。

FDP CSVから分数で独立に F・E を計算し、結果CSVと照合する。
-calc_f 未指定では何も変わらないこと、オプションの検証、直列・並列の一致も確認する。
"""
import argparse
import csv
from decimal import Decimal
from fractions import Fraction
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SAF_CIRCUIT = ROOT / 'input/circuit/saf/c17a.v'
TDF_CIRCUIT = ROOT / 'input/circuit/tdf/s27.v'
TOLERANCE = Fraction(1, 10**14)  # 結果は小数点以下15桁で書かれる


def read_report(path):
    """[Section] ごとに行を分けて返す。"""
    sections, current = {}, None
    for line in path.read_text().splitlines():
        if line.startswith('['):
            current = line.strip('[]')
            sections[current] = []
        elif line and current:
            sections[current].append(line.split(','))
    return sections


def expected_values(fdp_csv, counts, n_inputs, without_replacement):
    rows = list(csv.DictReader(fdp_csv.open()))
    densities = [Fraction(Decimal(row['fdp'])) for row in rows]
    total = len(densities)
    detectable = sum(1 for d in densities if d > 0)
    patterns = 2 ** n_inputs
    result = {}
    for count in counts:
        with_sum = sum(1 - (1 - d) ** count for d in densities if d > 0)
        values = [with_sum]
        if without_replacement:
            without_sum = Fraction(0)
            for d in densities:
                if d == 0:
                    continue
                detecting = min(max(round(d * patterns), 1), patterns)
                miss = Fraction(1)
                for k in range(count):
                    if patterns - detecting - k <= 0:
                        miss = Fraction(0)
                        break
                    miss *= Fraction(patterns - detecting - k, patterns - k)
                without_sum += 1 - miss
            values.append(without_sum)
        f_values = [v / total for v in values]
        e_values = [v / detectable for v in values]
        result[count] = f_values + e_values
    return rows, result


def check_report(report_path, fdp_csv, counts, n_inputs, without_replacement):
    report = read_report(report_path)
    settings = {row[0]: row[1] for row in report['Settings'][1:]}
    assert settings['Primary inputs (n_pi)'] == str(n_inputs), settings
    assert settings['Without replacement'] == ('on' if without_replacement else 'off')
    rows, expected = expected_values(fdp_csv, counts, n_inputs, without_replacement)

    stats = {row[0]: int(row[1]) for row in report['Fault Statistics'][1:]}
    assert stats['Total faults (Nf)'] == len(rows)
    assert stats['Undetectable (Nu)'] == sum(Decimal(r['fdp']) == 0 for r in rows)
    assert stats['Equivalent (Neq)'] == sum(r['cube_cnt'] == '' for r in rows)
    assert stats['Incomplete (Ninc)'] == sum(r['complete'] == '0' for r in rows)

    results = report['F/E Results'][1:]
    assert [int(r[0]) for r in results] == sorted(set(counts)), results
    for row in results:
        # 列順はどちらも F, [F_nr,] E, [E_nr]
        actual = [Fraction(Decimal(v)) for v in row[1:]]
        want = expected[int(row[0])]
        assert len(actual) == len(want), (row, want)
        for a, w in zip(actual, want):
            assert abs(a - w) <= TOLERANCE, (report_path, row, float(w))


def run(binary, directory, *extra, model='-saf', circuit=SAF_CIRCUIT, expect_ok=True):
    directory.mkdir(parents=True, exist_ok=True)
    command = [str(binary), model, '-net', str(circuit), '-fdp', str(directory / 'fdp.csv'),
               '-log', str(directory / 'log.txt'), '-progress', 'off', *extra]
    result = subprocess.run(command, capture_output=True, text=True, timeout=60)
    diagnostics = result.stdout + result.stderr
    if expect_ok:
        assert result.returncode == 0, (command, diagnostics)
    return result, diagnostics


def check_binary(binary, output):
    # 未指定: 結果CSVも標準出力のcalc F欄も作らない。
    _, text = run(binary, output / 'off')
    assert not (output / 'off/fdp_F.csv').exists()
    assert '=== calc F ===' not in text

    # SAF: 既定の出力先、Npの整列・重複除去、非復元抽出。FDP CSVは未指定時と同じ。
    counts = [1000, 1, 10, 100, 10]
    _, text = run(binary, output / 'saf', '-calc_f', ','.join(map(str, counts)), '-calc_f_nr', 'on')
    assert '=== calc F ===' in text
    assert (output / 'saf/fdp.csv').read_text() == (output / 'off/fdp.csv').read_text()
    check_report(output / 'saf/fdp_F.csv', output / 'saf/fdp.csv', counts, 5, True)

    # 復元抽出のみ・出力先指定・.setからの指定。
    settings = output / 'calc_f.set'
    settings.write_text('-calc_f 2,64\n-calc_f_out %s\n-cube_include_incomplete off\n'
                        % (output / 'set/custom.csv'))
    run(binary, output / 'set', '-set', str(settings))
    assert not (output / 'set/fdp_F.csv').exists()
    check_report(output / 'set/custom.csv', output / 'set/fdp.csv', [2, 64], 5, False)

    # 上限付き（未完了=下界）・並列でも同じ結果。
    run(binary, output / 'limit1', '-limit', '1', '-calc_f', '5,50')
    run(binary, output / 'limit1_jobs', '-limit', '1', '-jobs', '4', '-calc_f', '5,50')
    check_report(output / 'limit1/fdp_F.csv', output / 'limit1/fdp.csv', [5, 50], 5, False)
    assert ((output / 'limit1/fdp_F.csv').read_text().split('[Fault Statistics]')[1] ==
            (output / 'limit1_jobs/fdp_F.csv').read_text().split('[Fault Statistics]')[1])

    # TDF: N = 2^(PI + DFF) = 2^7（s27はPI 4本・DFF 3個）。
    run(binary, output / 'tdf', '-calc_f', '3,30,300', '-calc_f_nr', 'on',
        model='-tdf', circuit=TDF_CIRCUIT)
    check_report(output / 'tdf/fdp_F.csv', output / 'tdf/fdp.csv', [3, 30, 300], 7, True)

    # 不正な指定はFDP計算の前にエラー終了する。
    errors = [
        (('-calc_f',), '-calc_f'),
        (('-calc_f', '0'), '-calc_f'),
        (('-calc_f', '-5'), '-calc_f'),
        (('-calc_f', '1.5'), '-calc_f'),
        (('-calc_f', 'abc'), '-calc_f'),
        (('-calc_f', '10,,20'), '-calc_f'),
        (('-calc_f', '10,'), '-calc_f'),
        (('-calc_f', str(2**70)), '-calc_f'),
        (('-calc_f', ','.join(['1'] * 257)), '-calc_f'),
        (('-calc_f', '10', '-calc_f_nr', 'yes'), '-calc_f_nr'),
        (('-calc_f', '10', '-calc_f_out'), '-calc_f_out'),
        (('-calc_f_nr', 'on'), 'require -calc_f'),
        (('-calc_f_out', 'x.csv'), 'require -calc_f'),
    ]
    for index, (extra, message) in enumerate(errors):
        directory = output / f'error_{index}'
        result, text = run(binary, directory, *extra, expect_ok=False)
        assert result.returncode == 1, (extra, text)
        assert 'COMMAND ERROR' in text and message in text, (extra, text)
        assert not (directory / 'fdp.csv').exists(), extra
    result, text = run(binary, output / 'same', '-calc_f', '10', '-calc_f_out',
                       str(output / 'same/log.txt'), expect_ok=False)
    assert result.returncode == 1 and 'must differ' in text, text


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--binary', required=True, type=Path)
    parser.add_argument('--debug', type=Path)
    args = parser.parse_args()
    root = Path(tempfile.mkdtemp(prefix='calc_f_check_'))
    try:
        for label, binary in (('release', args.binary), ('debug', args.debug)):
            if binary:
                check_binary(binary.resolve(), root / label)
    finally:
        shutil.rmtree(root)
    print('calc_f_check: ok')


if __name__ == '__main__':
    main()
