#!/usr/bin/env python3
"""Check malformed inputs and explicit fault lists through the executable.

不正な入力はハングやクラッシュではなく、メッセージ付きの非ゼロ終了になること、
故障リスト指定時は指定した故障だけが重複なく出力されることを確認する。
"""
import argparse
import csv
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
C17 = ROOT / 'input/circuit/saf/c17a.v'
S27 = ROOT / 'input/circuit/tdf/s27.v'
EXPECTED = ROOT / 'tests/expected/saf/c17a_result.csv'


def rows_of(path):
    with path.open() as file:
        return [(row['net_name'], row['f_type'], row['fdp']) for row in csv.DictReader(file)]


def run(binary, directory, circuit, extra=(), success=True, message=None):
    directory.mkdir(parents=True)
    arguments = [str(binary), '-net', str(circuit), '-fdp', str(directory / 'fdp.csv'),
                 '-log', str(directory / 'time.log'), '-progress', 'off', *extra]
    try:
        result = subprocess.run(arguments, capture_output=True, text=True, timeout=20)
    except subprocess.TimeoutExpired:
        raise AssertionError((directory, 'timeout (infinite loop?)'))
    diagnostics = result.stdout + result.stderr
    assert result.returncode >= 0, (directory, 'killed by signal', result.returncode, diagnostics)
    assert (result.returncode == 0) == success, (directory, result.returncode, diagnostics)
    if message:
        assert message in diagnostics, (directory, diagnostics)
    return directory


def check_binary(binary, output):
    output.mkdir(parents=True)
    expected = {(name, kind): fdp for name, kind, fdp in rows_of(EXPECTED)}

    # 空行・空白だけの行・CRLFの空行を含む故障リストは、それらを無視して読む。
    plain = output / 'plain.faults'
    plain.write_text('c sa0\nf sa1\n')
    blank = output / 'blank.faults'
    blank.write_bytes(b'c sa0\n\n   \n\t\r\n\r\nf sa1\r\n  \n')
    base = run(binary, output / 'plain', C17, ('-fault', str(plain)))
    with_blank = run(binary, output / 'blank', C17, ('-fault', str(blank)))
    assert rows_of(base / 'fdp.csv') == rows_of(with_blank / 'fdp.csv')

    # 故障リスト指定時は指定した故障だけを1行ずつ出す（代表・等価の両方を書いても重複しない）。
    listed = output / 'listed.faults'
    listed.write_text('f sa1\nc_f_A sa0\n')
    directory = run(binary, output / 'listed', C17, ('-fault', str(listed)))
    rows = rows_of(directory / 'fdp.csv')
    assert sorted((name, kind) for name, kind, _ in rows) == [('c_f_A', 'sa0'), ('f', 'sa1')], rows
    for name, kind, fdp in rows:
        assert fdp == expected[name, kind], (name, kind, fdp)

    # 代表故障リストでも、指定した故障の行だけが既知のFDPで出る。
    representatives = ROOT / 'input/fault/saf/c17a_rep.txt'
    names = [tuple(line.split()) for line in representatives.read_text().splitlines() if line.strip()]
    directory = run(binary, output / 'representatives', C17, ('-fault', str(representatives)))
    rows = rows_of(directory / 'fdp.csv')
    assert [(name, kind) for name, kind, _ in rows] != [] and \
        sorted((name, kind) for name, kind, _ in rows) == sorted(names), rows
    for name, kind, fdp in rows:
        assert fdp == expected[name, kind], (name, kind, fdp)

    # -tdf を付け忘れて順序回路を指定した場合はエラーにする。
    run(binary, output / 'saf_sequential', S27, ('-saf',), success=False, message='組合せ回路のみ')

    # 不正なネットリストはハングせずにエラー終了する。
    text = C17.read_text()
    cases = {
        'truncated': (text[:text.index('OR2  G2') + 30], '途中で終わっています'),
        'unknown_gate': (text.replace('AND2 G1', 'XOR2 G1'), '未対応のゲート種別'),
        'assign': (text.replace('endmodule', 'assign x = a;\nendmodule'), 'assign'),
        'pin_out_of_range': (text.replace('.B(a), .Z(g)', '.C(a), .Z(g)'), '入力ピンが不正'),
        'pin_duplicated': (text.replace('.B(a), .Z(g)', '.A(a), .Z(g)'), '入力ピンが不正'),
        'pin_missing': (text.replace('.A(f), .B(a), .Z(g)', '.A(f), .Z(g)'), '入力ピンが不正'),
    }
    for label, (netlist, message) in cases.items():
        circuit = output / (label + '.v')
        circuit.write_text(netlist)
        run(binary, output / label, circuit, success=False, message=message)

    # 空白だけの故障名はエラーにならず読み飛ばされ、空のリストとして扱う。
    whitespace = output / 'whitespace.faults'
    whitespace.write_text('   \n\t\n')
    directory = run(binary, output / 'whitespace', C17, ('-fault', str(whitespace)))
    assert rows_of(directory / 'fdp.csv') == []


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--debug', type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='fdp_input_errors_') as temporary:
        output = Path(temporary)
        check_binary(args.binary.resolve(), output / 'release')
        if args.debug:
            check_binary(args.debug.resolve(), output / 'debug')
    print('Blank fault lines, explicit fault lists, sequential SAF and malformed netlists: PASS')


if __name__ == '__main__':
    main()
