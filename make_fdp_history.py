#!/usr/bin/env python3
"""FDPツール 開発・改善記録 PowerPoint生成スクリプト"""

from pptx import Presentation
from pptx.util import Inches, Pt
from pptx.dml.color import RGBColor
from pptx.enum.text import PP_ALIGN

# ─────────────────── カラーパレット ───────────────────
BG       = RGBColor(0x1a, 0x1a, 0x2e)
PANEL    = RGBColor(0x16, 0x21, 0x3e)
ACCENT1  = RGBColor(0x0f, 0x3d, 0x84)
ACCENT2  = RGBColor(0xe9, 0x45, 0x60)
ACCENT3  = RGBColor(0x53, 0xc0, 0x77)
ACCENT4  = RGBColor(0xf5, 0xa6, 0x23)
WHITE    = RGBColor(0xff, 0xff, 0xff)
GRAY     = RGBColor(0xaa, 0xaa, 0xcc)
LBLUE    = RGBColor(0x6b, 0xb5, 0xff)
PURPLE   = RGBColor(0x8b, 0x2f, 0x97)
TEAL     = RGBColor(0x0a, 0x7a, 0x6e)
ORANGE2  = RGBColor(0xc0, 0x50, 0x20)
DKBLUE   = RGBColor(0x22, 0x60, 0x9a)

SLIDE_W = Inches(13.33)
SLIDE_H = Inches(7.5)

prs = Presentation()
prs.slide_width  = SLIDE_W
prs.slide_height = SLIDE_H

def blank_slide(prs):
    return prs.slides.add_slide(prs.slide_layouts[6])

def bg_rect(slide, color=None):
    c = color or BG
    sh = slide.shapes.add_shape(1, 0, 0, SLIDE_W, SLIDE_H)
    sh.fill.solid()
    sh.fill.fore_color.rgb = c
    sh.line.fill.background()

def header_bar(slide, title, subtitle="", bar_color=None):
    bc = bar_color or ACCENT1
    sh = slide.shapes.add_shape(1, 0, 0, SLIDE_W, Inches(1.0))
    sh.fill.solid()
    sh.fill.fore_color.rgb = bc
    sh.line.fill.background()
    add_text(slide, title, Inches(0.3), Inches(0.05), Inches(12), Inches(0.6),
             font_size=26, bold=True, color=WHITE)
    if subtitle:
        add_text(slide, subtitle, Inches(0.4), Inches(0.62), Inches(12), Inches(0.34),
                 font_size=13, color=GRAY)

def add_text(slide, text, x, y, w, h, font_size=14, bold=False, color=None,
             align=PP_ALIGN.LEFT, italic=False):
    c = color or WHITE
    tx = slide.shapes.add_textbox(x, y, w, h)
    tf = tx.text_frame
    tf.word_wrap = True
    p = tf.paragraphs[0]
    p.alignment = align
    run = p.add_run()
    run.text = text
    run.font.size = Pt(font_size)
    run.font.bold = bold
    run.font.italic = italic
    run.font.color.rgb = c

def colored_box(slide, x, y, w, h, fill_color, text="", font_size=12,
                text_color=None, bold=False):
    tc = text_color or WHITE
    sh = slide.shapes.add_shape(1, x, y, w, h)
    sh.fill.solid()
    sh.fill.fore_color.rgb = fill_color
    sh.line.fill.background()
    if text:
        tf = sh.text_frame
        tf.word_wrap = True
        p = tf.paragraphs[0]
        p.alignment = PP_ALIGN.CENTER
        run = p.add_run()
        run.text = text
        run.font.size = Pt(font_size)
        run.font.bold = bold
        run.font.color.rgb = tc

def make_card(slide, x, y, w, h, title, title_color, lines, font_size=11):
    title_h = Inches(0.38)
    colored_box(slide, x, y, w, title_h, title_color, text=title, font_size=13, bold=True)
    # 本文背景
    sh = slide.shapes.add_shape(1, x, y + title_h, w, h - title_h)
    sh.fill.solid()
    sh.fill.fore_color.rgb = PANEL
    sh.line.color.rgb = title_color
    sh.line.width = Pt(1.5)
    # テキスト
    tx = slide.shapes.add_textbox(x + Inches(0.1), y + title_h + Inches(0.1),
                                   w - Inches(0.2), h - title_h - Inches(0.15))
    tf = tx.text_frame
    tf.word_wrap = True
    for i, line in enumerate(lines):
        p = tf.paragraphs[0] if i == 0 else tf.add_paragraph()
        run = p.add_run()
        run.text = line
        run.font.size = Pt(font_size)
        if line.startswith("✔"):
            run.font.color.rgb = ACCENT3
            run.font.bold = True
        elif line.startswith("✘"):
            run.font.color.rgb = ACCENT2
            run.font.bold = True
        elif line.startswith("◆"):
            run.font.color.rgb = ACCENT4
            run.font.bold = True
        elif line.startswith("▶"):
            run.font.color.rgb = LBLUE
            run.font.bold = True
        elif line.startswith("症状"):
            run.font.color.rgb = ACCENT2
            run.font.bold = True
        elif line.startswith("原因"):
            run.font.color.rgb = ACCENT4
            run.font.bold = True
        elif line.startswith("修正"):
            run.font.color.rgb = ACCENT3
            run.font.bold = True
        else:
            run.font.color.rgb = WHITE

# ═══════════════════════════════════════════════
# Slide 1: タイトル
# ═══════════════════════════════════════════════
def slide_title(prs):
    sl = blank_slide(prs)
    bg_rect(sl, BG)
    sh = sl.shapes.add_shape(1, 0, Inches(1.9), SLIDE_W, Inches(2.3))
    sh.fill.solid()
    sh.fill.fore_color.rgb = ACCENT1
    sh.line.fill.background()
    add_text(sl, "FDPツール  開発・改善の記録",
             Inches(0.5), Inches(2.05), Inches(12.3), Inches(0.95),
             font_size=38, bold=True, color=WHITE, align=PP_ALIGN.CENTER)
    add_text(sl, "Fault Detection Probability — SAT + BDD + XID パイプライン",
             Inches(0.5), Inches(3.05), Inches(12.3), Inches(0.5),
             font_size=18, color=GRAY, align=PP_ALIGN.CENTER)
    add_text(sl, "動機  ▶  手法  ▶  結果 を時系列でまとめた技術スライド",
             Inches(0.5), Inches(4.0), Inches(12.3), Inches(0.5),
             font_size=15, color=LBLUE, align=PP_ALIGN.CENTER, italic=True)
    add_text(sl, "実験ブランチ: experiment/maxdc-headroom  /  2025〜2026年",
             Inches(0.5), Inches(6.6), Inches(12.3), Inches(0.4),
             font_size=12, color=GRAY, align=PP_ALIGN.CENTER)

# ═══════════════════════════════════════════════
# Slide 2: タイムライン
# ═══════════════════════════════════════════════
def slide_timeline(prs):
    sl = blank_slide(prs)
    bg_rect(sl, BG)
    header_bar(sl, "全体タイムライン", "各フェーズの概要と主要成果", ACCENT1)

    phases = [
        ("①", "基本実装",           "SAT+BDD\nFDP計算",          ACCENT1),
        ("②", "等価故障\n支配関係",  "重複計算\n削減",            DKBLUE),
        ("③", "XID\nインライン化",   "約10倍\n高速化",            PURPLE),
        ("④", "検証ツール\nMC+BDD",  "健全性バグ\n発見",          ACCENT4),
        ("⑤", "3バグ\n修正",         "ALL\nVERIFIED",            ACCENT3),
        ("⑥", "CUBE_TREND\n分析",    "爆発原因\n特定",            TEAL),
        ("⑦", "MAXDC\n素項展開",     "cube\n−30〜75%",           ORANGE2),
        ("⑧", "MAXDC_CORE\n改良",    "速度・完成率\n向上",        ACCENT3),
    ]

    box_w = Inches(1.44)
    box_h = Inches(1.5)
    y_top = Inches(1.25)
    gap   = Inches(0.13)
    x0    = Inches(0.25)

    # 矢印ライン
    arrow_y = y_top + box_h / 2
    sh = sl.shapes.add_shape(1, x0 + box_w, arrow_y - Inches(0.03),
                              (box_w + gap) * (len(phases) - 1) - gap, Inches(0.06))
    sh.fill.solid()
    sh.fill.fore_color.rgb = GRAY
    sh.line.fill.background()

    for i, (num, label, result, color) in enumerate(phases):
        x = x0 + i * (box_w + gap)
        colored_box(sl, x, y_top, box_w, Inches(0.75), color,
                    text=label, font_size=11, bold=True)
        # バッジ (円形は shape 9)
        bx = x + box_w/2 - Inches(0.21)
        by = arrow_y - Inches(0.21)
        badge = sl.shapes.add_shape(9, bx, by, Inches(0.42), Inches(0.42))
        badge.fill.solid()
        badge.fill.fore_color.rgb = color
        badge.line.color.rgb = WHITE
        badge.line.width = Pt(1.5)
        tf = badge.text_frame
        p = tf.paragraphs[0]
        p.alignment = PP_ALIGN.CENTER
        r = p.add_run()
        r.text = num
        r.font.size = Pt(10)
        r.font.bold = True
        r.font.color.rgb = WHITE
        # 結果ボックス
        ry = y_top + Inches(0.75) + Inches(0.06)
        rb = sl.shapes.add_shape(1, x, ry, box_w, Inches(0.62))
        rb.fill.solid()
        rb.fill.fore_color.rgb = PANEL
        rb.line.color.rgb = color
        rb.line.width = Pt(1)
        rtf = rb.text_frame
        rp = rtf.paragraphs[0]
        rp.alignment = PP_ALIGN.CENTER
        rr = rp.add_run()
        rr.text = result
        rr.font.size = Pt(10)
        rr.font.color.rgb = LBLUE

    rows = [
        ("Phase 1〜3",  ACCENT1, "基本実装 → 高速化（ツールとして機能する状態に）"),
        ("Phase 4〜5",  ACCENT4, "健全性検証 → バグ根絶（正確さを厳密に証明）"),
        ("Phase 6〜8",  TEAL,    "分析 → 改善実験（cube 削減・高速化・完成率向上）"),
    ]
    for i, (ph, color, desc) in enumerate(rows):
        y = Inches(3.7) + i * Inches(0.5)
        colored_box(sl, Inches(0.3), y, Inches(1.4), Inches(0.36), color,
                    text=ph, font_size=11, bold=True)
        add_text(sl, desc, Inches(1.82), y + Inches(0.04), Inches(10.8), Inches(0.36),
                 font_size=12, color=WHITE)

# ═══════════════════════════════════════════════
# Phase スライド共通レイアウト (3カラム)
# ═══════════════════════════════════════════════
def three_col(slide, items):
    labels  = ["動機", "手法", "結果"]
    colors  = [ACCENT4, LBLUE, ACCENT3]
    x_list  = [Inches(0.25), Inches(4.55), Inches(8.85)]
    col_w   = Inches(4.1)
    y_lbl   = Inches(1.08)
    y_body  = Inches(1.48)
    body_h  = Inches(5.82)

    for label, color, x, lines in zip(labels, colors, x_list, items):
        colored_box(slide, x, y_lbl, col_w, Inches(0.36), color,
                    text=label, font_size=14, bold=True)
        sh = slide.shapes.add_shape(1, x, y_body, col_w, body_h)
        sh.fill.solid()
        sh.fill.fore_color.rgb = PANEL
        sh.line.color.rgb = color
        sh.line.width = Pt(1.5)
        tx = slide.shapes.add_textbox(x + Inches(0.12), y_body + Inches(0.12),
                                       col_w - Inches(0.24), body_h - Inches(0.24))
        tf = tx.text_frame
        tf.word_wrap = True
        for i, line in enumerate(lines):
            p = tf.paragraphs[0] if i == 0 else tf.add_paragraph()
            run = p.add_run()
            run.text = line
            run.font.size = Pt(12)
            if line.startswith("✔"):
                run.font.color.rgb = ACCENT3; run.font.bold = True
            elif line.startswith("✘"):
                run.font.color.rgb = ACCENT2; run.font.bold = True
            elif line.startswith("◆"):
                run.font.color.rgb = ACCENT4; run.font.bold = True
            elif line.startswith("▶"):
                run.font.color.rgb = LBLUE; run.font.bold = True
            else:
                run.font.color.rgb = WHITE

# ═══════════════════════════════════════════════
# Slide 3: Phase 1 基本実装
# ═══════════════════════════════════════════════
def slide_phase1(prs):
    sl = blank_slide(prs)
    bg_rect(sl, BG)
    header_bar(sl, "Phase 1  基本実装",
               "CaDiCaL(SAT) + CUDD(BDD) + GMP(多倍長) でFDP厳密計算", ACCENT1)
    three_col(sl, [
        ["・縮退故障(SAF)の FDP を厳密計算したい",
         "",
         "・ランダムパターン法は近似値しか得られない",
         "",
         "・テストキューブの和集合として",
         "  正確な検出確率を求める方式が必要",
         "",
         "・外部ツール(XID)でドントケアを最大化し",
         "  より大きなキューブで和集合を被覆する"],
        ["▶ CaDiCaL (SAT) でテストキューブ生成",
         "  ・正常回路 + 故障回路 + 検出節 → CNF",
         "  ・SAT → キューブ、禁止節追加 → 繰り返し",
         "  ・UNSAT で完全列挙完了",
         "",
         "▶ XID (外部プロセス) でドントケア埋め",
         "  ・各キューブの X ビットを最大化",
         "  ・故障シミュレーションで判定",
         "",
         "▶ CUDD (BDD) でキューブ和集合構築",
         "  ・GMP 多倍長有理数で厳密確率計算",
         "  ・出力: 故障ごとの fdp を CSV"],
        ["✔ 基本動作確認 (c17a, s27_C 等)",
         "  期待値ファイルと一致",
         "",
         "✔ 完全列挙 (UNSAT 到達) で complete=1",
         "  の故障は理論的に厳密値",
         "",
         "◆ 課題: XID が外部プロセス",
         "  → 1キューブごとにプロセス起動",
         "  → 低速(s5378 で約 620s)",
         "",
         "◆ 課題: 大回路は cube 数が爆発し",
         "  -limit 打ち切りが必要"],
    ])

# ═══════════════════════════════════════════════
# Slide 4: Phase 2 等価故障・支配関係
# ═══════════════════════════════════════════════
def slide_phase2(prs):
    sl = blank_slide(prs)
    bg_rect(sl, BG)
    header_bar(sl, "Phase 2  等価故障解析 + 支配関係による列挙効率化",
               "重複する計算を省き、処理故障数と SAT 呼び出し回数を削減", DKBLUE)
    three_col(sl, [
        ["・回路によって多数の故障が「等価」",
         "  → 同じテスト集合を持つ",
         "  → 代表故障1本だけ列挙すれば十分",
         "",
         "・支配関係:",
         "  ある故障Aの検出集合 T(A) が",
         "  別の故障Bの検出集合の部分集合",
         "  T(A) ⊆ T(B) のとき、",
         "  A のキューブを B の初期解として流用可",
         "",
         "→ SAT は差分 T(B) \\ T(A) のみ探索"],
        ["▶ 等価故障解析",
         "  ・代表故障のみキューブ列挙",
         "  ・等価故障は代表のFDPを共有",
         "  ・CSVに別行として記録",
         "",
         "▶ 支配関係の流用 (SetTarget)",
         "  ・subset_faults のキューブを",
         "    初期禁止節として注入",
         "  ・n_pending 参照カウントで",
         "    最終消費者が解放",
         "",
         "▶ env MDC_NODOM=1 で無効化",
         "  (流用を切った完全列挙との比較用)"],
        ["✔ 等価故障の繰り返し列挙を根絶",
         "",
         "✔ 支配流用で SAT 呼び出し回数を削減",
         "",
         "✔ n_enum (列挙故障数) は手法によらず不変",
         "  等価故障数も不変",
         "  ※ cube 削減は「1故障あたりの本数」のみ",
         "",
         "✔ MDC_NODOM=1 で支配解析の検証が可能"],
    ])

# ═══════════════════════════════════════════════
# Slide 5: Phase 3 XIDインライン化
# ═══════════════════════════════════════════════
def slide_phase3(prs):
    sl = blank_slide(prs)
    bg_rect(sl, BG)
    header_bar(sl, "Phase 3  XIDインライン化（故障値考慮）+ EssentialAssignment",
               "外部プロセス方式を廃止し C 実装に置き換え — 約10倍高速化", PURPLE)
    three_col(sl, [
        ["・旧 XID = 外部プロセス (FaultSim) 呼び出し",
         "  → 1キューブごとにプロセス生成+回路再読込",
         "  → I/O コストが全体時間の大半",
         "",
         "・故障値（D値）を考慮していない",
         "  → ドントケア判定が過保守",
         "  → 本来 X にできるビットが care のまま",
         "",
         "・EssentialAssignment(EA)が未実装",
         "  → 含意が浅く X が少ない"],
        ["▶ XID を C コードで再実装 (src/fdp/xid/)",
         "  ・InlineXID: 同一プロセス内で",
         "    順方向/逆方向含意を実行",
         "  ・故障3値(D/D̄) 考慮の J-frontier",
         "    正当化を実装",
         "",
         "▶ EssentialAssignment (EA)",
         "  (src/fdp/essential_assignment.c)",
         "  ・BFS フロンティアが1本になった",
         "    支配点の側面入力に非制御値を付加",
         "  ・より多くのビットを確定 → X 増加",
         "",
         "▶ スクラッチバッファ再利用で",
         "  アロケーションコスト削減"],
        ["✔ 処理速度: 約10倍高速化",
         "  s5378: 621s → 59s",
         "",
         "✔ X 率はほぼ同等 (±1pt未満)",
         "  s5378: inline 95.06% / ext 95.21%",
         "",
         "◆ 後に判明: 故障値考慮の実装に",
         "  バグ → 過大評価が残存",
         "  (Phase 5 の修正で解消)",
         "",
         "✔ MDC_NOEA=1 で EA 単体切り分け可能"],
    ])

# ═══════════════════════════════════════════════
# Slide 6: Phase 4 検証ツール
# ═══════════════════════════════════════════════
def slide_phase4(prs):
    sl = blank_slide(prs)
    bg_rect(sl, BG)
    header_bar(sl, "Phase 4  健全性検証ツール — Monte-Carlo + GT_BDD",
               "「FDPが本当に正しいか」を独立検証する手段を整備 (2026-06-10〜13)", ACCENT4)
    three_col(sl, [
        ["・中規模回路 (s298, s641 等) で",
         "  baseline の FDP が手法間で食い違う",
         "",
         "・小回路(c17a)は全故障で一致するが",
         "  中規模になると不一致が発生",
         "",
         "→ XID/EA/SAT パイプラインのどこかに",
         "  健全性バグがある可能性",
         "",
         "→ ゴールデンファイルが無い大回路で",
         "  「正しさ」を証明する手段が必要",
         "",
         "・シミュレーション誤差なしで",
         "  厳密判定できる手法が欲しい"],
        ["▶ Monte-Carlo 真値検証 (MDC_MC=1)",
         "  ・SAT を一切使わない独立シミュレーション",
         "  ・大量ランダムパターンで真値を推定",
         "  ・detSolver との三重照合",
         "  → baseline の過大評価を数値で確認",
         "",
         "▶ GT_BDD 厳密検証器 (GT_BDD=1)",
         "  (src/fdp/gt_verify.c)",
         "  ・ネットリストから検出関数 D_f を",
         "    BDD で直接構築（SAT 不使用）",
         "  ・キューブ和集合 vs D_f を",
         "    ポインタ比較で厳密照合",
         "  ・sound (⊆) / exact (=) を判定",
         "  ・GT_CUBES=1 で非健全キューブの",
         "    特定・固定候補を自動列挙"],
        ["✔ MC: s298 G14/sa0 で baseline が",
         "  真値より +0.008 過大を実証",
         "",
         "✔ GT_BDD: 誤差ゼロの厳密等価判定",
         "  （シミュレーション誤差なし）",
         "",
         "✔ c17a ゴールデン一致 + 独立 sim との",
         "  三重照合で GT_BDD 自体を検証",
         "",
         "◆ 発見: 修正前 s298:6 / s510:40 /",
         "  s641:9 / s713:10 / s5378:54 故障",
         "  で不一致 → Phase 5 で修正"],
    ])

# ═══════════════════════════════════════════════
# Slide 7: Phase 5 3バグ修正
# ═══════════════════════════════════════════════
def slide_phase5(prs):
    sl = blank_slide(prs)
    bg_rect(sl, BG)
    header_bar(sl, "Phase 5  3バグ修正 — 全9回路 ALL VERIFIED 達成",
               "EA過小評価・XID J-frontier・XIDサイト逆含意の根絶 (2026-06-13)", ACCENT3)

    bugs = [
        ("修正1: EA 過小評価・偽冗長",
         ACCENT2,
         ["症状: complete=1 でも和集合 ⊊ D_f",
          "s510: 40故障 / s5378: 26故障 / 偽冗長も発生",
          "",
          "原因: 観測点を先に通過した経路があると",
          "残りフロンティアは全検出経路の支配点でない",
          "→ 側面入力固定が先の検出経路を排除",
          "",
          "修正: obs_seen フラグを追加し",
          "PO/DFF到達後の割り当てを禁止",
          "",
          "確認: MDC_NOEA=1 で全過小が消えることで特定"]),
        ("修正2: XID J-frontier 正当化漏れ",
         ACCENT4,
         ["症状: キューブが非検出ミンタームを含む",
          "修正後: s298:6→2 / s641:9→2 / s713:10→2",
          "",
          "原因: (a) D を運ぶ入力が両側フィルタで脱落",
          "(b) 候補1つでコミットされず要求が消滅",
          "→ 必須 PI が X のまま出力",
          "",
          "修正: 探索を正常側/故障側で独立化",
          "候補が1つでも必ずコミット",
          "",
          "確認: GT_CUBES=1 でバグ箇所を自動特定"]),
        ("修正3: 故障サイト逆含意の汚染",
         PURPLE,
         ["症状: 修正2後も残る過大評価",
          "s298:2 / s641:2 / s713:2 / s5378:20",
          "",
          "原因: 分岐故障のステム故障値が",
          "前方含意で上書きされ D 効果が消失",
          "汚染値が別経路の側面条件を偽正当化",
          "→ 必要な care ビットが消える",
          "",
          "修正: bwd_imp に fsig_id を渡し",
          "サイトでは f3v を X にマスクして含意"]),
    ]

    col_w = Inches(4.0)
    xs = [Inches(0.25), Inches(4.52), Inches(8.79)]
    y0 = Inches(1.1)
    card_h = Inches(5.6)
    for (title, color, lines), x in zip(bugs, xs):
        colored_box(sl, x, y0, col_w, Inches(0.4), color,
                    text=title, font_size=12, bold=True)
        sh = sl.shapes.add_shape(1, x, y0 + Inches(0.4), col_w, card_h - Inches(0.4))
        sh.fill.solid()
        sh.fill.fore_color.rgb = PANEL
        sh.line.color.rgb = color
        sh.line.width = Pt(1.5)
        tx = sl.shapes.add_textbox(x + Inches(0.1), y0 + Inches(0.52),
                                    col_w - Inches(0.2), card_h - Inches(0.62))
        tf = tx.text_frame
        tf.word_wrap = True
        for i, line in enumerate(lines):
            p = tf.paragraphs[0] if i == 0 else tf.add_paragraph()
            run = p.add_run()
            run.text = line
            run.font.size = Pt(11)
            if line.startswith("症状"):
                run.font.color.rgb = ACCENT2; run.font.bold = True
            elif line.startswith("修正"):
                run.font.color.rgb = ACCENT3; run.font.bold = True
            elif line.startswith("原因"):
                run.font.color.rgb = ACCENT4; run.font.bold = True
            elif line.startswith("確認"):
                run.font.color.rgb = LBLUE
            else:
                run.font.color.rgb = WHITE

    add_text(sl, "✔  全9回路・全故障  ALL VERIFIED  — 3修正で不一致ゼロを達成",
             Inches(0.3), Inches(6.88), Inches(12.5), Inches(0.45),
             font_size=14, bold=True, color=ACCENT3, align=PP_ALIGN.CENTER)

# ═══════════════════════════════════════════════
# Slide 8: Phase 6 CUBE_TREND
# ═══════════════════════════════════════════════
def slide_phase6(prs):
    sl = blank_slide(prs)
    bg_rect(sl, BG)
    header_bar(sl, "Phase 6  CUBE_TREND — キューブ生成傾向の分析",
               "一部故障でキューブ数が爆発する根本原因を特定 (CUBE_TREND=1)", TEAL)
    three_col(sl, [
        ["・一部故障で cube 本数が異常に膨大",
         "  (b12 最大 521,746 本 / s1238 で 1000+本)",
         "",
         "・大回路で -limit 打ち切りが頻発し",
         "  FDP が不完全になる",
         "",
         "仮説1: X が極端に少なくなり",
         "  和集合が飽和しない",
         "",
         "仮説2: X 位置がほぼ同じで",
         "  拡大効果がほぼない",
         "",
         "→ どちらが主因かを計測で判定"],
        ["▶ CUBE_TREND=1 で有効化",
         "  (src/fdp/cube_trend.c)",
         "",
         "計測量 (故障ごとに [CT] 1行出力):",
         "  ・X/cube 平均 & head→tail 変化",
         "  ・mask_reuse: 同一 X マスク重複率",
         "  ・Xsupport: X になった PI 種類数",
         "  ・consec: 連続キューブの",
         "    X マスク一致率 & ケア値差分",
         "",
         "▶ バケット集計",
         "  cube 数帯ごとに avg_X% / mask_reuse%",
         "  → 仮説の定量的検証",
         "",
         "▶ CUBE_TREND_CSV で明細 CSV 出力",
         "  x_count vs idx プロットも可能"],
        ["✔ 仮説2「X 位置重複」を強く支持",
         "  ・連続キューブの 82.8% で",
         "    ケア値の変化がゼロ",
         "  ・mask_reuse がキューブ数と",
         "    単調増加 (13 → 25%)",
         "  → 同じ入力割当を保ったまま",
         "    X 窓だけをずらす「ほぼ複製」を",
         "    量産しているため飽和しない",
         "",
         "✔ 仮説1「X 不足」は弱〜中程度",
         "  X 少 → cube 多 は故障選別効果",
         "  爆発中の1故障内では X は減らない",
         "",
         "◆ 爆発の主因: 拡大効果欠如",
         "  → 素項化(MAXDC)で対処へ"],
    ])

# ═══════════════════════════════════════════════
# Slide 9: Phase 7 MAXDC
# ═══════════════════════════════════════════════
def slide_phase7(prs):
    sl = blank_slide(prs)
    bg_rect(sl, BG)
    header_bar(sl, "Phase 7  MAXDC — 非検出オラクルによる素項展開",
               "案1: 各キューブを素項(prime implicant)に拡大して cube 数を削減", ORANGE2)
    three_col(sl, [
        ["・列挙キューブは素項でない",
         "  = もっと X を増やせる余地がある",
         "",
         "・CUBE_TREND で判明:",
         "  「拡大効果の欠如」が爆発の主因",
         "",
         "→ 各キューブを素項に展開すれば",
         "  より少ない本数で同じ検出集合を",
         "  カバーできるはず",
         "",
         "・動機: cube↓ → BDD 構築コスト↓",
         "  + 打ち切り時の FDP 精度↑",
         "  + 完成する故障数が増える"],
        ["▶ MAXDC=1 で有効化",
         "  (src/fdp/experiment.c, MDC_Expand)",
         "",
         "▶ 案1: per-bit greedy 展開",
         "  各 PI ビットについて SAT を呼び",
         "  「X にしても非検出でないか？」を問合",
         "  → YES なら X に固定して次へ",
         "",
         "▶ MAXDC_CORE=1 (改良版)",
         "  UNSAT コアで一括判定:",
         "  1 solve/cube で素項化",
         "  per-bit より大幅に安価",
         "",
         "▶ GT_BDD=1 で健全性を同時確認",
         "  全構成 UNSOUND=0 を確認済み"],
        ["✔ cube 削減: 30〜75%",
         "  s208: 18,430 → 4,507本 (−75%)",
         "  s641: 27,819 → 22,784本 (−18%)",
         "",
         "✘ per-bit MAXDC: 総時間は増加",
         "  overhead が支配",
         "  s5378: 58s → 136s (+135%)",
         "  → 不採用",
         "",
         "✔ MAXDC_CORE: cube↓ SAT時間↓ 完成率↑",
         "  s5378 limit30 complete:",
         "  1,571 → 2,179 (+608故障)",
         "  limit100 cube: −28.6%",
         "  cadical: 134s → 113s",
         "",
         "✔ 全構成 GT_BDD: UNSOUND=0"],
    ])

# ═══════════════════════════════════════════════
# Slide 10: Phase 8 MAXHAM
# ═══════════════════════════════════════════════
def slide_phase8(prs):
    sl = blank_slide(prs)
    bg_rect(sl, BG)
    header_bar(sl, "Phase 8  MAXHAM（案2）— 最大ハミング距離多様化（却下）",
               "多様なキューブで空間カバレッジを向上させる試み / 実験で不採用", ACCENT2)
    three_col(sl, [
        ["・キューブが near-duplicate になる",
         "  → SAT が似た解を返し続ける",
         "",
         "・禁止節だけでは新しい方向の",
         "  解を強制できない",
         "",
         "→ 直前キューブとのハミング距離が",
         "  K 以上の解のみを受け入れる制約を追加",
         "  → より多様なキューブ列に",
         "",
         "・期待: cube 多様化 → cube 数↓",
         "  BDD コスト↓"],
        ["▶ MAXHAM=1 で有効化",
         "  (MAXHAM_K で目標距離を指定)",
         "",
         "▶ at-most 制約（Sinz 逐次カウンタ）",
         "  「直前キューブとケア値が一致する",
         "   PI の個数 ≤ (n_pi − K)」",
         "  を各 solve に追加",
         "",
         "▶ K を大きくするほど多様化",
         "  ただし CNF が肥大",
         "",
         "▶ K → 2 の幾何降下で K を下げながら",
         "  解を探索"],
        ["✘ 実行時間: 8〜15倍に増加",
         "  s641: 5s → 73s (×15)",
         "  s713: 6s → 47s (×8)",
         "",
         "✘ cube 数: 削減なし〜むしろ増加",
         "  s641: −5.8% (悪化)",
         "  s713: −7.4% (悪化)",
         "",
         "✘ FDP 過大評価:",
         "  多様化制約下で XID が",
         "  非検出点を混入",
         "",
         "→ 費用対効果が最悪",
         "  「多様化」路線は cube 削減にならない",
         "",
         "◆ 結論: MAXHAM は却下",
         "  MAXDC_CORE が本命"],
    ])

# ═══════════════════════════════════════════════
# Slide 11: データ比較表
# ═══════════════════════════════════════════════
def slide_data(prs):
    sl = blank_slide(prs)
    bg_rect(sl, BG)
    header_bar(sl, "主要回路 実測データ比較",
               "baseline / MAXDC_CORE / MAXHAM の cube 数・実行時間・完成率", ACCENT1)

    headers = ["回路", "手法", "cube 総数", "削減率", "wall (s)", "complete 数", "健全性"]
    rows = [
        ("c17a",    "baseline",    "   274",  "−",     " 0.02", "22/22",     "ALL VERIFIED"),
        ("s208",    "baseline",    "18,430",  "−",     " 0.42", "215/215",   "ALL VERIFIED"),
        ("s208",    "MAXDC_CORE",  " 4,507",  "−75%",  " 0.58", "215/215",   "ALL VERIFIED"),
        ("s641",    "baseline",    "27,819",  "−",     "~5",    "463/463",   "ALL VERIFIED"),
        ("s641",    "MAXDC_CORE",  "22,784",  "−18%",  "~5",    "463/463",   "ALL VERIFIED"),
        ("s641",    "MAXHAM",      "42,304",  "+52%",  "73.5",  "463/463",   "FDP 過大評価"),
        ("s5378",   "baseline",    "105,977", "−",     "57.9",  "1,571/4,551","ALL VERIFIED"),
        ("s5378",   "MAXDC_CORE",  " 91,736", "−13%",  "~55",   "2,179/4,551","ALL VERIFIED"),
        ("s5378",   "MAXDC(per-bit)","75,524","−27%",  "135.7", "−",         "ALL VERIFIED"),
    ]

    col_widths = [Inches(1.05), Inches(1.55), Inches(1.25), Inches(0.95),
                  Inches(1.0), Inches(1.55), Inches(1.55)]
    x = Inches(0.28)
    x_starts = []
    for w in col_widths:
        x_starts.append(x); x += w + Inches(0.04)

    row_h = Inches(0.38)
    y0 = Inches(1.1)

    for ci, (hdr, xs, cw) in enumerate(zip(headers, x_starts, col_widths)):
        colored_box(sl, xs, y0, cw, row_h, ACCENT1,
                    text=hdr, font_size=11, bold=True)

    for ri, row in enumerate(rows):
        y = y0 + row_h + Inches(0.02) + ri * (row_h + Inches(0.02))
        row_bg = PANEL if ri % 2 == 0 else RGBColor(0x20, 0x2d, 0x50)
        for ci, (val, xs, cw) in enumerate(zip(row, x_starts, col_widths)):
            sh = sl.shapes.add_shape(1, xs, y, cw, row_h)
            sh.fill.solid()
            sh.fill.fore_color.rgb = row_bg
            sh.line.color.rgb = RGBColor(0x44, 0x44, 0x66)
            sh.line.width = Pt(0.5)
            tx = sl.shapes.add_textbox(xs + Inches(0.04), y + Inches(0.05),
                                        cw - Inches(0.08), row_h - Inches(0.1))
            tf = tx.text_frame
            p = tf.paragraphs[0]
            p.alignment = PP_ALIGN.CENTER if ci > 0 else PP_ALIGN.LEFT
            run = p.add_run()
            run.text = val
            run.font.size = Pt(11)
            if val == "ALL VERIFIED":
                run.font.color.rgb = ACCENT3; run.font.bold = True
            elif "過大評価" in val or val.startswith("+"):
                run.font.color.rgb = ACCENT2; run.font.bold = True
            elif val.startswith("−") and "%" in val:
                run.font.color.rgb = ACCENT3; run.font.bold = True
            elif val == "−" or val == "−":
                run.font.color.rgb = GRAY
            else:
                run.font.color.rgb = WHITE

    add_text(sl, "※ s5378 は -limit 30。s208/s641 は完全列挙。MAXDC(per-bit) は per-bit greedy 展開（遅すぎるため不採用）。",
             Inches(0.3), Inches(6.82), Inches(12.5), Inches(0.45),
             font_size=11, color=GRAY, italic=True)

# ═══════════════════════════════════════════════
# Slide 12: まとめ
# ═══════════════════════════════════════════════
def slide_summary(prs):
    sl = blank_slide(prs)
    bg_rect(sl, BG)
    header_bar(sl, "まとめ  —  成果と今後の展望", "", ACCENT1)

    add_text(sl, "時系列サマリー",
             Inches(0.3), Inches(1.12), Inches(6.5), Inches(0.35),
             font_size=14, bold=True, color=LBLUE)

    tl = [
        (ACCENT1,  "①  基本実装",        "SAT+BDD+GMP で FDP 厳密計算"),
        (DKBLUE,   "②  等価故障・支配",   "重複列挙を排除し効率化"),
        (PURPLE,   "③  XID インライン化", "外部プロセス廃止 → 約10倍高速"),
        (ACCENT4,  "④  検証ツール整備",   "MC + GT_BDD で健全性を厳密確認"),
        (ACCENT3,  "⑤  3バグ修正",        "全9回路 ALL VERIFIED 達成"),
        (TEAL,     "⑥  CUBE_TREND 分析", "near-duplicate 量産が爆発の主因"),
        (ORANGE2,  "⑦  MAXDC 実験",       "素項展開で cube −30〜75%"),
        (ACCENT3,  "⑧  MAXDC_CORE",       "安い素項化で速度・完成率も向上"),
    ]
    y = Inches(1.55)
    for color, title, desc in tl:
        sh = sl.shapes.add_shape(1, Inches(0.3), y + Inches(0.04),
                                  Inches(0.2), Inches(0.26))
        sh.fill.solid()
        sh.fill.fore_color.rgb = color
        sh.line.fill.background()
        tx = sl.shapes.add_textbox(Inches(0.6), y, Inches(5.8), Inches(0.35))
        tf = tx.text_frame
        p = tf.paragraphs[0]
        r1 = p.add_run(); r1.text = title + "  "
        r1.font.size = Pt(11); r1.font.bold = True; r1.font.color.rgb = WHITE
        r2 = p.add_run(); r2.text = desc
        r2.font.size = Pt(10); r2.font.color.rgb = GRAY
        y += Inches(0.44)

    add_text(sl, "現在の状態",
             Inches(7.0), Inches(1.12), Inches(6.0), Inches(0.35),
             font_size=14, bold=True, color=LBLUE)
    status = [
        (ACCENT3, "✔ FDP 健全性: 全9回路 ALL VERIFIED（完全列挙は厳密等価）"),
        (ACCENT3, "✔ MAXDC_CORE: cube↓ SAT時間↓ 完成率↑ を健全に両立"),
        (ACCENT4, "◆ 大回路は依然 -limit 打ち切りが必要"),
        (ACCENT4, "◆ 打ち切り故障の FDP は上界（保守的な近似）"),
    ]
    y_st = Inches(1.55)
    for color, text in status:
        sh = sl.shapes.add_shape(1, Inches(7.0), y_st + Inches(0.04),
                                  Inches(0.2), Inches(0.26))
        sh.fill.solid()
        sh.fill.fore_color.rgb = color
        sh.line.fill.background()
        tx = sl.shapes.add_textbox(Inches(7.3), y_st, Inches(5.7), Inches(0.38))
        tf = tx.text_frame
        tf.word_wrap = True
        p = tf.paragraphs[0]
        run = p.add_run()
        run.text = text
        run.font.size = Pt(11); run.font.color.rgb = WHITE
        y_st += Inches(0.52)

    add_text(sl, "今後の展望",
             Inches(7.0), Inches(3.85), Inches(6.0), Inches(0.35),
             font_size=14, bold=True, color=LBLUE)
    future = [
        "・構成的素項 All-SAT / unate-covering で",
        "  MAXDC_CORE より攻めた素項を安く出す",
        "",
        "・大回路での健全性継続検証 (GT_BDD=1)",
        "",
        "・逐次回路での DFF(PPI) 扱いの精緻化",
        "",
        "・打ち切り故障の FDP 下界も推定できる",
        "  フレームワークの検討",
    ]
    y_fu = Inches(4.3)
    for line in future:
        tx = sl.shapes.add_textbox(Inches(7.0), y_fu, Inches(6.0), Inches(0.3))
        tf = tx.text_frame
        p = tf.paragraphs[0]
        run = p.add_run()
        run.text = line
        run.font.size = Pt(11)
        run.font.color.rgb = GRAY if line.startswith("  ") else WHITE
        y_fu += Inches(0.31)

# ═══════════════════════════════════════════════
# 生成
# ═══════════════════════════════════════════════
slide_title(prs)
slide_timeline(prs)
slide_phase1(prs)
slide_phase2(prs)
slide_phase3(prs)
slide_phase4(prs)
slide_phase5(prs)
slide_phase6(prs)
slide_phase7(prs)
slide_phase8(prs)
slide_data(prs)
slide_summary(prs)

out = "/home/koizumi/fdp/FDP_開発記録.pptx"
prs.save(out)
print(f"Saved: {out}  ({len(prs.slides)} slides)")
