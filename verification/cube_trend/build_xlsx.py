#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
CUBE_TREND の出力（[CT] stderr 行 + 明細CSV）から、2つの仮説を視覚化した
Excel ブックを生成する。

仮説1: キューブ生成回数が多い故障ほど X（ドントケア）が少ない
仮説2: 生成回数が多い故障ほど X の位置がほとんど同じ（マスク使い回し）で拡大効果が薄い

使い方:
  python3 build_xlsx.py <ct.txt> <detail.csv> <out.xlsx> [タイトル]
"""
import sys, re, csv, statistics
from collections import defaultdict, Counter
from openpyxl import Workbook
from openpyxl.chart import BarChart, ScatterChart, LineChart, Reference, Series
from openpyxl.styles import Font, Alignment, PatternFill

CT = re.compile(
    r"^\[CT\]\s+(\S+)\s+(sa0|sa1)\s+n=(\d+)\s*(\(capped\))?\s+"
    r"X/cube mean=([\d.]+)\(([\d.]+)%\)\s+head=([\d.]+)->tail=([\d.]+)\s+"
    r"mask_reuse=([\d.]+)%\s+Xsupport=(\d+)/(\d+)\s+topX=([\d.]+)%\s+"
    r"consec:\s+same_mask=([\d.]+)%\s+val_diff=([\d.]+)"
)

def parse_ct(path):
    rows = []
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = CT.match(line)
            if not m:
                continue
            (name, ft, n, capped, xmean, xpct, head, tail, mreuse,
             xsup, npi, topx, smask, vdiff) = m.groups()
            rows.append(dict(
                fault=name, ftype=ft, n=int(n), capped=bool(capped),
                xmean=float(xmean), xpct=float(xpct),
                xhead=float(head), xtail=float(tail),
                mask_reuse=float(mreuse), xsupport=int(xsup), npi=int(npi),
                topx=float(topx), same_mask=float(smask), val_diff=float(vdiff)))
    return rows

# ----------------------------------------------------------------------
def bucket_of(n):
    edges = [(1, 10), (11, 100), (101, 1000), (1001, 10000), (10001, 10**12)]
    for i, (lo, hi) in enumerate(edges):
        if lo <= n <= hi:
            return i, (f"{lo}-{hi}" if hi < 10**11 else f"{lo}+")
    return len(edges)-1, "?"

def main():
    ct_path, csv_path, out_path = sys.argv[1], sys.argv[2], sys.argv[3]
    title = sys.argv[4] if len(sys.argv) > 4 else "CUBE_TREND"
    faults = parse_ct(ct_path)
    if not faults:
        print("no [CT] lines parsed", file=sys.stderr); sys.exit(1)
    npi = faults[0]["npi"]

    wb = Workbook()

    # ============ Sheet 1: 仮説と結論 ============
    ws = wb.active
    ws.title = "仮説と結論"
    H = Font(bold=True, size=14); B = Font(bold=True)
    ws["A1"] = f"{title} — キューブ生成傾向の検証"; ws["A1"].font = Font(bold=True, size=16)
    lines = [
        ("", ""),
        ("検証対象の仮説", "B"),
        ("仮説1: キューブ生成回数が多い故障ほど X(ドントケア)が少ない", ""),
        ("仮説2: 生成回数が多い故障ほど X の位置がほとんど同じ(マスク使い回し)で", ""),
        ("        新しいキューブが空間を広げない(拡大効果が薄い)", ""),
        ("", ""),
        ("各シートの見方", "B"),
        ("・バケット集計 : 故障をキューブ数で帯分けし、平均X% と マスク重複% を棒グラフ化", ""),
        ("                 → 右肩下がりなら仮説1、右肩上がりなら仮説2を支持", ""),
        ("・故障別散布   : 1点=1故障。横軸=キューブ数(対数的)。X% と マスク重複% の散布", ""),
        ("・重故障の推移 : 生成回数の多い故障1つを取り、キューブ生成順(idx)に沿って", ""),
        ("                 X数・連続キューブ差分がどう動くかを折れ線で表示", ""),
        ("", ""),
        (f"PI数={npi}  解析故障数={len(faults)}  (生成順を保つため MDC_NODOM=1 で列挙)", ""),
    ]
    r = 2
    for text, style in lines:
        ws[f"A{r}"] = text
        if style == "B":
            ws[f"A{r}"].font = B
        r += 1
    ws.column_dimensions["A"].width = 90

    # ============ Sheet 2: バケット集計 ============
    wsb = wb.create_sheet("バケット集計")
    buckets = defaultdict(list)
    for f in faults:
        bi, label = bucket_of(f["n"])
        buckets[bi].append((label, f))
    hdr = ["キューブ数の帯", "故障数", "平均キューブ数", "平均X%(仮説1)", "マスク重複%(仮説2)",
           "X位置数/PI", "連続同マスク%", "連続val_diff"]
    wsb.append(hdr)
    for c in range(1, len(hdr)+1):
        wsb.cell(1, c).font = B
    order = sorted(buckets.keys())
    for bi in order:
        items = buckets[bi]
        label = items[0][0]
        fs = [it[1] for it in items]
        wsb.append([
            label, len(fs),
            round(statistics.mean(x["n"] for x in fs), 0),
            round(statistics.mean(x["xpct"] for x in fs), 1),
            round(statistics.mean(x["mask_reuse"] for x in fs), 1),
            round(statistics.mean(x["xsupport"] for x in fs), 1),
            round(statistics.mean(x["same_mask"] for x in fs), 1),
            round(statistics.mean(x["val_diff"] for x in fs), 2),
        ])
    nb = len(order)
    wsb.column_dimensions["A"].width = 16
    for col in "BCDEFGH":
        wsb.column_dimensions[col].width = 15

    # 棒グラフ: 平均X% (仮説1)
    c1 = BarChart(); c1.title = "仮説1: 平均X%（帯が右へ=キューブ数増 → 下がるか）"
    c1.y_axis.title = "平均X %"; c1.x_axis.title = "キューブ数の帯"
    data = Reference(wsb, min_col=4, min_row=1, max_row=1+nb)
    cats = Reference(wsb, min_col=1, min_row=2, max_row=1+nb)
    c1.add_data(data, titles_from_data=True); c1.set_categories(cats)
    c1.height = 8; c1.width = 16
    wsb.add_chart(c1, "J2")

    # 棒グラフ: マスク重複% (仮説2)
    c2 = BarChart(); c2.title = "仮説2: マスク重複%（帯が右へ=キューブ数増 → 上がるか）"
    c2.y_axis.title = "マスク重複 %"; c2.x_axis.title = "キューブ数の帯"
    data2 = Reference(wsb, min_col=5, min_row=1, max_row=1+nb)
    c2.add_data(data2, titles_from_data=True); c2.set_categories(cats)
    c2.height = 8; c2.width = 16
    wsb.add_chart(c2, "J18")

    # ============ Sheet 3: 故障別散布 ============
    wsp = wb.create_sheet("故障別散布")
    wsp.append(["fault", "type", "cube_cnt", "X%", "mask_reuse%", "Xsupport"])
    for c in range(1, 7):
        wsp.cell(1, c).font = B
    sf = sorted(faults, key=lambda x: x["n"])
    for f in sf:
        wsp.append([f["fault"], f["ftype"], f["n"], f["xpct"], f["mask_reuse"], f["xsupport"]])
    nrow = len(sf)
    wsp.column_dimensions["A"].width = 24

    # 散布図 仮説1: cube_cnt vs X%
    s1 = ScatterChart(); s1.title = "仮説1: キューブ数 vs X%（右に行くほど下がるか）"
    s1.x_axis.title = "キューブ数"; s1.y_axis.title = "X %"
    xref = Reference(wsp, min_col=3, min_row=2, max_row=1+nrow)
    yref = Reference(wsp, min_col=4, min_row=2, max_row=1+nrow)
    ser = Series(yref, xref, title="X%"); ser.marker.symbol = "circle"; ser.marker.size = 3
    ser.graphicalProperties.line.noFill = True
    s1.series.append(ser); s1.height = 9; s1.width = 17
    wsp.add_chart(s1, "H2")

    # 散布図 仮説2: cube_cnt vs mask_reuse%
    s2 = ScatterChart(); s2.title = "仮説2: キューブ数 vs マスク重複%（右に行くほど上がるか）"
    s2.x_axis.title = "キューブ数"; s2.y_axis.title = "マスク重複 %"
    yref2 = Reference(wsp, min_col=5, min_row=2, max_row=1+nrow)
    ser2 = Series(yref2, xref, title="mask_reuse%"); ser2.marker.symbol = "circle"; ser2.marker.size = 3
    ser2.graphicalProperties.line.noFill = True
    s2.series.append(ser2); s2.height = 9; s2.width = 17
    wsp.add_chart(s2, "H20")

    # ============ Sheet 4: 重故障の推移（明細CSVから） ============
    # キューブ数が最大の故障を1つ選ぶ
    detail = defaultdict(list)
    with open(csv_path, newline="") as fh:
        for row in csv.DictReader(fh):
            key = (row["fault"], row["f_type"])
            detail[key].append(row)
    heavy = max(detail.keys(), key=lambda k: len(detail[k]))
    rows = sorted(detail[heavy], key=lambda r: int(r["idx"]))

    wst = wb.create_sheet("重故障の推移")
    wst["A1"] = f"最重故障: {heavy[0]} {heavy[1]}  (cube_cnt={len(rows)})"; wst["A1"].font = B
    wst.append([])
    wst.append(["idx", "x_count", "x_rollmean(50)", "same_mask_vs_prev", "val_diff_vs_prev"])
    for c in range(1, 6):
        wst.cell(3, c).font = B
    win = []
    data_start = 4
    for r in rows:
        idx = int(r["idx"]); xc = int(r["x_count"])
        win.append(xc)
        if len(win) > 50: win.pop(0)
        rollmean = round(sum(win)/len(win), 1)
        sm = r["same_mask_vs_prev"]; vd = r["val_diff_vs_prev"]
        sm = "" if sm == "-1" else int(sm)
        vd = "" if vd == "-1" else int(vd)
        wst.append([idx, xc, rollmean, sm, vd])
    data_end = data_start + len(rows) - 1

    # 折れ線 仮説1: x_count(と移動平均) vs idx
    l1 = LineChart(); l1.title = "仮説1: 生成順(idx) に対する X数（減るか）"
    l1.x_axis.title = "生成順 idx"; l1.y_axis.title = "X数"
    d = Reference(wst, min_col=2, min_row=3, max_col=3, max_row=data_end)
    cat = Reference(wst, min_col=1, min_row=data_start, max_row=data_end)
    l1.add_data(d, titles_from_data=True); l1.set_categories(cat)
    for s in l1.series:
        s.smooth = False
    l1.series[0].graphicalProperties.line.width = 3000   # x_count 細い
    l1.height = 9; l1.width = 20
    wst.add_chart(l1, "G3")

    # 折れ線 仮説2: val_diff vs idx（連続キューブが1ビットしか違わない）
    l2 = LineChart(); l2.title = "仮説2: 連続キューブの相違ビット数 val_diff（ほぼ同一か）"
    l2.x_axis.title = "生成順 idx"; l2.y_axis.title = "val_diff (ケア値が違うビット数)"
    d2 = Reference(wst, min_col=5, min_row=3, max_row=data_end)
    l2.add_data(d2, titles_from_data=True); l2.set_categories(cat)
    l2.height = 9; l2.width = 20
    wst.add_chart(l2, "G22")

    # ============ Sheet 5: val_diff分布（全故障の連続キューブ差分・観察②） ============
    vc = Counter(); tot = 0
    for rws in detail.values():
        for rr in rws:
            vd = rr["val_diff_vs_prev"]
            if vd == "-1": continue
            vc[min(int(vd), 3)] += 1; tot += 1
    wsd = wb.create_sheet("val_diff分布")
    wsd.append(["val_diff", "割合%"]); wsd.cell(1,1).font = B; wsd.cell(1,2).font = B
    for k, lab in [(0,"0"),(1,"1"),(2,"2"),(3,"3+")]:
        wsd.append([lab, round(100*vc[k]/tot, 1) if tot else 0])
    wsd.column_dimensions["A"].width = 12; wsd.column_dimensions["B"].width = 12
    cdv = BarChart(); cdv.title = "観察②: 連続キューブのval_diff分布 (0=ケア値不変=ほぼ複製)"
    cdv.y_axis.title = "割合 %"; cdv.x_axis.title = "val_diff (ケア値が違うビット数)"
    dref = Reference(wsd, min_col=2, min_row=1, max_row=5)
    cref = Reference(wsd, min_col=1, min_row=2, max_row=5)
    cdv.add_data(dref, titles_from_data=True); cdv.set_categories(cref)
    cdv.height = 8; cdv.width = 16
    wsd.add_chart(cdv, "D2")

    wb.save(out_path)
    print(f"wrote {out_path}: {len(faults)} faults, heavy={heavy}, npi={npi}, "
          f"heavy_rows={len(rows)}, buckets={nb}")

if __name__ == "__main__":
    main()
