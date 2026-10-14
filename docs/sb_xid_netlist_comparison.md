# FDPとSB-XIDのネットリスト・状態の比較

2026-10-13。FDP本体は `06d12975f70844294fd85b19fd39b14977eaef28`、SB-XIDは追加コミット `52bb7ac268d2425ab2a8089c7b929fa87d262322` の同梱v1。置換実装は行っていない。

設計更新：現行プログラムを基軸に専用3項目を追加する方針と、danglingの現行TPGでの再現結果を [移植設計](sb_xid_host_integration_design.md) に追記した。本書の末尾の4案比較は検討経緯として残す。

前の検討書の「回路を別表現に変換する」という案は、可能な構成の一つ。コードを詳しく照合した結果、既存NLISTを拡張して回路を直接共用する構成も成立しうる。構造体のレイアウト不一致、論理値の保持場所、フラグの扱いを解決する必要があり、グラフ全体の複製が必須という意味ではない。

**監査の範囲と根拠**

- 両 `Netlist.h` の全フィールド・ゲート定数・グローバルを照合。
- 両 `Netlist.c` 全体を、コメント・空白・内部変数の名称変更を除いて比較。
- `saxid.c` 全体のNLISTフィールド読書きを検索し、各処理での用途を確認。
- 現行のSATモデル生成、EA、正常CNF範囲限定、XID、列挙後処理を確認。
- 別々の調査用プログラムへ両パーサをコンパイルし、既存52回路の全ノード、信号ID・名前・ゲート種別・入力／出力接続の順序・PI/POおよびFF配列を照合。全52回路で一致。
- XOR、XNOR、POからの分岐、未使用の終端信号を持つ小回路を用意して前提差を確認。
- x86-64 Linux、GCC 14.2で `sizeof` / `offsetof` を測定。同名の外部シンボルは `nm` で確認。

実験結果は [parser_results.json](sb_xid_netlist_audit/parser_results.json)、読込処理の全正規化差分は [normalized_parser.diff](sb_xid_netlist_audit/normalized_parser.diff)。パーサのグラフ照合であり、FDP・PBO最適化の統合試験ではない。

ソース参照：

- FDP：`src/netlist/netlist.h`、`src/netlist/netlist.c`、`src/fdp/read.h`、`src/fdp/init.c`、`src/fdp/essential_assignment.c`、`src/fdp/cnf/`、`src/fdp/normal_scope_values.c`、`src/fdp/xid/`、`src/fdp/fault_detection_prob.c`。
- [SB-XID Netlist.h](https://github.com/koizumi25019/SAF_RPR_GEN/blob/52bb7ac268d2425ab2a8089c7b929fa87d262322/SB-XID/src/Netlist.h)、[Netlist.c](https://github.com/koizumi25019/SAF_RPR_GEN/blob/52bb7ac268d2425ab2a8089c7b929fa87d262322/SB-XID/src/Netlist.c)、[saxid.c](https://github.com/koizumi25019/SAF_RPR_GEN/blob/52bb7ac268d2425ab2a8089c7b929fa87d262322/SB-XID/src/saxid.c)。以下のSB行番号はこれらに対応する。

## 1. NLIST全フィールド

FDPは24項目、SB-XIDは21項目。共通項目は11個。SB-XID本体が実際に使うNLIST項目は9個である。

| 両方にある項目 | 型 | FDPでの意味 | SB-XIDでの使用 | 接続上の扱い |
|---|---|---|---|---|
| `name` | `char*` | 信号名。幹・枝を含む | 名前→配列添字の検索、診断、OPBコメント。信号名自体は変更しない | 同じ名前を使える。文字列の所有権を決める |
| `n` | `int` | `nl[]` の添字となるID | `saxid.c` は参照しない。名前検索から添字を取得 | 本体のIDを保てば対応が簡単 |
| `type` | `int` | ゲート種別 | 評価、正当化、PI判定 | 数値定義は同じ。読込時のXOR分類に差あり |
| `n_in` | `int` | 入力本数 | 評価・後方探索・制約生成 | そのまま使用できる |
| `in` | `NLIST**` | 入力信号への順序付きポインタ配列 | 正常／故障値の取得と後方探索 | 別NLISTを作るなら指し先も対応先へ張り替える |
| `n_out` | `int` | 出力先本数 | シミュレーションの前方イベント配布 | そのまま使用できる |
| `out` | `NLIST**` | 出力先へのポインタ配列 | 同上 | 別NLISTなら指し先の張り替えが必要 |
| `name_port` | `char*` | ピン名 | 参照しない | SB-XIDの計算用に変換不要 |
| `name_ins` | `char*` | インスタンス名 | 参照しない | 同上 |
| `level` | `int` | 初期化時に求めるトポロジカルレベル | 定義されているが参照しない | SB-XIDのための再計算・変換は不要 |
| `flag` | `int` | FP / TFO / TPO | EVALUATED / PATH / JUSTIFYを読み書き | 意味・ビットが異なる。分離または段階的共用を設計 |

SB-XIDが使う9個は `name,type,n_in,in,n_out,out,flag,value,signal`。`name` の用途も主に索引であり、接続構造そのものは双方とも同じ信号グラフである。

**FDPだけにある13項目**

| 項目 | 型 | 実際の用途 | SB-XIDとの対応 |
|---|---|---|---|
| `varsgc` | `unsigned int` | 正常回路のSAT変数番号。PIのSAT値取得と禁止節に使用 | SBのPBO番号に置換しない |
| `varsfc` | `unsigned int` | 故障回路のSAT変数番号。TFO外では正常側番号を共有 | SBの故障論理値とは異なる |
| `varprop` | `unsigned int` | D-chainのSAT変数番号 | SBには対応項目なし |
| `logic_value` | `int` | EAが求めた必須割当て。未確定は-1 | SBの全回路シミュレーション値 `value` に流用しない |
| `ea_flag` | `int` | EA_DOWN=0 / EA_UP=1 | SBのPATH/JUSTIFYとは独立 |
| `unique_flag` | `int` | EAの一意化探索。DOWN=0 / UP=1 / MIDDLE=2 | SBには対応項目なし |
| `consgc` | `int*` | 正常回路CNFの0終端節を並べた配列 | 移植しない。既存CaDiCaL側のもの |
| `consgc_len` | `int` | `consgc` の配列長 | 同上 |
| `consfc` | `char**` | この版ではNULL初期化されるのみ | SBのために変換不要 |
| `suf_fc` | `unsigned int` | 現行src内では宣言のみ | 同上 |
| `fault_pass` | `unsigned int` | 現行src内では宣言のみ | 同上 |
| `test_sa0` | `int` | 縮退0故障を代表故障に含めるか。等価故障出力にも使用 | SBの `sa0` と同一視しない |
| `test_sa1` | `int` | 縮退1故障について同上 | SBの `sa1` と同一視しない |

**SB-XIDだけにある10項目**

| 項目 | 型 | 同梱v1の本体での使用 | 必要性 |
|---|---|---|---|
| `target` | `int` | 使用なし | 移植用には不要 |
| `value` | `int` | 正常回路のシミュレーション値。0/1/X=2 | 必要 |
| `signal` | `int` | 故障回路のシミュレーション値。0/1/X=2 | 必要 |
| `f0ptr` | `struct _Fault0_Table_*` | 使用なし | 不要 |
| `f1ptr` | `struct _Fault0_Table_*` | 使用なし。同じ型が宣言されている | 不要 |
| `sa0` | `int` | 使用なし | 本体の `test_sa0` をコピーする理由はない |
| `sa1` | `int` | 使用なし | 同上 |
| `cc0` | `int` | 使用なし | PBO目的関数はこの費用を使っていない |
| `cc1` | `int` | 使用なし | 同上 |
| `cco` | `int` | 使用なし | 同上 |

SBの `FAULT_0` は `net` と `detection` の2項目を持つが、`saxid.c` はこの構造体・ポインタを使用しない。現行のFNODE（故障種別、検出状態、キューブ、支配関係等）に変換する対象ではない。

## 2. レイアウトは互換ではない

両ヘッダは同じ `struct _Netlist_Format_` / `NLIST` という名前を使っている。異なるヘッダを同じ翻訳単位へ含めれば再定義になる。別々にコンパイルして名前だけ一致させても、以下の不一致がある。

調査環境の `sizeof(NLIST)` はFDP=136、SB=128バイト。アラインメントはいずれも8。最初の9項目、`name` から `name_ins` までのオフセットは一致するが、その後は一致しない。

| 項目 | FDPのオフセット | SBのオフセット | SBの定義でFDPのメモリを読んだ場合 |
|---|---:|---:|---|
| `level` | 92 | 64 | FDPの `flag` をレベルとして読む |
| `flag` | 64 | 68 | FDPの `varsgc` をフラグとして読み書きする |
| `target` | なし | 72 | FDPの `varsfc` に重なる |
| `value` | なし | 76 | FDPの `varprop` に重なる |
| `signal` | なし | 80 | FDPの `logic_value` に重なる |

さらに配列のストライドが違うため、SB側の `nl[1]` は128バイト先、本体の本当の `nl[1]` は136バイト先になる。共通の先頭項目であっても2要素目以降は位置がずれる。`NLIST*` のキャスト、共有グローバルのリンクだけでは解決しない。

オフセット値は調査環境の例であり、実装へ固定値として埋め込むものではない。型を統一するか、対応する別の構造体を構築する。

## 3. flagの意味と生存期間

FDPの `read.h:39–43` とSBの `saxid.c:69–71` は次の定義である。

| 値 | FDPの意味 | SB-XIDの意味 |
|---:|---|---|
| 1 | FP：故障点 | EVALUATED：評価したノード |
| 2 | TFO：故障点から出力方向へ到達可能 | PATH：元パターンで正常値と故障値が違い、検出POから遡った領域 |
| 4 | TPO：故障TFO中の観測終端 | JUSTIFY：PATHの値を正当化する入力側領域 |

TFOとPATHは範囲の定義も異なる。TFOは回路構造で決まり、元の入力パターンによらない。PATHは実際の正常／故障値に基づくため、パターンごとに変わる。EAとSBのJUSTIFYも別の処理である。

SB側は以下のように書き換える。

```c
/* saxid.c:417 */
void clear_all_flags() {
    for (int i = 0; i < n_net; i++) nl[i].flag = 0;
}

/* saxid.c:423,442,495,540 */
nl[i].flag &= ~FLAG_EVALUATED;
node->flag |= FLAG_EVALUATED;
current->flag |= FLAG_PATH;
node->flag |= FLAG_JUSTIFY;
```

`build_multi_case()` は故障シミュレーション前とPATH抽出前に全消去する（577、581行）。同じ `flag` を無策に共用するとFDPのFP/TFO/TPOを消すことになる。

ただし、**現行の処理順なら段階的に共用できる余地がある**。TPGモデル・正常CNF範囲はXIDの前に作られる。その後はキューブの禁止節、CubeSet、BDD、FNODEの更新を行い、これらはNLIST.flagを使わない。次の故障では `SearchTFO()` が再びフラグを初期化する。正常CNFの保持範囲も別の `needed[]` 配列に保存済みで、PI値取得時にフラグを再解釈しない。

したがって「フラグを共用すれば現行FDPが必ず壊れる」とは断定しない。段階的共用を採用するなら生存期間の契約を明示する。保存・復元を呼出側で行えば元のSBフラグ操作を保てる。別フィールドを用意する方法もある。ビットを8/16/32にずらすだけでは、`clear_all_flags()` の全消去が残るため不十分。

EAの `ea_flag` / `unique_flag`、XIDの `ed_tag` / `edx_tag` / `xid_tag` はそれぞれ別の状態であり、SBの3ビットへ意味を変換する対象ではない。

## 4. 正常・故障値の保存場所と値域

現行NLISTの `logic_value` は全信号の正常値ではなく、EAによる必須割当て。未割当ては-1。EAはこれを使って単位節をCaDiCaLへ投入する（`essential_assignment.c:67–79`）。

現行XIDは、正常／故障の2値・3値を別の `XID_VAR_INFO[n_net]` に持つ（`xid/XID.h:32–40`）。SBはNLIST内部の `value` と `signal` を両方の用途に使い、初回の完全パターンでは0/1、候補キューブ検証時にはX=2も入れる（`saxid.c:350–361,421–468`）。

```c
pi[i]->value = val;          /* 正常側のPI */
pi[i]->signal = val;         /* 故障側のPI */
node->value = evaluate_gate_logic(node, 0);
node->signal = evaluate_gate_logic(node, 1);
if (node == &nl[fault_net_id]) node->signal = fault_type;
```

`logic_value` をSBの `value` に読み替えることはできない。専用の2値を追加するか、外部配列へ持たせる。

両パーサの `alloc_nl()` はmallocした配列のtype・入力／出力本数とポインタを初期化するだけで、SBのvalue/signalを初期化しない。SBのシミュレーションは429–430行で更新前の値も読むため、接続部では初回を含む作業状態を明示的に初期化する。

検証はvalue/signalを候補キューブの3値へ上書きする。フォールバック用の元の0/1パターンは別に保持し、検証後のPI.valueから復元しない。

## 5. 4種類の番号を区別する

| 番号 | 意味 | 基本の値域 |
|---|---|---|
| `nl[]` 添字 / `node->n` | 信号ID | 0..n_net-1 |
| `pi[]` 添字 | パターン文字列での位置 | 0..n_pi-1 |
| `varsgc` / `varsfc` / `varprop` | CaDiCaLの変数ID | 正の整数。0は未割当て |
| SBの `varV[]` / `varS[]` | ケア／任意を表すPBO変数ID | 正の整数。0は未割当て |

例えばc17aでは `pi[0]` はa、aの信号IDは2。`AssigneVarsGC()` はPI順で先に番号を振るのでaの `varsgc` は1になる。これらを同じ番号として扱えない。

SBの `get_net_index()` は名前から `nl[]` 添字を返す。`g_pi_of_net[]` は信号ID→PI文字位置を保持する。PBOのモデル値1は「その入力がケア」であり、信号の論理値が1という意味ではない。モデル値1なら元パターンの0または1を残す（`saxid.c:1174–1183`）。

回路を別構造体にする場合、`in/out/pi/po` の指し先と故障点を同じ対応表で変換する。名前とtypeだけコピーし、接続ポインタをFDP側のまま残すと、入力参照時に異なるNLISTレイアウトを読む。

## 6. PI順序とドントケア文字

両パーサはinput宣言順で `pi[]` を作る。信号ID順ではない。c17aではPIの信号ID列は `[2,6,0,4,8]` である。

FDPのキューブはこの `pi[]` 順で `0/1/X`。禁止節追加関数は `0` と `1` のみを読み、他の文字は節へ追加しない（`fault_detection_prob.c:30–38`）。SBは `0/1/-` を出力し、入力としては `-` / `X` / `x` をXとして扱う。最終結果を `X` へ統一する必要がある。記号を全モジュールで無条件に互換と扱わない。

SBには外部pinファイルに基づく並べ替えがあるが、ライブラリ接続では本体の `pi[]` 順を直接使える。同じ回路をコピーする際にもこの順序を保つ。

## 7. 正常CNFの範囲限定

現行の正常SATモデルは全回路を投入していない。`NormalScopeModelValues()` は保持範囲内PIのSAT値を読み、範囲外PIは0で補完し、その後回路全体を論理シミュレーションする（`normal_scope_values.c:37–70`）。

```c
values[pi[i]->n] = NormalScopeRequiredNet(pi[i]->n)
    ? ccadical_val(solver, (int)pi[i]->varsgc) > 0 : 0;
```

SBへの入力もこの完全な0/1パターンとし、未投入の内部SAT変数からvalue/signalを作らない。元のv1は全回路の接続を前提にシミュレーションするので、まず現行グラフ全体を利用する。SAT範囲外ゲートをSBからも削ることは別の設計変更になる。

全PIと `n_pi` を維持することはキューブ長・PI順序・FDPの分母 `2^n_pi` のためにも必要。`n_net` はSBのPI重み `(K+1)*n_net+1` にも使われる。別のデータ構造にすることと、回路を削減してノード数・入力数を変えることは別の変更である。

SBの元入力にXがあると、PATH抽出の `value != signal` は片側Xも不一致として扱いうる（479、500行）。最終検証では両側がXでないことを要求する（467行）。今回の完全0/1入力を保つことは、この前提差を避ける意味もある。

## 8. PO・終端・分岐の表現

**分岐の展開方法は共通。** ゲート入力へ入る枝をFOUT=18の別ノードにし、名前は `信号名_接続先出力名_ピン名`。外部出力がさらにゲートへ入る場合は幹を `_stem` に改名し、元の出力名を持つ終端FOUTを作り、`po[]` をその終端へ向ける（FDPの `connection_yes_fout():980`、`connection_po_fout():1068`。SBも同じロジック）。

PO分岐の調査用回路では、両方とも `z_stem(AND)` → `z_y_A(FOUT)` → `y(INV)` と、`z_stem` → `z(FOUT,終端PO)` を生成した。分岐や幹の名前を作り直す必要はない。

**観測対象の決め方には差がある。** 現行の `SearchTFO()` は `n_out==0` をTPOとし、XIDも同じ条件で検出先へ追加する。SBは `po[]` にあるノードだけを検出POとして扱う。

```c
/* FDP faulty_circuit.c:105–115 */
if (netptr->n_out != 0) {
    /* 出力方向へ探索 */
} else {
    numtranpo++;
    netptr->flag |= TPO;
}

/* SB saxid.c:466–467 */
for (int i = 0; i < n_po; i++) {
    if (po[i]->value != XVAL && po[i]->signal != XVAL
        && po[i]->value != po[i]->signal) return 1;
}
```

既存52回路では「nout=0の集合」と `po[]` が一致した。未使用内部信号danglingを持つ小回路では、danglingとzがnout=0になる一方、`po[]` はzだけだった。その場合に現行TPGがdanglingを観測先とみなす故障は、SBの入力検証と一致しない。現在のベンチマークでは発生していない前提差として扱い、一般化時に観測対象の仕様を決める。

FDPには `ppo` / `n_ppo` も宣言されるが、現行src内では宣言だけで構築・使用はない。SBにないことを理由に、擬似出力が現行処理で失われるとはいえない。`_C` 回路の擬似入出力はVerilog上のinput/outputへ変換済みである。

## 9. ゲート種別と読込処理の全差分

定数は全16種類で一致する。

```text
IN=0, BUF=10, INV=11, AND=12, NAND=13, OR=14, NOR=15,
EXOR=16, EXNOR=17, FOUT=18, DFF=19, RDFF=21,
DFFS=23, RDFFS=24, GND=25, ACC=26
```

ただしread_module_typeのEで始まる型名の分類が異なる。

```c
/* FDP netlist.c:404 */
case 'E': if (strlen(str) == 4) type = EXOR;
          else type = EXNOR;

/* SB Netlist.c:401 */
case 'E': if (strlen(str) == 5) type = EXOR;
          else type = EXNOR;
```

上は代入先の長い表記をtypeへ省略した抜粋。`get_token(...,2,&in_num)` は末尾の入力数を分離するため、EXOR2はEXOR（4文字）、EXNOR2はEXNOR（5文字）で判定される。調査用回路を実際に読むと、SBはEXOR2をEXNOR=17、EXNOR2をEXOR=16に分類した。接続やIDは同じで、出力ゲートのtypeだけが変わる。

本体が読んだtypeを共用／コピーし、SBのNetlist.cを呼ばなければ、この読込差をSBコアへ持ち込まずに済む。SBのゲート評価関数自体のEXOR/EXNOR計算は通常のXOR/XNORである。

両Netlist.c全体のコード差分は、コメント・空白・内部名変更を除くと次の内容だった。

- `_CRT_SECURE_NO_WARNINGS` と `#pragma warning(disable:4996)`、ヘッダ名の大文字小文字。
- 現行版は `read_nl()` でファイルを閉じ、SBは完了メッセージを出してファイルを閉じない。
- 上記EXOR/EXNORの長さ条件。
- `strlen()` のintキャスト、未使用変数の削除。
- エラーメッセージの文言。
- 現行版は `m_in_name` のポインタ配列も解放し、SBではその配列の解放がない。
- 現行版には `free_netlist()` があり、SBにはない。

内部の `nl_id`→`nid`、`out_nl_id`→`out_nid`、`in_nl_id`→`in_nid` は名称変更。分岐展開、PI/PO順序、通常のゲート接続の意味が変わる差は見つからなかった。読込処理を2つ統合する必要はない。

## 10. 定数があることと実際の対応範囲

| ゲート群 | 現行FDP | SB-XID | 接続上の意味 |
|---|---|---|---|
| IN、AND/NAND/OR/NOR、BUF/FOUT/INV | SAT・シミュレーション対象 | 対応する3値評価・正当化処理がある | 現行typeを利用 |
| EXOR/EXNOR | 読込は2入力に制限、2入力演算 | パーサは2入力制限。評価関数は全入力のパリティを計算 | 今回は2入力。読込差は本体グラフ使用で回避 |
| DFF | SAT側では定義生成をスキップし、levelの起点。これだけで生の順序回路に一般対応するとはいえない | シミュレーションにDFFのcaseがなく、IN以外として評価されるとdefault=0 | `_C` で組合せ化された入力を基準にする |
| RDFF/DFFS/RDFFS | 正常CNF生成のdefaultで未対応エラー | ゲート評価のdefault=0 | ヘッダの定数一致だけで対応と扱わない |
| GND/ACC | 正常CNF・正常シミュレーションでは未対応 | 評価関数に0/1のcaseはある | 両パーサはassign読込の呼出しが無効。PIのみをキューの起点にするので、SBにも一般的な定数回路対応を保証しない |

両パーサがDFF配列を作れること、FDPにppoの宣言があることを、SBのシミュレーションとそのまま接続できる証拠にしない。今回の既存52回路のグラフ比較は、この対応範囲を拡張する検証ではない。

## 11. 故障表現と状態の値

FDPはFNODEに `netptr`、`type`、`detect`、等価／支配関係とキューブを持つ。SBの処理入力はFAULT_SPECの `net_id` と `type` であり、これらからFAULT_CTX / MULTI_CASEを作る。

| 意味 | FDP | SB |
|---|---|---|
| 縮退0 | SF0=8 | type=0 |
| 縮退1 | SF1=16 | type=1 |
| 故障点 | `target->netptr` | `&nl[fault_net_id]` |
| 検出状態 | DETECTED=7、UNDETECTED=17、REDEUNDANT=27 | 検証関数の戻り値0/1。別に求解状態がある |

SBは故障側へ `node->signal=fault_type` と代入する。8/16を渡すと論理値として不正なので、0/1へ明示変換する。現行XIDは検出用SATモデルの励起条件を前提に、正常値を反転して故障値を作る（`xid_fsim.c:104–105`）。SBは反転による推定ではなく対象FNODEの故障型を渡す。

SBのFAULT_SPEC.nameは256文字配列、FDPの信号名は動的文字列。本体から信号IDを直接渡せば名前の再解析・切詰めに依存しない。SBの既存name索引は制約生成内部で使える。

## 12. グローバルと外部シンボル

両Netlist.hはextern宣言ではなく、次の17個のグローバルを同じ名前で定義する。

```text
nl, module_name, n_net, pi, n_pi, po, n_po,
dff, n_dff, rdff, n_rdff, dffs, n_dffs, rdffs, n_rdffs,
assign, n_assign
```

FDPだけに `ppo` / `n_ppo` がある。SBコアが計算で使うのは `nl,n_net,pi,n_pi,po,n_po`。他は主に読込ヘッダの共通定義であり、すべてのグローバルに接続部を用意する必要はない。

別ネットリストを持つならこれらを名前分けする。同じネットリストを使うなら同じ型・同じ配列であることを明示する。既存ビルドの `-fcommon` は同名のCグローバルをまとめうるため、型が異なるままリンクが通ることは互換性の証明にならない。C++側の多重定義も避ける必要がある。

`nm` で確認した衝突する関数は、SB本体の `main/enqueue/dequeue`、SBパーサの `read_nl`。両パーサを含める場合には `parser_array` と `pin_name_array` も衝突する。

キューのenqueueはプロトタイプも異なる。

```c
/* FDP xid_adapter.h:42 */
void enqueue(Queue_t* q, void* item);
/* SB saxid.c:208 */
void enqueue(NLIST* node);
```

SBのCLI/mainとNetlist.cをライブラリ経路から外し、必要なSB関数を名前分けする。既存XIDを比較用にビルドへ残す間はenqueue/dequeueの衝突を解消する。

## 13. 寿命・所有権・シミュレーションの作業状態

- **NLISTと名前・接続の寿命**：現行回路は読込後に安定し、SBの名前索引は文字列を借用する。回路が変わると索引は作り直す。名前を借用した別NLISTを現行 `free_netlist()` 相当で丸ごと解放すると二重解放になるので所有権を定める。
- **状態の寿命**：FP/TFO/TPO・EA・varsfcは故障単位。SBの正常／故障値・PATH/JUSTIFYはパターンおよび故障単位。バッチ版は故障ごとにsignal/PATH/JUSTIFYをFAULT_CTXへ退避し、正常側は共通として扱う。現在の用途ではK=1。
- **level**：現行XIDはlevelのイベントスタックを使う。SBはPIからのFIFOイベント処理でlevelを使わない。評価の順序と途中状態が異なるため、作業値を無初期化で共有しない。
- **キュー**：SBは `n_net*10` の容量を毎回確保し、満杯になるとenqueueを黙って捨てる。現在のキュー関数を同名のまま差し替えると引数・所有権・処理が一致しない。これは個別に管理する。
- **検証の実行条件**：SBバッチ版はログも結果出力も無効なら検証を省く（1223行）。ライブラリ化で出力を外す際にも検証は必須にする。検証はSBのvalue/signalを上書きする。
- **元入力と返却キューブ**：元の0/1パターンは検証失敗時に返せるよう保持。SBの値・flagの復元と、返却文字列の所有権はすべての終了経路で処理する。
- **並列**：現行の並列はforkしたワーカーが故障を順次処理する方式。グラフもグローバルもプロセス間では独立する。SBのグローバルを同時に複数スレッドから使えるAPIとは扱わない。
- **Linux上の読込周辺**：SBのNetlist.c/hはCP932、saxid.cはUTF-8。ヘッダ名の大文字小文字、CLIのstrtok_s、Windowsパス・system呼出しは移植周辺の差。同じ回路グラフを表すかという問題とは分ける。

## 14. 接続案の比較

| 案 | 本体NLISTの変更 | SBコアの変更 | グラフ複製 | 主な確認点 |
|---|---|---|---|---|
| A：SB専用NLISTを構築 | なし | ほぼ同じフィールド操作を維持。名前分け・I/O接続を変更 | ノードと接続配列を構築 | ポインタ変換、所有権、追加メモリ |
| B：NLISTへvalue/signal/sbxid_flagを追加 | 3項目を追加 | flagアクセスを専用項目へ機械的変更 | なし | 本体構造体の拡張、初期化 |
| C：NLISTへvalue/signalを追加、flagを保存・復元 | 2項目を追加 | flag操作は元のまま。接続部で保存・復元 | なし | 全終了経路での復元、呼出中にFDPフラグを読む処理がないこと |
| D：既存NLISTと別の状態配列を利用 | なし | value/signal/flagのアクセスを配列参照へ変更 | なし | 本体IDによる索引、機械的変更箇所の照合 |

Cなら例えば次の流れになる。これは検討用の擬似コードであり、まだ実装していない。

```c
save_fdp_flags();
initialize_sbxid_values();
run_original_sbxid_core();
verify_and_choose_cube();
restore_fdp_flags();  /* 失敗・途中終了でも復元 */
```

B/Cで型を統一する場合はSB側Netlist.hをそのまま併用せず、共通のNLIST定義を参照する。SBのNetlist.cも呼ばず、本体の配列を使う。その上で、故障型変換、PIパターン取得、文字変換、clasp接続、検証とフォールバックを行う。

本体NLISTへの2項目追加を許容するなら、元のSBコードの変更を少なくする目的に対してCは具体的な候補になる。フラグの恒久的な独立を優先するならB。本体の構造体を保つことを優先するならA/D。現在のコード差だけを理由にAを必須とはしない。
