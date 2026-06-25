#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
CUBE_TREND の出力から、2仮説を簡潔にまとめた PowerPoint を生成する。
使い方: python3 build_pptx.py <ct.txt> <detail.csv> <out.pptx> [タイトル]
"""
import sys, re, csv, json, math, statistics
from collections import defaultdict, Counter
from pptx import Presentation
from pptx.util import Inches, Pt
from pptx.dml.color import RGBColor
from pptx.chart.data import CategoryChartData, XyChartData
from pptx.enum.chart import XL_CHART_TYPE, XL_LEGEND_POSITION
from pptx.oxml.ns import qn

CT = re.compile(
    r"^\[CT\]\s+(\S+)\s+(sa0|sa1)\s+n=(\d+)\s*(\(capped\))?\s+"
    r"X/cube mean=([\d.]+)\(([\d.]+)%\)\s+head=([\d.]+)->tail=([\d.]+)\s+"
    r"mask_reuse=([\d.]+)%\s+Xsupport=(\d+)/(\d+)\s+topX=([\d.]+)%\s+"
    r"consec:\s+same_mask=([\d.]+)%\s+val_diff=([\d.]+)")

XLSX_BLOB = None   # b12_cube_trend.xlsx のバイト列（main で読む）

# PPT のグラフを「Excel ブック埋め込み」にする: グラフの埋め込みデータを実 xlsx に
# 差し替え、系列参照(c:f)を本物のシート範囲へ向ける。これで PowerPoint 上の
# 「データの編集」が b12_cube_trend.xlsx を開く＝Excel で作ったグラフを反映した形になる。
def embed_xlsx(chart, refs):
    chart.part.chart_workbook.update_from_xlsx_blob(XLSX_BLOB)
    fs = chart._chartSpace.findall('.//' + qn('c:f'))
    assert len(fs) == len(refs), f"c:f={len(fs)} != refs={len(refs)}"
    for el, ref in zip(fs, refs):
        el.text = ref

ACCENT = RGBColor(0x1F, 0x49, 0x7D)
GREEN  = RGBColor(0x2E, 0x8B, 0x57)
ORANGE = RGBColor(0xC0, 0x55, 0x00)

def parse_ct(path):
    rows = []
    for line in open(path, encoding="utf-8", errors="replace"):
        m = CT.match(line)
        if not m: continue
        g = m.groups()
        rows.append(dict(fault=g[0], ftype=g[1], n=int(g[2]), capped=bool(g[3]),
            xpct=float(g[5]), xhead=float(g[6]), xtail=float(g[7]),
            mask_reuse=float(g[8]), xsupport=int(g[9]), npi=int(g[10]),
            same_mask=float(g[12]), val_diff=float(g[13])))
    return rows

def buckets(faults):
    edges = [(1,10),(11,100),(101,1000),(1001,10000)]
    out = []
    for lo,hi in edges:
        fs = [f for f in faults if lo<=f["n"]<=hi]
        if not fs: continue
        out.append(dict(label=f"{lo}-{hi}", n=len(fs),
            cubes=statistics.mean(f["n"] for f in fs),
            xpct=statistics.mean(f["xpct"] for f in fs),
            reuse=statistics.mean(f["mask_reuse"] for f in fs),
            same=statistics.mean(f["same_mask"] for f in fs),
            vdiff=statistics.mean(f["val_diff"] for f in fs)))
    return out

# ---------- slide helpers ----------
def title_only(prs, title):
    s = prs.slides.add_slide(prs.slide_layouts[5])
    s.shapes.title.text = title
    s.shapes.title.text_frame.paragraphs[0].font.size = Pt(30)
    s.shapes.title.text_frame.paragraphs[0].font.color.rgb = ACCENT
    return s

def bullets(slide, items, left=Inches(0.6), top=Inches(1.5),
            width=Inches(8.8), height=Inches(5.0), size=18):
    tb = slide.shapes.add_textbox(left, top, width, height).text_frame
    tb.word_wrap = True
    for i,(text,lvl,bold,color) in enumerate(items):
        p = tb.paragraphs[0] if i==0 else tb.add_paragraph()
        p.text = text; p.level = lvl
        p.font.size = Pt(size - 2*lvl); p.font.bold = bold
        if color: p.font.color.rgb = color
        p.space_after = Pt(6)
    return tb

def bar_by_bucket(slide, bks, key, title, color, fmt="{:.1f}"):
    cd = CategoryChartData()
    cd.categories = [b["label"] for b in bks]
    cd.add_series(title, [round(b[key],1) for b in bks])
    gf = slide.shapes.add_chart(XL_CHART_TYPE.COLUMN_CLUSTERED,
        Inches(0.5), Inches(1.5), Inches(5.6), Inches(4.8), cd)
    ch = gf.chart; ch.has_legend = False
    plot = ch.plots[0]; plot.has_data_labels = True
    plot.data_labels.number_format = '0.0'; plot.data_labels.number_format_is_linked = False
    plot.data_labels.font.size = Pt(12)
    ser = plot.series[0]; ser.format.fill.solid(); ser.format.fill.fore_color.rgb = color
    ch.value_axis.has_major_gridlines = True
    ch.category_axis.tick_labels.font.size = Pt(12)
    return gf

# ビット列を色分けグリッド（表）で描く: 0=青, 1=橙, X=灰
C0 = RGBColor(0xCF, 0xE2, 0xF3); T0 = RGBColor(0x10, 0x40, 0x70)
C1 = RGBColor(0xFB, 0xE0, 0xC0); T1 = RGBColor(0x9C, 0x40, 0x00)
CX = RGBColor(0xEE, 0xEE, 0xEE); TX = RGBColor(0xBB, 0xBB, 0xBB)
def bit_grid(slide, ex, left, top, colw=0.205, rowh=0.26):
    cols = ex["cols"]; rows = ex["rows"]; start = ex["start"]
    nr = len(rows) + 1; nc = len(cols) + 1
    gtab = slide.shapes.add_table(nr, nc, left, top,
        Inches(0.55 + colw*len(cols)), Inches(rowh*nr)).table
    gtab.first_row = False; gtab.horz_banding = False
    gtab.columns[0].width = Inches(0.55)
    for j in range(len(cols)): gtab.columns[j+1].width = Inches(colw)
    # ヘッダ行 = 列番号
    gtab.cell(0,0).text = "idx"
    for j,col in enumerate(cols):
        c = gtab.cell(0, j+1); c.text = str(col)
        for p in c.text_frame.paragraphs:
            p.alignment = 2
            p.font.size = Pt(7); p.font.color.rgb = RGBColor(0x88,0x88,0x88)
    # データ行
    for i,bits in enumerate(rows):
        rc = gtab.cell(i+1, 0); rc.text = str(start+i)
        for p in rc.text_frame.paragraphs: p.font.size = Pt(9); p.font.bold = True
        for j,col in enumerate(cols):
            v = bits[col]
            cell = gtab.cell(i+1, j+1)
            fill, txt, tc = (CX,"·",TX)
            if v=='0': fill,txt,tc = (C0,"0",T0)
            elif v=='1': fill,txt,tc = (C1,"1",T1)
            cell.fill.solid(); cell.fill.fore_color.rgb = fill
            cell.text = txt
            for p in cell.text_frame.paragraphs:
                p.alignment = 2; p.font.size = Pt(9); p.font.color.rgb = tc
                p.font.bold = (v!='X')
    for r in range(nr):
        for cc in range(nc):
            tf = gtab.cell(r,cc).text_frame
            tf.margin_left=Pt(1); tf.margin_right=Pt(1)
            tf.margin_top=Pt(0); tf.margin_bottom=Pt(0)
    return gtab

# ---------- main ----------
def main():
    global XLSX_BLOB
    ct_path, csv_path, out_path = sys.argv[1:4]
    title = sys.argv[4] if len(sys.argv)>4 else "CUBE_TREND"
    # 同名 .xlsx を各グラフの埋め込みデータとして使う（Excel で作ったグラフを反映）
    xlsx_path = out_path.rsplit('.', 1)[0] + '.xlsx'
    XLSX_BLOB = open(xlsx_path, 'rb').read()
    faults = parse_ct(ct_path)
    npi = faults[0]["npi"]
    bks = buckets(faults); nb = len(bks)

    prs = Presentation()
    prs.slide_width = Inches(10); prs.slide_height = Inches(7.5)

    # 1. タイトル
    s = prs.slides.add_slide(prs.slide_layouts[0])
    s.shapes.title.text = "テストキューブ生成回数 爆発の原因分析"
    s.placeholders[1].text = f"{title} / 全{len(faults)}故障 (PI={npi})\nCUBE_TREND 観察ツールによる検証"
    s.shapes.title.text_frame.paragraphs[0].font.color.rgb = ACCENT

    # 2. 背景と仮説
    s = title_only(prs, "背景と検証する仮説")
    bullets(s, [
        ("SATでテストパターンを生成→ドントケア判定でテストキューブ化し、UNSATまで列挙する。",0,False,None),
        ("一部の故障はキューブ生成回数が爆発する（例: b12 のある故障は52万本）。",0,False,None),
        ("",0,False,None),
        ("仮説1: 生成回数が多い故障ほど X（ドントケア）が少ない",0,True,ORANGE),
        ("  → キューブが完全指定に近く、空間を狭くしか覆えない",1,False,None),
        ("仮説2: 生成回数が多い故障ほど X の位置がほぼ同じ（マスク使い回し）で",0,True,GREEN),
        ("  拡大しても新しい空間を広げない（拡大効果がほぼない）",1,True,GREEN),
        ("  → ほぼ同一のキューブを量産し、和集合が飽和せず本数が膨らむ",1,False,None),
    ])

    # 3. 検証方法
    s = title_only(prs, "検証方法（CUBE_TREND 観察ツール）")
    bullets(s, [
        ("本体パイプラインは変えず、env CUBE_TREND=1 で各故障のキューブ列を後から走査。",0,False,None),
        ("",0,False,None),
        ("各キューブについて測る量:",0,True,ACCENT),
        ("X数 … ドントケアの個数（仮説1）",1,False,None),
        ("Xマスク重複率 … 同じX位置パターンの使い回し（仮説2）",1,False,None),
        ("連続キューブ差分 … same_mask（X位置の一致率）と val_diff（ケア値が違うビット数）",1,False,None),
        ("",0,False,None),
        (f"対象: b12_C を -limit 1000 で列挙（MDC_NODOM=1 で生成順を保持）。{len(faults)}故障。",0,False,None),
    ])

    BK = "'バケット集計'"; catB = f"{BK}!$A$2:$A${1+nb}"

    # 4. 仮説1: 棒グラフ（Excel ブック埋め込み）
    s = title_only(prs, "仮説1: キューブ数が多い帯ほど X% は下がるか")
    gf = bar_by_bucket(s, bks, "xpct", "平均X%", ORANGE)
    embed_xlsx(gf.chart, [f"{BK}!$D$1", catB, f"{BK}!$D$2:$D${1+nb}"])
    bullets(s, [
        ("平均X%（縦） vs キューブ数の帯（横）",0,True,None),
        ("",0,False,None),
        (f"{bks[0]['xpct']:.1f}% → {bks[-1]['xpct']:.1f}% へ単調減少。",0,True,ORANGE),
        ("確かに下がるが下げ幅は緩やか",1,False,None),
        ("（~7pt）。少数キューブ列の各",1,False,None),
        ("キューブも X はまだ多い。",1,False,None),
        ("",0,False,None),
        ("→ 仮説1は「弱〜中程度」で支持。",0,True,None),
    ], left=Inches(6.3), top=Inches(1.7), width=Inches(3.4), size=16)

    # 5. 仮説2: 棒グラフ（Excel ブック埋め込み）
    s = title_only(prs, "仮説2: キューブ数が多い帯ほどマスク重複は上がるか")
    gf = bar_by_bucket(s, bks, "reuse", "マスク重複%", GREEN)
    embed_xlsx(gf.chart, [f"{BK}!$E$1", catB, f"{BK}!$E$2:$E${1+nb}"])
    bullets(s, [
        ("マスク重複%（縦） vs キューブ数の帯（横）",0,True,None),
        ("",0,False,None),
        (f"{bks[0]['reuse']:.1f}% → {bks[-1]['reuse']:.1f}% へ急増。",0,True,GREEN),
        ("キューブ数が増えるほど同じX",1,False,None),
        ("位置パターンを使い回している。",1,False,None),
        ("",0,False,None),
        (f"連続キューブの一致率 same_mask",0,False,None),
        (f"は全帯で約{statistics.mean(b['same'] for b in bks):.0f}%、val_diff≈"
         f"{statistics.mean(b['vdiff'] for b in bks):.1f}ビット。",0,False,None),
        ("→ 仮説2は明確に支持。",0,True,GREEN),
    ], left=Inches(6.3), top=Inches(1.7), width=Inches(3.4), size=16)

    # 6. 重故障の生成順推移（折れ線）
    detail = defaultdict(list)
    with open(csv_path, newline="") as fh:
        for row in csv.DictReader(fh):
            detail[(row["fault"],row["f_type"])].append(row)
    heavy = max(detail, key=lambda k: len(detail[k]))
    rows = sorted(detail[heavy], key=lambda r:int(r["idx"]))
    # xlsx の「重故障の推移」シート全行(A=idx,B=x_count,E=val_diff)を参照するため全点を入れる
    cd = CategoryChartData()
    cd.categories = [int(r["idx"]) for r in rows]
    cd.add_series("X数(x_count)", [int(r["x_count"]) for r in rows])
    cd.add_series("連続キューブの相違ビット数(val_diff)",
                  [0 if r["val_diff_vs_prev"]=="-1" else int(r["val_diff_vs_prev"]) for r in rows])
    s = title_only(prs, "最重故障の生成順推移（ほぼ同一キューブの量産）")
    gf = s.shapes.add_chart(XL_CHART_TYPE.LINE, Inches(0.5), Inches(1.6),
                            Inches(6.2), Inches(4.6), cd)
    gf.chart.has_legend = True; gf.chart.legend.position = XL_LEGEND_POSITION.BOTTOM
    gf.chart.legend.include_in_layout = False
    HV = "'重故障の推移'"; lastH = 3 + len(rows); catH = f"{HV}!$A$4:$A${lastH}"
    embed_xlsx(gf.chart, [f"{HV}!$B$3", catH, f"{HV}!$B$4:$B${lastH}",
                          f"{HV}!$E$3", catH, f"{HV}!$E$4:$E${lastH}"])
    bullets(s, [
        (f"故障: {heavy[0]}",0,True,None),
        (f"({heavy[1]}, {len(rows)}本で打ち切り)",1,False,None),
        ("",0,False,None),
        ("初手から X=124/126（ケア2本）。",0,False,None),
        ("以降 X は ~100 で頭打ち。",0,False,None),
        ("",0,False,None),
        ("val_diff はほぼ 0 = ケア値は",0,True,GREEN),
        ("変えずXマスクを数ビット",1,True,GREEN),
        ("ずらすだけのほぼ同一キューブ。",1,True,GREEN),
    ], left=Inches(6.9), top=Inches(1.7), width=Inches(2.9), size=15)

    # 7. 追加観察（当初2仮説の外）— 相関と val_diff 分布
    logn = [math.log10(f["n"]) for f in faults]
    def pear(xs, ys):
        n=len(xs); mx=sum(xs)/n; my=sum(ys)/n
        sx=math.sqrt(sum((x-mx)**2 for x in xs)); sy=math.sqrt(sum((y-my)**2 for y in ys))
        return sum((a-mx)*(b-my) for a,b in zip(xs,ys))/(sx*sy) if sx and sy else float('nan')
    r_x = pear(logn,[f["xpct"] for f in faults])
    # capped 故障の head->tail
    cap=[f for f in faults if f["capped"]]
    tail_up = 100*sum(1 for f in cap if f["xtail"]>f["xhead"])/len(cap) if cap else 0
    # val_diff 分布
    vc=Counter(); tot=0
    for rows in detail.values():
        for rr in sorted(rows,key=lambda r:int(r["idx"])):
            vd=rr["val_diff_vs_prev"]
            if vd=="-1": continue
            vc[min(int(vd),3)]+=1; tot+=1
    careall=[int(rr["care_count"]) for rows in detail.values() for rr in rows]
    care_mean=statistics.mean(careall)

    s = title_only(prs, "追加観察① 当初2仮説の外で分かったこと")
    bullets(s, [
        ("X低下は『故障内』では起きない（仮説1の解釈修正）",0,True,ORANGE),
        (f"故障横断: log(キューブ数) と X% の相関 r={r_x:.2f}（多いほどXが少ない）",1,False,None),
        (f"だが爆発故障の中では X は減らない: capped故障の {tail_up:.0f}% で末尾>先頭",1,False,None),
        ("→ 『Xが少ない故障ほど元々本数が多い』選別効果。原因ではなく相関。",1,True,None),
        ("",0,False,None),
        (f"各キューブが指定する入力は平均わずか {care_mean:.0f}/{npi} 本（低次元）",0,True,ACCENT),
        ("ほぼ全部Xの“near-fully-X”キューブは全体の4%だけ。",1,False,None),
        ("同じ低次元部分空間を多数の低次元キューブで刻むため本数が膨らむ。",1,False,None),
    ], top=Inches(1.3), height=Inches(2.7))
    # val_diff 分布バー
    cd = CategoryChartData()
    cd.categories = ["0","1","2","3+"]
    cd.add_series("割合%", [round(100*vc[k]/tot,1) for k in [0,1,2,3]])
    gf = s.shapes.add_chart(XL_CHART_TYPE.COLUMN_CLUSTERED,
        Inches(0.6), Inches(4.2), Inches(4.4), Inches(3.0), cd)
    gf.chart.has_legend=False
    pl=gf.chart.plots[0]; pl.has_data_labels=True; pl.data_labels.number_format='0.0'
    pl.data_labels.number_format_is_linked=False; pl.data_labels.font.size=Pt(11)
    pl.series[0].format.fill.solid(); pl.series[0].format.fill.fore_color.rgb=GREEN
    VD="'val_diff分布'"
    embed_xlsx(gf.chart, [f"{VD}!$B$1", f"{VD}!$A$2:$A$5", f"{VD}!$B$2:$B$5"])
    tb=s.shapes.add_textbox(Inches(5.2),Inches(4.3),Inches(4.4),Inches(2.8)).text_frame
    tb.word_wrap=True
    p=tb.paragraphs[0]; p.text="連続キューブの相違ビット数(val_diff)の分布"; p.font.bold=True; p.font.size=Pt(15)
    for t in [f"val_diff=0 が {100*vc[0]/tot:.0f}%",
              "＝ケア値を1ビットも変えず、",
              "Xにする位置だけをずらした“ほぼ複製”。",
              "仮説2を最も直接的に裏付ける。"]:
        q=tb.add_paragraph(); q.text=t; q.font.size=Pt(14)
        if "0 が" in t: q.font.color.rgb=GREEN; q.font.bold=True

    # 8. 具体例（ビット列グリッド）
    s = title_only(prs, "追加観察② 具体例: ケア値はそのまま, X窓だけがずれる")
    try:
        ex = json.load(open("ex_window.json"))
    except Exception:
        ex = None
    if ex:
        cap = s.shapes.add_textbox(Inches(0.4),Inches(1.15),Inches(9.2),Inches(0.5)).text_frame
        cap.word_wrap=True
        p=cap.paragraphs[0]
        p.text=f"故障 {ex['fault']} の連続キューブ idx {ex['start']}–{ex['start']+len(ex['rows'])-1}（ケアが現れる列のみ抜粋, 他列は全てX）"
        p.font.size=Pt(13)
        bit_grid(s, ex, Inches(0.4), Inches(1.75))
        ncol=len(ex["cols"])
        leg=s.shapes.add_textbox(Inches(0.4),Inches(1.78)+Inches(0.26)*(len(ex["rows"])+1)+Inches(0.15),
                                 Inches(9.2),Inches(2.2)).text_frame
        leg.word_wrap=True
        items=[("青=0 / 橙=1 / 灰(·)=X(ドントケア)",True,ACCENT),
               (f"連続 val_diff = {ex['vds']}  … ケア値の変化はほぼ無い",True,GREEN),
               ("縦に揃った色 = 不動の『コア割当』（毎回同じ入力を同じ値に固定）",False,None),
               ("端の灰⇔色のちらつき = ドントケア窓が1～数ビット動いているだけ",False,None),
               ("→ 新しい行(キューブ)が増えても覆う空間はほとんど広がらない＝拡大効果がほぼ無い",True,None)]
        for i,(t,b,c) in enumerate(items):
            p=leg.paragraphs[0] if i==0 else leg.add_paragraph()
            p.text="・"+t; p.font.size=Pt(13); p.font.bold=b
            if c: p.font.color.rgb=c

    # 9. 結論
    s = title_only(prs, "結論")
    bullets(s, [
        ("仮説2（X位置がほぼ同じ・拡大効果がほぼない）: 強く支持",0,True,GREEN),
        ("マスク重複率はキューブ数とともに単調増。連続キューブはX位置95%以上一致・",1,False,None),
        ("ケア値はほぼ不変。新キューブが空間をほとんど広げず和集合が飽和しない。",1,False,None),
        ("",0,False,None),
        ("仮説1（Xが極端に少ない）: 弱〜中程度に支持",0,True,ORANGE),
        ("平均X%は確かに下がる（94.5→87.0%）が「極端に少ない」とは言えない。",1,False,None),
        ("",0,False,None),
        ("⇒ 生成回数爆発の主因は『X不足』より",0,True,ACCENT),
        ("   『ほぼ同一の near-duplicate キューブの量産（拡大効果の欠如）』",0,True,ACCENT),
        ("",0,False,None),
        ("詳細データ: b12_cube_trend.xlsx / verification/cube_trend/SUMMARY.md",0,False,None),
    ])

    prs.save(out_path)
    print(f"wrote {out_path}: {len(prs.slides._sldIdLst)} slides, heavy={heavy}")

if __name__ == "__main__":
    main()
