from pathlib import Path
import csv,re,json,math,zipfile
from collections import Counter
import xlsxwriter
from openpyxl import load_workbook
from PIL import Image, ImageDraw, ImageFont
BASE=Path(__file__).resolve().parent
ROOT=BASE.parent
CIRCS=['s5378','s9234','s13207','s15850']
METHODS=['xid','sbxid']; LABELS=['XID','SB-XID']
FIELDS=['Time','CPU Time','CPU Time (CaDiCaL)','CPU Time (BDD)',"CPU Time (Don't care)"]
D={}; SOURCE=[]
for c in CIRCS:
 D[c]={}
 for m in METHODS:
  lp=ROOT/'log'/f'{c}_C_{m}.txt'; cp=ROOT/'fdp'/f'{c}_C_{m}.csv'
  log={k.strip():v.strip() for k,v in re.findall(r'^//\s+(.+?)\s+: (.*)$',lp.read_text(),re.M)}
  rows=list(csv.DictReader(cp.open())); cnt=Counter(r['complete'] for r in rows)
  faults={(r['net_name'],r['f_type']):r for r in rows}
  n=int(log['Number of Target Faults'])
  assert len(faults)==len(rows) and cnt['0']+cnt['1']==n and set(cnt)<={'0','1',''}
  assert all(r['cube_cnt']=='30' for r in rows if r['complete']=='0')
  assert sum(int(r['cube_cnt']) for r in rows if r['cube_cnt'])==int(log['Total Cubes / Reused Cubes'].split('/')[0])
  D[c][m]={'log':log,'counts':dict(cnt),'total_rows':len(rows),'target':n,'times':{f:float(log[f].split()[0]) for f in FIELDS},'faults':faults}
  SOURCE += [str(lp.relative_to(ROOT)),str(cp.relative_to(ROOT))]
 a,b=[D[c][m] for m in METHODS]
 assert a['faults'].keys()==b['faults'].keys() and a['target']==b['target']
 D[c]['trans']=Counter((a['faults'][k]['complete'],b['faults'][k]['complete']) for k in a['faults'])
 print(c,'complete',a['counts']['1'],b['counts']['1'],'delta',b['counts']['1']-a['counts']['1'],'CPU ratio',round(b['times']['CPU Time']/a['times']['CPU Time'],2),'transitions',dict(D[c]['trans']))

OUT=BASE/'xid_sbxid_4circuits.xlsx'; wb=xlsxwriter.Workbook(OUT)
wb.set_properties({'title':'XID / SB-XID：4回路の実験結果比較'})
COLORS=['#2878B5','#E58A2B']; NAVY='#20344A'; GRAY='#64748B'
font='Yu Gothic'
fm={
 'title':wb.add_format({'font_name':font,'font_size':22,'bold':True,'font_color':NAVY}),
 'h':wb.add_format({'font_name':font,'font_size':12,'bold':True,'bg_color':NAVY,'font_color':'white','text_wrap':True,'valign':'vcenter'}),
 'text':wb.add_format({'font_name':font,'font_size':12,'border':1,'border_color':'#DFE5EC','valign':'vcenter'}),
 'int':wb.add_format({'font_name':font,'font_size':12,'num_format':'#,##0','border':1,'border_color':'#DFE5EC'}),
 'sec':wb.add_format({'font_name':font,'font_size':12,'num_format':'#,##0.000','border':1,'border_color':'#DFE5EC'}),
 'pct':wb.add_format({'font_name':font,'font_size':12,'num_format':'0.00%','border':1,'border_color':'#DFE5EC'}),
 'ratio':wb.add_format({'font_name':font,'font_size':12,'num_format':'0.00"倍"','border':1,'border_color':'#DFE5EC'}),
 'pp':wb.add_format({'font_name':font,'font_size':12,'num_format':'0.00" pt"','border':1,'border_color':'#DFE5EC'}),
 'note':wb.add_format({'font_name':font,'font_size':12,'font_color':GRAY,'text_wrap':True,'valign':'top'}),
}
def sheet(name,title,headers,widths=None):
 s=wb.add_worksheet(name); s.hide_gridlines(2); s.freeze_panes(4,1); s.set_default_row(26)
 s.merge_range(0,0,0,max(len(headers)-1,5),title,fm['title']); s.set_row(0,40)
 s.set_column(0,0,18); s.set_column(1,max(len(headers)-1,5),19)
 if widths:
  for col,width in widths: s.set_column(col,col,width)
 for j,h in enumerate(headers):s.write(3,j,h,fm['h'])
 s.set_row(3,42); s.set_landscape(); s.set_paper(9); s.fit_to_pages(1,1)
 return s
def cell(s,r,c,value,typ='text'):s.write(r,c,value,fm[typ])
def formula(s,r,c,f,value,typ):s.write_formula(r,c,f,fm[typ],value)
summary=sheet('比較概要','XID / SB-XID：4回路の比較',['回路','対象故障数','完了 XID','完了 SB-XID','完了増加数','完了率差 (pt)','CPU時間倍率','経過時間倍率'])
complete=sheet('完了故障数','complete=1 の故障数と完了率',['回路','方式','対象故障数','CSV全行数','complete=1','complete=0','complete空欄','完了率'])
times=sheet('時間比較','時間比較（全項目：秒）',['回路','ログ項目','XID (s)','SB-XID (s)','差 (s)','倍率'],[(1,30)])
clasp=sheet('Clasp集計','SB-XID：Clasp集計',['回路','Optimal','Feasible','Timeouts','UNKNOWN Fallbacks','Feasible率'])
for i,c in enumerate(CIRCS):
 a,b=[D[c][m] for m in METHODS]; n=a['target']; x=a['counts']['1']; y=b['counts']['1']; r=4+i
 vals=[c,n,x,y,y-x,(y-x)/n*100,b['times']['CPU Time']/a['times']['CPU Time'],b['times']['Time']/a['times']['Time']]
 for j,v in enumerate(vals):cell(summary,r,j,v,'text' if j==0 else 'pp' if j==5 else 'ratio' if j>=6 else 'int')
 formula(summary,r,4,f'=D{r+1}-C{r+1}',y-x,'int')
 formula(summary,r,5,f'=(D{r+1}-C{r+1})/B{r+1}*100',(y-x)/n*100,'pp')
 for k,m in enumerate(METHODS):
  o=D[c][m]; rr=4+i*2+k
  for j,v in enumerate([c,LABELS[k],n,o['total_rows'],o['counts']['1'],o['counts']['0'],o['counts'].get('',0),o['counts']['1']/n]):cell(complete,rr,j,v,'text' if j<2 else 'pct' if j==7 else 'int')
  formula(complete,rr,7,f'=E{rr+1}/C{rr+1}',o['counts']['1']/n,'pct')
 for k,f in enumerate(FIELDS):
  rr=4+i*5+k; v1=a['times'][f]; v2=b['times'][f]
  for j,v in enumerate([c,f,v1,v2]):cell(times,rr,j,v,'text' if j<2 else 'sec')
  formula(times,rr,4,f'=D{rr+1}-C{rr+1}',v2-v1,'sec');formula(times,rr,5,f'=D{rr+1}/C{rr+1}',v2/v1,'ratio')
 opt,feas=map(int,b['log']['Clasp Optimal / Feasible'].split(' / ')); to,unk=map(int,b['log']['Clasp Timeouts / UNKNOWN Fallbacks'].split(' / '))
 for j,v in enumerate([c,opt,feas,to,unk,feas/(opt+feas)]):cell(clasp,r,j,v,'text' if j==0 else 'pct' if j==5 else 'int')
 formula(clasp,r,5,f'=C{r+1}/(B{r+1}+C{r+1})',feas/(opt+feas),'pct')
summary.merge_range('A11:H12','全回路：SAF、15ワーカー、30キューブ上限。倍率 = SB-XID / XID。完了率の分母はログの対象故障数。各方式・各回路1実験。',fm['note'])
summary.merge_range('A14:H15','グラフは G01〜G08 シート。Excel上で編集・コピー可能。PNGフォルダにはPowerPointに貼れる16:9画像を収録（1920×1080）。青：XID、橙：SB-XID。',fm['note'])
complete.merge_range('A15:H16','completeが厳密に1の行だけを完了とする。空欄は完了数と完了率の分母に含めない。complete=0は全件cube_cnt=30。',fm['note'])
clasp.merge_range('A11:F12','Feasible率 = Feasible / (Optimal + Feasible)。TimeoutsとFeasibleは別のログ項目であり、足し合わせない。s13207はFeasible 3,051件、Timeouts 3,052件。',fm['note'])
# 故障単位での状態変化
complete.write_row(19,0,['回路','両方完了','XID未完了→完了','XID完了→未完了','両方未完了','両方空欄'],fm['h'])
complete.set_row(19,44)
for i,c in enumerate(CIRCS):
 cell(complete,20+i,0,c)
 for j,k in enumerate([('1','1'),('0','1'),('1','0'),('0','0'),('','')],1):cell(complete,20+i,j,D[c]['trans'][k],'int')

# PowerPoint用PNG：白背景、16:9、大きな文字、値ラベル付き。
PNG=BASE/'png';PNG.mkdir(exist_ok=True)
FONTP='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'; BOLD='/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'
def ft(n,b=False):return ImageFont.truetype(BOLD if b else FONTP,n)
def nice_max(v):
 if v<=0:return 1
 p=10**math.floor(math.log10(v)); q=v/p
 return next(z for z in [1,1.5,2,2.5,3,4,5,6,8,10] if z>=q)*p

def png_bars(slug,title,values,unit,fmt,foot,maximum=None):
 im=Image.new('RGB',(1920,1080),'white'); dr=ImageDraw.Draw(im)
 dr.text((105,60),title,font=ft(46,True),fill=NAVY)
 dr.text((108,126),'SAF | 15 workers | 30-cube limit',font=ft(25),fill=GRAY)
 for j,label in enumerate(LABELS if len(values)==2 else []):
  px=1350+j*220;dr.rectangle((px,125,px+30,155),fill=COLORS[j]);dr.text((px+42,119),label,font=ft(28),fill=NAVY)
 left,right,top,bottom=150,1820,225,865
 ymax=maximum if maximum else nice_max(max(max(v) for v in values)*1.17)
 for k in range(6):
  v=ymax*k/5; py=bottom-(bottom-top)*k/5
  dr.line((left,py,right,py),fill='#E4E9EF',width=2)
  label=f'{v:,.0f}' if ymax>=10 else f'{v:,.1f}'
  dr.text((left-20,py),label,font=ft(24),fill=GRAY,anchor='rm')
 dr.text((left,top-48),unit,font=ft(27),fill=GRAY)
 group=(right-left)/4;bw=110 if len(values)==2 else 150
 for i,c in enumerate(CIRCS):
  center=left+group*(i+.5)
  for j,vs in enumerate(values):
   value=vs[i]; x=center+(j-(len(values)-1)/2)*(bw+14);y=bottom-value/ymax*(bottom-top)
   dr.rectangle((x-bw/2,y,x+bw/2,bottom),fill=COLORS[j])
   dr.text((x,y-15),fmt(value),font=ft(27,True),fill=NAVY,anchor='mb')
  dr.text((center,bottom+32),c,font=ft(31,True),fill=NAVY,anchor='mt')
 dr.line((left,bottom,right,bottom),fill=GRAY,width=2)
 dr.text((105,980),foot,font=ft(24),fill=GRAY)
 p=PNG/f'{slug}.png'; im.save(p,dpi=(144,144)); return p

specs=[
 ('G01_完了故障数','Completed faults',[[D[c][m]['counts']['1'] for c in CIRCS] for m in METHODS],'Faults',lambda v:f'{v:,.0f}','Count of complete=1; blank complete values are excluded.',None),
 ('G02_完了率','Completion rate',[[D[c][m]['counts']['1']/D[c][m]['target']*100 for c in CIRCS] for m in METHODS],'%',lambda v:f'{v:.2f}%','Denominator: target faults in each circuit log.',100),
]
for num,f in enumerate(['CPU Time','Time','CPU Time (CaDiCaL)','CPU Time (BDD)',"CPU Time (Don't care)"],3):
 jp=['総CPU時間','経過時間','CaDiCaL','BDD','Dontcare'][num-3]
 specs.append((f'G{num:02}_{jp}',f,[[D[c][m]['times'][f] for c in CIRCS] for m in METHODS],'Seconds',lambda v:f'{v:,.3f}','Linear scale from zero. CPU time sums parent and all workers.' if f!='Time' else 'Linear scale from zero. Time is wall-clock elapsed time.',None))
specs.append(('G08_CPU倍率','CPU time ratio (SB-XID / XID)',[[D[c]['sbxid']['times']['CPU Time']/D[c]['xid']['times']['CPU Time'] for c in CIRCS]],'Ratio',lambda v:f'{v:.2f}x','Ratio of measured CPU times; one experiment per method and circuit.',None))
# グラフ用データは同じシートの下部。数値キャッシュとPNGに同じ値を使用。
for slug,title,values,unit,label_fmt,foot,maximum in specs:
 sh=wb.add_worksheet(slug);sh.hide_gridlines(2);sh.set_column('A:P',12);sh.set_default_row(24)
 sh.write('A1',title,fm['title']);sh.merge_range('A3:P3',foot,fm['note'])
 sh.write(37,0,'Circuit',fm['h'])
 for j in range(len(values)):sh.write(37,j+1,LABELS[j] if len(values)==2 else 'SB-XID / XID',fm['h'])
 for i,c in enumerate(CIRCS):
  sh.write(38+i,0,c,fm['text'])
  for j,vs in enumerate(values):sh.write_number(38+i,j+1,vs[i],fm['sec'] if unit=='Seconds' else fm['int'] if unit=='Faults' else fm['ratio'] if unit=='Ratio' else fm['sec'])
 ch=wb.add_chart({'type':'column'})
 for j in range(len(values)):
  nf='#,##0.000' if unit=='Seconds' else '0.00"%"' if unit=='%' else '0.00"x"' if unit=='Ratio' else '#,##0'
  ch.add_series({'name':[slug,37,j+1],'categories':[slug,38,0,41,0],'values':[slug,38,j+1,41,j+1],
   'fill':{'color':COLORS[j]},'border':{'none':True},'gap':95,
   'data_labels':{'value':True,'position':'outside_end','num_format':nf,'font':{'name':'Arial','size':14,'color':NAVY}}})
 ch.set_title({'name':title,'name_font':{'name':'Arial','size':23,'bold':True,'color':NAVY}})
 ch.set_x_axis({'num_font':{'name':'Arial','size':17},'line':{'color':'#A7B3C2'},'major_tick_mark':'none'})
 axis={'name':unit,'name_font':{'name':'Arial','size':15},'num_font':{'name':'Arial','size':14},'min':0,'max':maximum or nice_max(max(max(v) for v in values)*1.17),'major_gridlines':{'visible':True,'line':{'color':'#E4E9EF'}},'line':{'none':True}}
 ch.set_y_axis(axis);ch.set_legend({'position':'top','font':{'name':'Arial','size':17}} if len(values)==2 else {'none':True})
 ch.set_chartarea({'border':{'none':True},'fill':{'color':'white'}});ch.set_plotarea({'border':{'none':True}})
 ch.set_size({'width':1280,'height':720});sh.insert_chart('A5',ch)
 sh.set_landscape();sh.set_paper(9);sh.fit_to_pages(1,1);sh.print_area('A1:P35')
 png_bars(slug,title,values,unit,label_fmt,foot,maximum)

# Claspは桁が大きく異なるため、正確な比較ができるスライド用表。
im=Image.new('RGB',(1920,1080),'white');dr=ImageDraw.Draw(im)
dr.text((100,70),'SB-XID: Clasp results',font=ft(48,True),fill=NAVY)
cols=[100,450,815,1155,1505]; headers=['Circuit','Optimal','Feasible','Timeouts','UNKNOWN']
for j,h in enumerate(headers):dr.text((cols[j],235),h,font=ft(33,True),fill='white')
# header背景は先に描き、文字を再描画。
dr.rectangle((80,208,1840,290),fill=NAVY)
for j,h in enumerate(headers):dr.text((cols[j],230),h,font=ft(33,True),fill='white')
for i,c in enumerate(CIRCS):
 y=310+i*125
 if i%2==0:dr.rectangle((80,y-5,1840,y+105),fill='#EFF4F9')
 opt,feas=map(int,D[c]['sbxid']['log']['Clasp Optimal / Feasible'].split(' / '));to,unk=map(int,D[c]['sbxid']['log']['Clasp Timeouts / UNKNOWN Fallbacks'].split(' / '))
 for j,v in enumerate([c,f'{opt:,}',f'{feas:,}',f'{to:,}',f'{unk:,}']):dr.text((cols[j],y+25),v,font=ft(36,j==0),fill=NAVY)
dr.text((100,910),'Feasible and Timeouts are separate log fields; do not add them.',font=ft(27),fill=GRAY)
dr.text((100,960),'Clasp 3.3.9 | Search limit: 1.0 sec/call | SB-XID v1 (no CEGAR)',font=ft(26),fill=GRAY)
im.save(PNG/'G09_Clasp.png',dpi=(144,144))
# 同じClasp画像をExcelからもコピーできる。
clasp.insert_image('A16',str(PNG/'G09_Clasp.png'),{'x_scale':.6,'y_scale':.6})

src=sheet('集計条件・参照元','集計条件・参照元',['項目','内容'],[(0,28),(1,90)])
notes=[('対象回路',', '.join(CIRCS)),('Fault Model','SAF'),('Fault Workers','15（全実験共通）'),('Normal CNF Scope','on (affected-output TFI)'),('Essential Assignment','on'),('Dominance Cube Reuse','off'),('SB-XID objective','scope (normal CNF PI)'),('SB-XID version','SB-XID v1 (no CEGAR)'),('Clasp','3.3.9 / 1.0 sec/call'),('CPU Time','親プロセス + 全ワーカーのCPU時間。Time（経過時間）と区別する'),('完了件数','complete が厳密に1の故障数'),('完了率','complete=1 / ログのNumber of Target Faults'),('差・倍率','差 = SB-XID − XID、倍率 = SB-XID / XID'),('状態照合','各回路で(net_name, f_type)を照合。同一故障集合・重複なし'),('時間内訳','要求された3内訳は総CPU時間の全項目を網羅しない'),('解釈上の範囲','各回路・各方式1実験。平均・標準偏差・統計的有意差は未評価'),('PowerPointへの貼付','G01〜G08のグラフをコピー。画像ならpngフォルダを使用'),('図の仕様','白背景、青=XID／橙=SB-XID、線形軸・ゼロ起点、値ラベル付き'),('画像の仕様','1920×1080（16:9）PNG、144 dpi。図中のラベルは英語')]
for r,(k,v) in enumerate(notes,4):cell(src,r,0,k);cell(src,r,1,v)
start=26;src.write(start,0,'参照元（output/limit30相対パス）',fm['h'])
for i,p in enumerate(SOURCE,start+1):src.write(i,0,p,fm['note'])
wb.close()
# Excelを再読込し、全回路の主要セル・数式キャッシュとグラフ数を検証。
check=load_workbook(OUT,data_only=True)
for i,c in enumerate(CIRCS):
 r=5+i
 assert check['比較概要'].cell(r,3).value==D[c]['xid']['counts']['1']
 assert check['比較概要'].cell(r,4).value==D[c]['sbxid']['counts']['1']
 assert check['比較概要'].cell(r,5).value==D[c]['sbxid']['counts']['1']-D[c]['xid']['counts']['1']
check2=load_workbook(OUT)
assert sum(len(s._charts) for s in check2)==8
for p in PNG.glob('*.png'):
 with Image.open(p) as im:assert im.size==(1920,1080)
with zipfile.ZipFile(BASE/'PowerPoint用グラフPNG.zip','w',zipfile.ZIP_DEFLATED) as z:
 for p in sorted(PNG.glob('*.png')):z.write(p,p.name)
print('Workbook:',OUT,'bytes',OUT.stat().st_size)
print('Verified',len(check.sheetnames),'sheets, 8 editable charts, 9 PNG files.')
# 画像確認用のコンタクトシート
thumb=Image.new('RGB',(1440,3*270),'#DCE3EB')
for i,p in enumerate(sorted(PNG.glob('*.png'))):
 im=Image.open(p);im.thumbnail((480,270));thumb.paste(im,((i%3)*480,(i//3)*270))
thumb.save(BASE/'preview.png')
