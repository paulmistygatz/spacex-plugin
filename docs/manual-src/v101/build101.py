# -*- coding: utf-8 -*-
# SpaceX 1.0.1 manual builder: HTML (fonts + images embedded) -> PDF via Playwright/Chromium.
import base64, html, os, random, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from text101 import *

HERE = os.path.dirname(os.path.abspath(__file__))
def b64(path, mime):
    with open(os.path.join(HERE, path), 'rb') as f:
        return 'data:%s;base64,%s' % (mime, base64.b64encode(f.read()).decode())
def jpg(name): return b64('img/%s.jpg' % name, 'image/jpeg')
def img(name):
    # PNGs with alpha stay PNG, screenshots go in as JPEG - the PDF is embedded in the plugin binary
    if name.endswith('_a'):
        return b64('img/%s.png' % name, 'image/png')
    import io
    from PIL import Image
    im = Image.open(os.path.join(HERE, 'img/%s.png' % name)).convert('RGB')
    if im.size[0] > 1800:
        im = im.resize((1800, round(im.size[1] * 1800 / im.size[0])), Image.LANCZOS)
    buf = io.BytesIO(); im.save(buf, 'JPEG', quality=86, optimize=True)
    return 'data:image/jpeg;base64,' + base64.b64encode(buf.getvalue()).decode()
def e(s): return html.escape(s, quote=False)

FONTS = ''
for fam, w in [('Sora', 300), ('Sora', 400), ('Sora', 600), ('Sora', 700),
               ('Inter', 300), ('Inter', 400), ('Inter', 500), ('Inter', 600), ('Inter', 700)]:
    FONTS += "@font-face{font-family:'%s';font-weight:%d;src:url(%s) format('woff2')}\n" % (
        fam, w, b64('fonts/%s-latin-%d-normal.woff2' % (fam.lower(), w), 'font/woff2'))

# deterministic star dust as SVG background
def stars(seed, n=90, w=210, h=297):
    r = random.Random(seed)
    out = []
    for _ in range(n):
        x, y = r.uniform(0, w), r.uniform(0, h)
        rad = r.choice([0.12, 0.15, 0.18, 0.25, 0.35])
        op = r.uniform(0.15, 0.6)
        col = r.choice(['#ffffff', '#ffffff', '#dabd76', '#7fb0ff'])
        out.append('<circle cx="%.1f" cy="%.1f" r="%.2f" fill="%s" opacity="%.2f"/>' % (x, y, rad, col, op))
    return ('<svg class="dust" viewBox="0 0 %d %d" preserveAspectRatio="none" xmlns="http://www.w3.org/2000/svg">%s</svg>'
            % (w, h, ''.join(out)))

CSS = FONTS + r"""
@page { size: A4; margin: 0 }
:root{ --bg:#0b0d12; --card:#141821; --card2:#10131a; --line:#ffffff14; --gold:#dabd76; --gold2:#ecdcb8;
       --blue:#7fb0ff; --text:#dfe3ea; --mut:#9aa1ae; --dim:#6d7280 }
*{ box-sizing:border-box; margin:0; padding:0 }
html,body{ background:var(--bg) }
body{ text-rendering:geometricPrecision; font-kerning:normal; font-family:Inter; color:var(--text); font-size:9.6pt; line-height:1.55; -webkit-print-color-adjust:exact; print-color-adjust:exact }
.page{ width:210mm; height:297mm; position:relative; overflow:hidden; page-break-after:always; padding:20mm 18mm 22mm;
       background: radial-gradient(170mm 130mm at 100% 0%, #18181b 0%, #0b0d12 75%) }
.page:last-child{ page-break-after:auto }
.dust{ position:absolute; inset:0; width:100%; height:100%; z-index:0 }
.page > *:not(.dust){ position:relative; z-index:1 }
.foot{ position:absolute !important; left:18mm; right:18mm; bottom:10mm; display:flex; justify-content:space-between;
       font-family:Sora; font-size:6.8pt; letter-spacing:.28em; color:var(--dim) }
.foot b{ color:var(--gold); font-weight:600 }
.eyebrow{ font-family:Sora; font-weight:600; font-size:7.4pt; letter-spacing:.34em; color:var(--gold); text-transform:uppercase; margin-bottom:3mm }
.eyebrow i{ font-style:normal; color:var(--blue); margin-right:2.5mm }
h1{ font-family:Sora; font-weight:600; font-size:25pt; line-height:1.12; letter-spacing:.06em; text-transform:uppercase; margin-bottom:5mm }
h1 em{ font-style:normal; color:var(--gold) }
h2{ font-family:Sora; font-weight:600; font-size:12pt; letter-spacing:.2em; text-transform:uppercase; margin:0 0 2.5mm }
h2 small{ font-size:7pt; letter-spacing:.24em; color:var(--mut); font-weight:400; margin-left:3mm }
h3{ font-family:Sora; font-weight:600; font-size:8pt; letter-spacing:.22em; text-transform:uppercase; color:var(--gold); margin:0 0 2mm }
p{ margin:0 0 3mm; color:#c9ced8 }
p.lead{ font-size:12.5pt; line-height:1.5; color:var(--text); font-weight:300; margin-bottom:6mm }
.mut{ color:var(--mut) }
.card{ background:linear-gradient(180deg,#171b25,#12151d); border:1px solid var(--line); border-radius:4mm; padding:5mm 5.5mm }
.shot{ display:block; width:100%; border-radius:3mm; border:1px solid #ffffff1c }
.grid2{ display:grid; grid-template-columns:1fr 1fr; gap:5mm }
.grid3{ display:grid; grid-template-columns:repeat(3,1fr); gap:4mm }
.grid4{ display:grid; grid-template-columns:repeat(4,1fr); gap:3.5mm }
.sp{ height:6mm } .sp2{ height:3mm }
dl.kv{ display:grid; grid-template-columns:31mm 1fr; column-gap:4mm; row-gap:2.1mm }
dl.kv.w{ grid-template-columns:44mm 1fr }
dl.kv dt{ font-family:Sora; font-weight:600; font-size:7.4pt; letter-spacing:.14em; text-transform:uppercase; color:var(--gold2); padding-top:.5mm }
dl.kv dd{ color:#c9ced8 }
.tip{ border-left:.7mm solid var(--gold); padding:2.2mm 0 2.2mm 4mm; background:linear-gradient(90deg,#17161a,#0b0d12); border-radius:0 2mm 2mm 0; color:var(--text) }
.tip b, .warn b{ font-family:Sora; font-size:7pt; letter-spacing:.24em; color:var(--gold); display:block; margin-bottom:.8mm; font-weight:600 }
.warn{ border-left:.7mm solid #ff8a7a; padding:2.2mm 0 2.2mm 4mm; background:linear-gradient(90deg,#1a1316,#0b0d12); border-radius:0 2mm 2mm 0 }
.warn b{ color:#ff9d8f }
.pill{ font-family:Sora; font-weight:600; font-size:6.8pt; letter-spacing:.2em; text-transform:uppercase; padding:1.6mm 3mm; border-radius:10mm;
       border:1px solid #dabd7655; color:var(--gold2); background:#dabd7610; display:inline-block; white-space:nowrap }
.pill.b{ border-color:#7fb0ff55; color:#cfe0ff; background:#7fb0ff10 }
.pill.x{ border-color:#ffffff22; color:var(--mut); background:#ffffff06 }
.num{ display:inline-flex; width:5.2mm; height:5.2mm; border-radius:50%; background:var(--gold); color:#141004; font-family:Sora; font-weight:700;
      font-size:6.8pt; align-items:center; justify-content:center; flex:none }
/* cover */
.cover{ padding:0 }
.cover .bg{ position:absolute; inset:0; background-size:100% 100% }
.cover .veil{ display:none }
.cover .top{ position:absolute; top:20mm; left:18mm; right:18mm; display:flex; justify-content:space-between; font-family:Sora; font-size:7pt; letter-spacing:.34em; color:var(--mut) }
.cover .logo{ position:absolute; top:40mm; left:0; right:0; text-align:center }
.cover .logo img{ width:92mm }
.cover .claim{ position:absolute; left:18mm; right:18mm; bottom:66mm }
.cpil{ position:absolute !important; left:18mm; right:18mm; bottom:30mm; display:grid; grid-template-columns:repeat(4,1fr); gap:5mm }
.cpil i{ display:block; width:7mm; height:.7mm; background:var(--gold); border-radius:1mm; margin-bottom:2.5mm }
.cpil h3{ margin-bottom:1.2mm } .cpil p{ font-size:8.6pt; line-height:1.4; color:#c3c8d2; margin:0 }
.cover .claim h1{ font-size:34pt; letter-spacing:.05em; margin-bottom:4mm }
.cover .claim p{ font-size:11.5pt; color:#d5d9e1; max-width:130mm; font-weight:300 }
.cover .meta{ position:absolute; left:18mm; right:18mm; bottom:16mm; display:flex; gap:3mm; align-items:center }
.cover .meta .sp-auto{ flex:1 }
/* chain */
.chain{ display:flex; flex-wrap:wrap; gap:2.2mm 1.4mm; align-items:center }
.chain .arr{ color:var(--dim); font-size:9pt }
/* tour hotspots */
.hot{ position:relative }
.hot .num{ position:absolute; transform:translate(-50%,-50%); border:.5mm solid #0b0d12 }
.legend{ display:grid; grid-template-columns:1fr 1fr; gap:2.2mm 6mm; margin-top:5mm }
.legend div{ display:flex; gap:2.6mm; align-items:flex-start }
.legend b{ font-family:Sora; font-weight:600; font-size:7.2pt; letter-spacing:.12em; text-transform:uppercase; color:var(--gold2); display:block }
.legend span{ color:#b9bfca; font-size:8.6pt; line-height:1.4 }
.legend .num{ color:#141004; font-size:6.8pt; line-height:1 }
ol.steps{ list-style:none; counter-reset:s; display:grid; gap:2.4mm }
ol.steps li{ counter-increment:s; display:flex; gap:3mm; align-items:flex-start; color:#c9ced8 }
ol.steps li:before{ content:counter(s); font-family:Sora; font-weight:700; font-size:7pt; color:#141004; background:var(--gold);
      width:5mm; height:5mm; border-radius:50%; display:inline-flex; align-items:center; justify-content:center; flex:none; margin-top:.4mm }
.pillar h3{ margin-bottom:1.5mm } .pillar p{ font-size:8.6pt; margin:0; color:#b9bfca }
.pillar .ic{ width:7mm; height:.8mm; background:var(--gold); border-radius:1mm; margin-bottom:3mm }
table.jobs{ width:100%; border-collapse:collapse }
table.jobs td{ padding:2.3mm 0; border-bottom:1px solid var(--line); vertical-align:top }
table.jobs td:first-child{ font-family:Sora; font-weight:600; font-size:7.8pt; letter-spacing:.08em; color:var(--text); width:62mm }
table.jobs td:last-child{ color:var(--mut) }
table.jobs td.x{ width:8mm; color:var(--gold); font-family:Sora }
.where{ display:grid; grid-template-columns:repeat(4,1fr); gap:3mm }
.where .card{ padding:3.4mm 3.8mm }
.where h3{ font-size:7pt; margin-bottom:1mm } .where p{ font-size:8.2pt; margin:0; line-height:1.4; color:#b9bfca }
.folders{ display:grid; grid-template-columns:repeat(4,1fr); gap:3mm }
.folders .card{ padding:3.4mm 3.6mm }
.folders h3{ display:flex; justify-content:space-between; font-size:6.9pt }
.folders h3 span{ color:var(--dim) }
.folders ul{ list-style:none } .folders li{ font-size:8pt; color:#c3c8d2; padding:.7mm 0; border-top:1px solid #ffffff0d; line-height:1.3 }
.folders .card.cat{ border-color:#dabd7633 }
.recipe{ display:grid; grid-template-columns:1fr; gap:1.2mm }
.recipe .from{ font-size:7.8pt; color:var(--blue); font-family:Sora; letter-spacing:.06em }
.lab{ display:grid; grid-template-columns:repeat(3,1fr); gap:2.4mm }
.lab div{ font-family:Sora; font-size:7pt; letter-spacing:.14em; text-transform:uppercase; color:var(--mut); border:1px solid var(--line); border-radius:2mm; padding:2mm 2.6mm }
.lab div b{ color:var(--gold2); font-weight:600; display:block }
.short{ display:grid; grid-template-columns:1fr; }
.short div{ display:flex; justify-content:space-between; gap:6mm; padding:2.1mm 0; border-bottom:1px solid var(--line) }
.short div span:first-child{ font-family:Sora; font-weight:600; font-size:7.6pt; letter-spacing:.06em; color:var(--text) }
.short div span:last-child{ color:var(--mut); text-align:right }
.sec{ padding:4.4mm 0 4.6mm; border-top:1px solid var(--line) }
.sec h2{ font-size:9.6pt; color:var(--gold); letter-spacing:.22em; margin:0 0 1.8mm }
.sec p{ font-size:10.2pt; line-height:1.62; color:#c3c8d2; margin:0; max-width:160mm }
.sec p b{ color:#eef0f4; font-weight:600 }
.endlogo{ position:absolute !important; left:18mm; right:18mm; bottom:24mm; text-align:center }
.endlogo img{ width:70mm; display:block; margin:0 auto 4mm }
.endlogo p.thanks{ font-family:Inter; font-style:italic; font-size:9.4pt; letter-spacing:0; color:#9aa1ae; margin:0 0 9mm }
.endlogo p{ font-family:Sora; font-size:7.4pt; letter-spacing:.14em; color:var(--gold2); margin:0 }
.big-quote{ font-family:Sora; font-weight:300; font-size:15pt; line-height:1.45; color:var(--text) }
.big-quote em{ font-style:normal; color:var(--gold) }
"""

PAGES = []
def page(inner, cls='', seed=None, foot=True):
    n = len(PAGES) + 1
    f = ('<div class="foot"><span>SPACEX <b>%s</b> · MANUAL</span><span>%02d</span></div>' % (VERSION, n)) if foot else ''
    PAGES.append('<section class="page %s">%s%s%s</section>' % (cls, stars(seed if seed is not None else n), inner, f))

def kv(rows, wide=False):
    return '<dl class="kv%s">%s</dl>' % (' w' if wide else '', ''.join('<dt>%s</dt><dd>%s</dd>' % (e(k), e(v)) for k, v in rows))
def eyebrow(num, name): return '<div class="eyebrow">%s</div>' % name

# 1 cover --------------------------------------------------------------------
page('''
<div class="bg" style="background-image:url(%s)"></div>
<div class="top"><span>MISTYCAT STUDIOS</span><span>MANUAL · v%s</span></div>
<div class="logo"><img src="%s"></div>
<div class="claim"><div class="eyebrow">Stereo imaging, tuned by ear</div>
<h1>Width. Depth.<br><em>Dimension.</em></h1>
<p>Six stages, the Smart dice, a real L / C / R split. Wider, deeper, fuller - and still exactly where the song needs it.</p></div>
<div class="cpil">%s</div>
<div class="meta"><span class="pill">Version %s</span></div>
''' % (jpg('cover_bg'), VERSION, img('logo_a'),
       ''.join('<div><i></i><h3>%s</h3><p>%s</p></div>' % (e(a), e(b)) for a, b in COVER_PILLARS), VERSION), cls='cover', foot=False)

# 4 tour -------------------------------------------------------------------------
HOT = [(30, 30), (835, 88), (1270, 20), (1535, 20), (1320, 170), (1720, 170), (1968, 97),
       (600, 201), (115, 480), (400, 1105), (1150, 798)]
hs = ''.join('<span class="num" style="left:%.2f%%;top:%.2f%%">%d</span>' % (x / 20.0, y / 11.57, i + 1) for i, (x, y) in enumerate(HOT))
lg = ''.join('<div><span class="num">%d</span><span><b>%s</b>%s</span></div>' % (i + 1, e(a), e(b)) for i, (a, b) in enumerate(TOUR))
page('''%s<h1>The one-minute <em>tour.</em></h1>
<div class="hot"><img class="shot" src="%s">%s</div>
<div class="legend">%s</div><div class="sp"></div>
<div class="card"><h3>First result in one minute</h3><ol class="steps">%s</ol>
<p class="mut" style="margin:3mm 0 0">That is the whole workflow. The knobs are there for when you already know what you want.</p></div>''' % (
    eyebrow('', 'Overview'), img('full'), hs, lg, ''.join('<li>%s</li>' % e(x) for x in QUICK)))


# short manual: text pages -----------------------------------------------------------
import re as _re
from text_short import *
def rich(t):
    return _re.sub(r'\*\*(.+?)\*\*', r'<b>\1</b>', e(t))
def secs(rows):
    return ''.join('<div class="sec"><h2>%s</h2><p>%s</p></div>' % (e(a), rich(b)) for a, b in rows)

page('''%s<h1>The <em>sections.</em></h1><p class="lead">%s</p>%s''' % (
    eyebrow('', 'Signal path, top-left to bottom-right'), e(SECTIONS_INTRO), secs(SECTIONS)))

page('''%s<h1>Header, output, <em>presets.</em></h1>%s''' % (eyebrow('', 'Everything else'), secs(CONTROL)))

page('''%s<h1>Good to <em>know.</em></h1>
<div class="sec"><h2>Rule of thumb</h2><p>%s</p></div>
<div class="sec"><h2>Making space</h2><p>%s</p></div>
<div class="sec"><h2>Demo and activation</h2><p>%s</p></div>
<div class="endlogo"><p class="thanks">%s</p><img src="%s"><p>%s</p></div>''' % (
    eyebrow('', 'Tips'), e(RULE), e(CREATE), e(LICENCE), e(THANKS), img('logo_a'), e(CONTACT)))

HTML = '<!doctype html><html><head><meta charset="utf-8"><title>SpaceX Manual %s</title><style>%s</style></head><body>%s</body></html>' % (
    VERSION, CSS, '\n'.join(PAGES))

out_html = os.path.join(HERE, 'SpaceXManual_EN.html')
open(out_html, 'w').write(HTML)

if '--pdf' in sys.argv:
    from playwright.sync_api import sync_playwright
    out_pdf = sys.argv[sys.argv.index('--pdf') + 1]
    with sync_playwright() as p:
        b = p.chromium.launch()
        pg = b.new_page()
        pg.goto('file://' + out_html)
        pg.wait_for_timeout(600)
        pg.pdf(path=out_pdf, format='A4', print_background=True, prefer_css_page_size=True)
        b.close()
    print('pdf', out_pdf)
print('pages', len(PAGES))
