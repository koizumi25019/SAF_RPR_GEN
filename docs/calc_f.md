# calc F（ランダムパターンによる故障検出率）

`-calc_f` を指定したときだけ、FDP計算の後に書き出したFDP CSVを読み、
ランダムパターンを `Np` 個印加したときの故障検出率 F と故障検出効率 E を計算する。
指定しなければ従来と同じ動作で、FDP CSV・ログ・キューブ出力は `-calc_f` の有無で変わらない。

元は独立ツール `calc_F_gmp`（Windows用）だったものを、本体のオプションとして組み込んだ。
計算式・出力値は元ツールと一致する（c17a・s27で確認済み）。

## 計算式

```text
F = (1/Nf) * Σ P(fi)      E = (1/Nd) * Σ P(fi)

復元抽出（常に計算）       : P(fi) = 1 - (1 - D(fi))^Np
非復元抽出（-calc_f_nr on） : P(fi) = 1 - Π_{k=0..Np-1} (N - Fd - k) / (N - k)
                              N = 2^n_pi,  Fd = round(D(fi) * N)
```

| 記号 | 意味 |
|---|---|
| `D(fi)` | FDP CSVの `fdp` 列 |
| `Nf` | CSVの全故障行（等価故障の行を含む） |
| `Nu` | `fdp = 0` の故障数 |
| `Nd` | `Nf - Nu` |
| `n_pi` | FDPの分母の指数。SAFは外部入力数、TDFは展開後の入力数（PI + DFF） |

`N` は元ツールの `-i` に相当し、本体が回路から自動で決める。
多倍長演算（GMP）の精度は `max(2048, n_pi + 128)` ビットで、`D = 2^-n_pi` でも `1 - D` が1に丸まらない。

## 使い方

```bash
./main_release -set ../input/script/saf/c17a.set -calc_f 100,1000,10000
./main_release -set ../input/script/saf/c17a.set -calc_f 100,1000 -calc_f_nr on -calc_f_out ../output/saf/full/c17a_F.csv
```

| 項目 | 内容 |
|---|---|
| `-calc_f <Np[,Np...]>` | 計算を有効にする。正の整数をカンマ区切りで最大256個。順不同・重複可 |
| `-calc_f_nr on\|off` | 非復元抽出のFも求める。既定off。故障ごとに最大Np回の多倍長演算をするため、大きなNpでは遅い |
| `-calc_f_out <file>` | 結果CSVの出力先。既定は `-fdp` の拡張子の前に `_F` を付けたもの（`c17a.csv` → `c17a_F.csv`） |

CLIと`.set`の両方で指定でき、後の指定が有効になる。`-calc_f` なしで `-calc_f_nr`・`-calc_f_out` を指定するとエラー。
出力先が `-fdp`・`-log`・`-cube` と同じ場合もエラー。不正値はFDP計算の前に検出する。
計算時間はログの `Time`・`CPU Time` に含めない。結果は標準出力にも表示する。

## 出力形式

```text
[Settings]
Item,Value
Circuit,c17a
FDP CSV,../output/saf/full/fdp/c17a.csv
Primary inputs (n_pi),5
N total patterns,2^5
GMP precision (bits),2048
Without replacement,on
Np values,"1 10 100"

[Fault Statistics]
Item,Count,Note
Total faults (Nf),36,rows in FDP CSV
Undetectable (Nu),0,fdp=0
Detectable (Nd),36,Nf-Nu
Equivalent (Neq),14,cube_cnt blank
Incomplete (Ninc),0,complete=0 (fdp is a lower bound)

[F/E Results]
Np,F with-repl (=Sum/Nf),F_nr without-repl (=Sum/Nf),E with-repl (=Sum/Nd),E_nr without-repl (=Sum/Nd)
1,0.290798611111111,0.290798611111111,0.290798611111111,0.290798611111111
...
```

`-calc_f_nr off` のときは `Np,F (=Sum/Nf),E (=Sum/Nd)` の3列になる。
元ツールの `N30`（`cube_cnt = 30` の数）は上限値に依存するため、`complete = 0` の故障数 `Ninc` に置き換えた。
`Ninc > 0` のとき、FDPは下界なので F・E も下界になる。

## 注意

- 入力はFDP CSVの文字列（有効11桁）なので、F・Eはその丸めの影響を受ける。元ツールでCSVを読んだ場合と同じ値になる。
- 非復元抽出の `Fd` はCSVのFDPから最近接整数として復元する。有効11桁（相対誤差最大5×10^-11）から見積もると、`D * N` が約10^10を超えると厳密な検出パターン数とずれうる。
- 元ツールの集計CSV（`-s`、複数回路を1ファイルに追記）は移植していない。

## 実装

`src/coverage/` に閉じており、本体とは次の3か所だけでつながる。

| 場所 | 内容 |
|---|---|
| `src/cli/options.{h,c}` | `OPTION.coverage` と、`-calc_f*` を `SetCoverageOption()` へ渡す分岐（CLI・`.set`） |
| `src/main.c` | ログ出力後に `RunCoverage(&opt.coverage, -fdpのパス, 回路名, n_pi)` を1回呼ぶ |
| `CMakeLists.txt` | ソース3つとテスト |

| ファイル | 役割 |
|---|---|
| `coverage.h` | 公開API・設定構造体 |
| `coverage_options.c` | オプション解析・既定の出力先 |
| `coverage_model.{h,c}` | 数値計算（復元・非復元抽出）。CSVに依存しない |
| `coverage.c` | FDP CSVの読込・集計・結果出力 |

FDP計算の内部状態（故障リスト・BDD・並列ワーカー）は参照しない。並列実行でも親が書き終えたCSVを読むだけなので、
直列・並列で結果は同じになる。

テストは `coverage_model`（数値計算の単体テスト）と `calc_f`（実行ファイル経由で、分数による独立計算と照合）。
