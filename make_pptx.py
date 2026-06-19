"""
FDP 結果ファイル説明資料を生成する。
"""
from pptx import Presentation
from pptx.util import Inches, Pt, Emu
from pptx.dml.color import RGBColor
from pptx.enum.text import PP_ALIGN
from pptx.util import Inches, Pt
import copy

# ---- 色定義 ----
BLUE_DARK  = RGBColor(0x1F, 0x49, 0x7D)   # タイトル背景
BLUE_MID   = RGBColor(0x2E, 0x74, 0xB5)   # アクセント
BLUE_LIGHT = RGBColor(0xD6, 0xE4, 0xF7)   # 薄背景
GRAY_LIGHT = RGBColor(0xF2, 0xF2, 0xF2)   # テーブル偶数行
WHITE      = RGBColor(0xFF, 0xFF, 0xFF)
BLACK      = RGBColor(0x00, 0x00, 0x00)
GRAY_TEXT  = RGBColor(0x40, 0x40, 0x40)
GREEN      = RGBColor(0x37, 0x86, 0x54)
ORANGE     = RGBColor(0xC5, 0x5A, 0x11)

prs = Presentation()
prs.slide_width  = Inches(13.33)
prs.slide_height = Inches(7.5)

BLANK = prs.slide_layouts[6]   # 完全ブランク


# ===========================================================
# ヘルパー
# ===========================================================
def add_rect(slide, x, y, w, h, fill=None, line=None):
    from pptx.util import Pt
    shape = slide.shapes.add_shape(1, Inches(x), Inches(y), Inches(w), Inches(h))
    shape.line.fill.background()
    if fill:
        shape.fill.solid()
        shape.fill.fore_color.rgb = fill
    else:
        shape.fill.background()
    if line:
        shape.line.color.rgb = line
        shape.line.width = Pt(1)
    else:
        shape.line.fill.background()
    return shape


def add_text(slide, text, x, y, w, h,
             font_size=18, bold=False, color=BLACK,
             align=PP_ALIGN.LEFT, wrap=True, italic=False):
    txb = slide.shapes.add_textbox(Inches(x), Inches(y), Inches(w), Inches(h))
    txb.word_wrap = wrap
    tf = txb.text_frame
    tf.word_wrap = wrap
    p = tf.paragraphs[0]
    p.alignment = align
    run = p.add_run()
    run.text = text
    run.font.size = Pt(font_size)
    run.font.bold = bold
    run.font.italic = italic
    run.font.color.rgb = color
    return txb


def title_slide_header(slide, title_text, sub_text=""):
    add_rect(slide, 0, 0, 13.33, 7.5, fill=BLUE_DARK)
    add_text(slide, title_text,
             0.5, 2.5, 12.33, 1.5,
             font_size=40, bold=True, color=WHITE, align=PP_ALIGN.CENTER)
    if sub_text:
        add_text(slide, sub_text,
                 0.5, 4.3, 12.33, 0.8,
                 font_size=22, color=RGBColor(0xBF, 0xD7, 0xED), align=PP_ALIGN.CENTER)


def content_header(slide, title_text):
    add_rect(slide, 0, 0, 13.33, 1.0, fill=BLUE_DARK)
    add_text(slide, title_text,
             0.3, 0.1, 12.73, 0.8,
             font_size=24, bold=True, color=WHITE)
    add_rect(slide, 0, 1.0, 13.33, 0.04, fill=BLUE_MID)


def add_bullet(slide, items, x, y, w, font_size=16, color=GRAY_TEXT, indent=""):
    """items: list of (text, level)"""
    txb = slide.shapes.add_textbox(Inches(x), Inches(y), Inches(w), Inches(6))
    txb.word_wrap = True
    tf = txb.text_frame
    tf.word_wrap = True
    first = True
    for text, level in items:
        if first:
            p = tf.paragraphs[0]
            first = False
        else:
            p = tf.add_paragraph()
        p.level = level
        prefix = "・" if level == 0 else "　－"
        run = p.add_run()
        run.text = prefix + " " + text
        run.font.size = Pt(font_size)
        run.font.color.rgb = color
        from pptx.util import Pt as Pt2
        p.space_before = Pt2(4)
    return txb


# ===========================================================
# スライド1: タイトル
# ===========================================================
slide = prs.slides.add_slide(BLANK)
title_slide_header(slide,
    "FDP 結果ファイル 説明資料",
    "故障検出確率 (Fault Detection Probability) 計算結果")
add_text(slide, "― ITC'99 / ISCAS ベンチマーク回路 ―",
         0.5, 5.3, 12.33, 0.6,
         font_size=16, color=RGBColor(0x9D, 0xC3, 0xE6), align=PP_ALIGN.CENTER)


# ===========================================================
# スライド2: 概要
# ===========================================================
slide = prs.slides.add_slide(BLANK)
add_rect(slide, 0, 0, 13.33, 7.5, fill=WHITE)
content_header(slide, "概要")

add_text(slide, "何のデータか",
         0.4, 1.2, 12.0, 0.4,
         font_size=18, bold=True, color=BLUE_MID)

add_bullet(slide, [
    ("論理回路の各信号線（ネット）に対して、縮退故障（Stuck-At Fault）を仮定したときの「故障検出確率」を計算した結果", 0),
    ("縮退故障とは：信号線が常に 0（sa0）または常に 1（sa1）に固着した故障モデル", 1),
    ("故障検出確率（FDP）とは：ランダムな入力パターンを与えたとき、その故障が検出される確率", 1),
], 0.4, 1.7, 12.5, font_size=15)

add_text(slide, "計算手法",
         0.4, 3.6, 12.0, 0.4,
         font_size=18, bold=True, color=BLUE_MID)

add_bullet(slide, [
    ("SAT ソルバ（CaDiCaL）でテストキューブ（検出パターン）を列挙", 0),
    ("列挙したキューブの和集合を BDD（Binary Decision Diagram）で表現し、厳密な確率を計算", 0),
    ("ドントケアビットは含意操作（XID）で最大化 → 各キューブの被覆領域を拡大", 0),
    ("計算モード: -limit 30（故障ごとのキューブ上限 30 本）", 0),
], 0.4, 4.1, 12.5, font_size=15)


# ===========================================================
# スライド3: ファイル一覧
# ===========================================================
slide = prs.slides.add_slide(BLANK)
add_rect(slide, 0, 0, 13.33, 7.5, fill=WHITE)
content_header(slide, "ファイル一覧")

add_text(slide, "output/limit30/fdp/ フォルダに回路ごとの CSV が格納されています",
         0.4, 1.1, 12.5, 0.5,
         font_size=16, color=GRAY_TEXT)

# テーブル
headers = ["ファイル名", "回路系列", "故障数（行数）", "備考"]
rows_data = [
    ["b05_C.csv",    "ITC'99 b05",  "4,498",  ""],
    ["b07_C.csv",    "ITC'99 b07",  "2,178",  ""],
    ["b11_C.csv",    "ITC'99 b11",  "—",      "既存結果"],
    ["b12_C.csv",    "ITC'99 b12",  "—",      ""],
    ["b14_C.csv〜",  "ITC'99 b14,15,20,21,22",  "—",  "大規模回路"],
    ["s5378_C.csv〜","ISCAS s5378 ほか",  "—",  ""],
]

col_w = [2.8, 2.5, 2.2, 4.5]
col_x = [0.4, 3.3, 5.9, 8.2]
row_h = 0.45
start_y = 1.7

# ヘッダー行
for ci, (hdr, cx, cw) in enumerate(zip(headers, col_x, col_w)):
    add_rect(slide, cx, start_y, cw, row_h, fill=BLUE_MID)
    add_text(slide, hdr, cx+0.05, start_y+0.05, cw-0.1, row_h-0.05,
             font_size=13, bold=True, color=WHITE)

for ri, row in enumerate(rows_data):
    y = start_y + row_h * (ri + 1)
    bg = GRAY_LIGHT if ri % 2 == 0 else WHITE
    for ci, (cell, cx, cw) in enumerate(zip(row, col_x, col_w)):
        add_rect(slide, cx, y, cw, row_h, fill=bg, line=RGBColor(0xCC, 0xCC, 0xCC))
        add_text(slide, cell, cx+0.05, y+0.05, cw-0.1, row_h-0.05,
                 font_size=13, color=GRAY_TEXT)

add_text(slide, "※ ファイル名は <回路名>.csv 形式。fdp 列の降順でソート済み。",
         0.4, 7.1, 12.5, 0.35, font_size=12,
         color=RGBColor(0x80, 0x80, 0x80), italic=True)


# ===========================================================
# スライド4: 列の説明（メイン）
# ===========================================================
slide = prs.slides.add_slide(BLANK)
add_rect(slide, 0, 0, 13.33, 7.5, fill=WHITE)
content_header(slide, "CSV 列の説明")

add_text(slide, "各 CSV は以下の 5 列で構成されます",
         0.4, 1.1, 12.5, 0.4, font_size=16, color=GRAY_TEXT)

# カード形式で各列を説明
cards = [
    ("net_name",  BLUE_MID,   "信号線名",
     "回路内の信号線（ネット）の識別子。\n通常はゲート名や内部信号名（例: U591, COUNT_REG_0__SCAN_IN）。"),
    ("f_type",    BLUE_MID,   "故障種別",
     "sa0（stuck-at-0）または sa1（stuck-at-1）。\n同じ信号線でも sa0 と sa1 は別の故障として扱う。"),
    ("cube_cnt",  BLUE_MID,   "テストキューブ数",
     "この故障を検出するために列挙したキューブ（テストパターン）の本数。\n空欄 = 等価故障・支配故障（後述）。"),
    ("complete",  GREEN,      "列挙完了フラグ",
     "1 = UNSAT まで完全列挙（厳密な FDP）。\n0 = キューブ上限（limit=30）到達（近似値）。\n空欄 = 等価故障・支配故障。"),
    ("fdp",       ORANGE,     "故障検出確率",
     "ランダムな入力パターンで故障が検出される確率（0〜1）。\nBDD を用いた厳密計算。0 = 冗長故障（検出不能）。"),
]

card_w = 2.4
card_h = 2.0
start_x = 0.35
gap = 0.27

for i, (col, color, label, desc) in enumerate(cards):
    x = start_x + i * (card_w + gap)
    y = 1.65
    add_rect(slide, x, y, card_w, 0.38, fill=color)
    add_text(slide, col, x+0.08, y+0.04, card_w-0.1, 0.32,
             font_size=14, bold=True, color=WHITE)
    add_rect(slide, x, y+0.38, card_w, 0.28, fill=BLUE_LIGHT)
    add_text(slide, label, x+0.08, y+0.38, card_w-0.1, 0.28,
             font_size=13, bold=True, color=BLUE_DARK)
    add_rect(slide, x, y+0.66, card_w, card_h-0.66, fill=GRAY_LIGHT,
             line=RGBColor(0xCC, 0xCC, 0xCC))
    add_text(slide, desc, x+0.08, y+0.7, card_w-0.15, card_h-0.7,
             font_size=12, color=GRAY_TEXT)

# 凡例
add_rect(slide, 0.35, 4.0, 12.6, 0.04, fill=BLUE_LIGHT)
add_text(slide, "サンプル行",
         0.35, 4.15, 3.0, 0.3, font_size=13, bold=True, color=BLUE_MID)

# サンプルテーブル
sample = [
    ["net_name", "f_type", "cube_cnt", "complete", "fdp"],
    ["U591",     "sa1",    "9",        "1",         "9.2187500000e-01"],
    ["U885",     "sa0",    "",         "",          "8.0664062500e-01"],
    ["some_net", "sa0",    "30",       "0",         "1.2345678900e-02"],
    ["redundant","sa1",    "0",        "1",         "0.0000000000e+00"],
]
sc = [1.5, 1.5, 1.5, 1.5, 2.5]
sx = [0.35, 1.9, 3.45, 5.0, 6.55]
sy = 4.5
sh = 0.37
for ri, row in enumerate(sample):
    bg = BLUE_MID if ri == 0 else (GRAY_LIGHT if ri % 2 == 1 else WHITE)
    tc = WHITE if ri == 0 else GRAY_TEXT
    for ci, (cell, cx, cw) in enumerate(zip(row, sx, sc)):
        add_rect(slide, cx, sy + ri*sh, cw, sh, fill=bg,
                 line=RGBColor(0xCC, 0xCC, 0xCC))
        add_text(slide, cell, cx+0.05, sy+ri*sh+0.04, cw-0.08, sh-0.05,
                 font_size=11, bold=(ri==0), color=tc)

# 注釈
add_text(slide, "← 完全列挙 (FDP 厳密値)    ← 等価/支配故障（cube_cnt 空欄）    ← 上限到達 (FDP 近似値)    ← 冗長故障 (FDP=0)",
         9.2, 4.65, 3.9, 2.5, font_size=10, color=GRAY_TEXT)


# ===========================================================
# スライド5: 等価故障・支配故障の説明
# ===========================================================
slide = prs.slides.add_slide(BLANK)
add_rect(slide, 0, 0, 13.33, 7.5, fill=WHITE)
content_header(slide, "等価故障・支配故障（cube_cnt が空欄の行）")

add_text(slide,
    "cube_cnt と complete が空欄の行は、計算を省略した故障です。省略しても FDP は正確に求まります。",
    0.4, 1.1, 12.5, 0.5, font_size=15, color=GRAY_TEXT)

# 等価故障
add_rect(slide, 0.4, 1.75, 5.8, 2.5, fill=BLUE_LIGHT, line=BLUE_MID)
add_text(slide, "等価故障（Equivalent Fault）",
         0.55, 1.82, 5.5, 0.4, font_size=15, bold=True, color=BLUE_DARK)
add_bullet(slide, [
    ("テスト集合がまったく同じ故障", 0),
    ("例: 2入力 AND ゲートの出力 sa0 は、両入力の sa1 と等価になることがある", 0),
    ("代表故障 1 つだけ計算し、残りはその FDP をコピー", 0),
    ("cube_cnt = 空欄（独自に列挙していない）", 0),
], 0.55, 2.3, 5.5, font_size=14)

# 支配故障
add_rect(slide, 6.8, 1.75, 6.1, 2.5, fill=RGBColor(0xE2, 0xF0, 0xD9), line=GREEN)
add_text(slide, "支配故障（Dominated Fault）",
         6.95, 1.82, 5.8, 0.4, font_size=15, bold=True, color=GREEN)
add_bullet(slide, [
    ("故障 A のテスト集合 ⊆ 故障 B のテスト集合 → A は B に支配される", 0),
    ("A のキューブを B の計算で再利用することで計算コストを削減", 0),
    ("再利用分の cube_cnt は空欄（重複カウントしない）", 0),
    ("FDP は正確に計算される", 0),
], 6.95, 2.3, 5.8, font_size=14)

add_text(slide, "FDP の解釈",
         0.4, 4.45, 12.0, 0.4, font_size=18, bold=True, color=BLUE_MID)

# FDPの値の解釈ボックス
fdp_items = [
    ("fdp = 1.0",  "理論上すべての入力で検出可能（実際にはまれ）", BLUE_MID),
    ("fdp = 0.0",  "冗長故障（いかなる入力でも検出不能）",          ORANGE),
    ("0 < fdp < 1","多くの故障はこの範囲。値が高いほど検出されやすい", GREEN),
    ("complete=0", "キューブ上限（30本）到達 → FDP は下限値（近似）", RGBColor(0x80, 0x60, 0x00)),
]
bx = [0.4, 3.5, 6.6, 9.7]
for i, (val, desc, color) in enumerate(fdp_items):
    x = bx[i]
    add_rect(slide, x, 4.95, 3.0, 0.45, fill=color)
    add_text(slide, val, x+0.08, 4.97, 2.85, 0.4,
             font_size=14, bold=True, color=WHITE)
    add_rect(slide, x, 5.4, 3.0, 1.0, fill=GRAY_LIGHT, line=RGBColor(0xCC,0xCC,0xCC))
    add_text(slide, desc, x+0.08, 5.45, 2.85, 0.95,
             font_size=13, color=GRAY_TEXT)


# ===========================================================
# スライド6: complete フラグの意味
# ===========================================================
slide = prs.slides.add_slide(BLANK)
add_rect(slide, 0, 0, 13.33, 7.5, fill=WHITE)
content_header(slide, "complete フラグと FDP の精度")

add_text(slide,
    "complete 列は、その故障の FDP が「厳密値」か「下限近似値」かを示します。",
    0.4, 1.1, 12.5, 0.4, font_size=16, color=GRAY_TEXT)

# complete=1
add_rect(slide, 0.4, 1.65, 5.9, 3.5, fill=RGBColor(0xE2, 0xF0, 0xD9), line=GREEN)
add_rect(slide, 0.4, 1.65, 5.9, 0.5, fill=GREEN)
add_text(slide, "complete = 1（厳密値）",
         0.55, 1.7, 5.7, 0.4, font_size=16, bold=True, color=WHITE)
add_bullet(slide, [
    ("SAT ソルバが UNSAT を返した = これ以上テストキューブは存在しない", 0),
    ("キューブの和集合 = 故障の検出集合（完全）", 0),
    ("BDD で算出した FDP は厳密な確率値", 0),
    ("cube_cnt が少なくても精度は保証される", 0),
    ("（例）cube_cnt=9 で complete=1 → 9本のキューブで完全網羅", 0),
], 0.55, 2.25, 5.7, font_size=14)

# complete=0
add_rect(slide, 7.0, 1.65, 5.9, 3.5, fill=RGBColor(0xFD, 0xF0, 0xE5), line=ORANGE)
add_rect(slide, 7.0, 1.65, 5.9, 0.5, fill=ORANGE)
add_text(slide, "complete = 0（上限到達・近似値）",
         7.15, 1.7, 5.7, 0.4, font_size=16, bold=True, color=WHITE)
add_bullet(slide, [
    ("キューブ上限（-limit 30）に達した = 探索を打ち切り", 0),
    ("まだ検出できるパターンが存在する可能性がある", 0),
    ("出力 FDP は下限値（実際の FDP ≥ 出力値）", 0),
    ("大規模回路・複雑な故障で発生しやすい", 0),
    ("より正確な値を得るには -limit 値を増やすか完全列挙が必要", 0),
], 7.15, 2.25, 5.7, font_size=14)

add_text(slide, "ソート順について",
         0.4, 5.35, 12.0, 0.4, font_size=18, bold=True, color=BLUE_MID)
add_bullet(slide, [
    ("各 CSV は fdp 列の降順でソート済み → 先頭が最も検出されやすい故障、末尾が冗長故障（fdp=0）", 0),
    ("complete=0 の故障は FDP が過小評価されている可能性があるため、上位行との比較には注意が必要", 0),
], 0.4, 5.85, 12.5, font_size=15)


# ===========================================================
# スライド7: まとめ・利用方法
# ===========================================================
slide = prs.slides.add_slide(BLANK)
add_rect(slide, 0, 0, 13.33, 7.5, fill=WHITE)
content_header(slide, "まとめ・データの利用方法")

# 左カラム: クイックリファレンス
add_rect(slide, 0.4, 1.15, 5.8, 0.4, fill=BLUE_MID)
add_text(slide, "列クイックリファレンス",
         0.5, 1.18, 5.6, 0.35, font_size=13, bold=True, color=WHITE)

ref = [
    ("net_name", "信号線名（回路内の識別子）"),
    ("f_type",   "sa0 / sa1 の別"),
    ("cube_cnt", "使用キューブ数（空欄=等価/支配）"),
    ("complete", "1=厳密値 / 0=下限近似 / 空欄=等価/支配"),
    ("fdp",      "故障検出確率（0〜1、降順ソート済み）"),
]
for i, (col, desc) in enumerate(ref):
    y = 1.55 + i * 0.48
    bg = BLUE_LIGHT if i % 2 == 0 else WHITE
    add_rect(slide, 0.4, y, 5.8, 0.48, fill=bg, line=RGBColor(0xCC,0xCC,0xCC))
    add_text(slide, col,  0.5,  y+0.08, 1.6, 0.35, font_size=13, bold=True,  color=BLUE_DARK)
    add_text(slide, desc, 2.15, y+0.08, 3.9, 0.35, font_size=13, color=GRAY_TEXT)

# 右カラム: 利用例
add_rect(slide, 6.7, 1.15, 6.2, 0.4, fill=BLUE_MID)
add_text(slide, "典型的な利用方法",
         6.8, 1.18, 6.0, 0.35, font_size=13, bold=True, color=WHITE)

add_bullet(slide, [
    ("テスト容易性の評価", 0),
    ("FDP が低い故障 → テスト生成が難しい / 回路構造上検出しにくい", 1),
    ("FDP が高い故障 → ランダムテストでも検出可能", 1),
    ("冗長故障の特定", 0),
    ("fdp=0 かつ complete=1 の行 → 設計上不要なロジックの可能性", 1),
    ("complete=0 の故障をフォローアップ", 0),
    ("より多くのキューブ（limit を増やす）で精度向上が見込める故障を特定", 1),
    ("回路間の比較", 0),
    ("FDP の分布（平均・中央値・最小値）を回路ごとに比較し、テスト難易度を評価", 1),
], 6.8, 1.65, 6.0, font_size=14)

add_text(slide, "注意事項",
         0.4, 6.1, 12.0, 0.35, font_size=14, bold=True, color=ORANGE)
add_text(slide,
    "FDP は -limit 30 での計算結果です。complete=0 の行は下限値であり、実際の FDP はそれ以上の可能性があります。"
    "また、ドントケアビット最大化（XID）を適用しているため、通常のランダムテスト確率より高い値になります。",
    0.4, 6.5, 12.5, 0.8, font_size=13, color=GRAY_TEXT)


# ===========================================================
# 保存
# ===========================================================
out = "/home/koizumi/fdp/FDP結果ファイル説明.pptx"
prs.save(out)
print("Saved:", out)
