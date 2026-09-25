# -*- coding: utf-8 -*-
# SpaceX 1.0.1 manual builder: HTML (fonts + images embedded) -> PDF via Playwright/Chromium.
import base64, html, os, random, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from text101 import *

HERE = os.path.dirname(os.path.abspath(__file__))
def b64(path, mime):
    with open(os.path.join(HERE, path), 'rb') as f:
        return 'data:%s;base64,%s' % (mime, base64.b64encode(f.read()).decode())
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
body{ font-family:Inter; color:var(--text); font-size:9.6pt; line-height:1.55; -webkit-print-color-adjust:exact; print-color-adjust:exact }
.page{ width:210mm; height:297mm; position:relative; overflow:hidden; page-break-after:always; padding:20mm 18mm 22mm;
       background: radial-gradient(120mm 90mm at 100% 0%, #dabd7612, transparent 70%),
                   radial-gradient(140mm 110mm at 0% 100%, #7fb0ff0f, transparent 70%), var(--bg) }
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
.shot{ display:block; width:100%; border-radius:3mm; border:1px solid #ffffff1c; box-shadow:0 3mm 9mm #0009 }
.grid2{ display:grid; grid-template-columns:1fr 1fr; gap:5mm }
.grid3{ display:grid; grid-template-columns:repeat(3,1fr); gap:4mm }
.grid4{ display:grid; grid-template-columns:repeat(4,1fr); gap:3.5mm }
.sp{ height:6mm } .sp2{ height:3mm }
dl.kv{ display:grid; grid-template-columns:31mm 1fr; column-gap:4mm; row-gap:2.1mm }
dl.kv.w{ grid-template-columns:44mm 1fr }
dl.kv dt{ font-family:Sora; font-weight:600; font-size:7.4pt; letter-spacing:.14em; text-transform:uppercase; color:var(--gold2); padding-top:.5mm }
dl.kv dd{ color:#c9ced8 }
.tip{ border-left:.7mm solid var(--gold); padding:2.2mm 0 2.2mm 4mm; background:linear-gradient(90deg,#dabd7614,transparent); border-radius:0 2mm 2mm 0; color:var(--text) }
.tip b, .warn b{ font-family:Sora; font-size:7pt; letter-spacing:.24em; color:var(--gold); display:block; margin-bottom:.8mm; font-weight:600 }
.warn{ border-left:.7mm solid #ff8a7a; padding:2.2mm 0 2.2mm 4mm; background:linear-gradient(90deg,#ff8a7a14,transparent); border-radius:0 2mm 2mm 0 }
.warn b{ color:#ff9d8f }
.pill{ font-family:Sora; font-weight:600; font-size:6.8pt; letter-spacing:.2em; text-transform:uppercase; padding:1.6mm 3mm; border-radius:10mm;
       border:1px solid #dabd7655; color:var(--gold2); background:#dabd7610; display:inline-block; white-space:nowrap }
.pill.b{ border-color:#7fb0ff55; color:#cfe0ff; background:#7fb0ff10 }
.pill.x{ border-color:#ffffff22; color:var(--mut); background:#ffffff06 }
.num{ display:inline-flex; width:5.2mm; height:5.2mm; border-radius:50%; background:var(--gold); color:#141004; font-family:Sora; font-weight:700;
      font-size:6.8pt; align-items:center; justify-content:center; flex:none }
/* cover */
.cover{ padding:0 }
.cover .bg{ position:absolute; inset:0; background-size:cover; background-position:center 40%; opacity:.55; filter:saturate(1.1) }
.cover .veil{ position:absolute; inset:0; background:
   linear-gradient(180deg, #0b0d12 0%, #0b0d12cc 22%, #0b0d1200 48%, #0b0d1299 70%, #0b0d12 92%),
   radial-gradient(90mm 70mm at 50% 45%, #dabd7622, transparent 70%) }
.cover .top{ position:absolute; top:20mm; left:18mm; right:18mm; display:flex; justify-content:space-between; font-family:Sora; font-size:7pt; letter-spacing:.34em; color:var(--mut) }
.cover .logo{ position:absolute; top:40mm; left:0; right:0; text-align:center }
.cover .logo img{ width:118mm }
.cover .claim{ position:absolute; left:18mm; right:18mm; bottom:36mm }
.cover .claim h1{ font-size:34pt; letter-spacing:.05em; margin-bottom:4mm }
.cover .claim p{ font-size:11.5pt; color:#d5d9e1; max-width:130mm; font-weight:300 }
.cover .meta{ position:absolute; left:18mm; right:18mm; bottom:16mm; display:flex; gap:3mm; align-items:center }
.cover .meta .sp-auto{ flex:1 }
/* chain */
.chain{ display:flex; flex-wrap:wrap; gap:2.2mm 1.4mm; align-items:center }
.chain .arr{ color:var(--dim); font-size:9pt }
/* tour hotspots */
.hot{ position:relative }
.hot .num{ position:absolute; transform:translate(-50%,-50%); box-shadow:0 0 0 1.2mm #0b0d12aa, 0 0 5mm #dabd76aa }
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
.pillar .ic{ width:7mm; height:.8mm; background:var(--gold); border-radius:1mm; margin-bottom:3mm; box-shadow:0 0 3mm #dabd76 }
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
def eyebrow(num, name): return '<div class="eyebrow"><i>%s</i>%s</div>' % (num, name)

# 1 cover --------------------------------------------------------------------
page('''
<div class="bg" style="background-image:url(%s)"></div><div class="veil"></div>
<div class="top"><span>MISTYCAT STUDIOS</span><span>MANUAL · v%s</span></div>
<div class="logo"><img src="%s"></div>
<div class="claim"><div class="eyebrow">Stereo imaging, tuned by ear</div>
<h1>Depth<br>without <em>reverb.</em></h1>
<p>Six stages, the Smart dice, a real L / C / R split. Wider, deeper, fuller - and still exactly where the song needs it.</p></div>
<div class="meta"><span class="pill">Version %s</span><span class="pill b">VST3</span><span class="pill x">Low CPU</span><span class="pill x">Zero latency without LCR</span></div>
''' % (img('starfield'), VERSION, img('logo_a'), VERSION), cls='cover', foot=False)

# 2 intro ----------------------------------------------------------------------
pill = ''.join('<div class="card pillar"><div class="ic"></div><h3>%s</h3><p>%s</p></div>' % (e(a), e(b)) for a, b in PILLARS)
page('''%s<h1>Space is the<br><em>last free room</em><br>in your mix.</h1>
<p class="lead">%s</p>%s
<div class="sp"></div><div class="grid4">%s</div><div class="sp"></div>
<div class="card"><h3>Who built this</h3>%s</div>''' % (
    eyebrow('01', 'What it is'), e(INTRO_LEAD), ''.join('<p>%s</p>' % e(x) for x in INTRO), pill,
    ''.join('<p>%s</p>' % e(x) for x in BIO)))

# 3 jobs + where ---------------------------------------------------------------
rows = ''.join('<tr><td class="x">%02d</td><td>%s</td><td>instead of %s</td></tr>' % (i + 1, e(a), e(b)) for i, (a, b) in enumerate(JOBS))
wh = ''.join('<div class="card"><h3>%s</h3><p>%s</p></div>' % (e(a), e(b)) for a, b in WHERE)
wh += '<div class="card" style="border-color:#dabd7644;background:linear-gradient(180deg,#221e14,#15130e)"><h3>35 presets</h3><p>In eight folders, from vocals to drums. Page 11.</p></div>'
page('''%s<h1>One plugin.<br><em>Up to five jobs.</em></h1>
<table class="jobs">%s</table><div class="sp2"></div>
<p class="big-quote" style="margin-top:4mm">%s</p>
<div class="sp"></div>%s<h1 style="font-size:17pt">Where it belongs</h1>
<div class="where">%s</div><div class="sp"></div><div class="tip"><b>RULE OF THUMB</b>%s</div>''' % (
    eyebrow('02', 'Why it exists'), rows, e(JOBS_NOTE), eyebrow('', 'Built for vocals. Great on everything else.'), wh, e(WHERE_RULE)))

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
    eyebrow('03', 'Overview'), img('full'), hs, lg, ''.join('<li>%s</li>' % e(x) for x in QUICK)))

# 5 smart ----------------------------------------------------------------------------
prof = ''.join('<div style="display:flex;gap:3mm;padding:1.5mm 0;border-bottom:1px solid #ffffff0d"><span style="font-family:Sora;font-weight:600;font-size:7.4pt;letter-spacing:.14em;text-transform:uppercase;color:var(--gold2);width:27mm;flex:none">%s</span><span class="mut">%s</span></div>' % (e(a), e(b)) for a, b in PROFILES)
page('''%s<h1>The Smart <em>engine.</em></h1>
<p class="lead">Tell it what you are working on. Roll the Smart dice. Everything else is taste.</p>
<div class="grid2" style="grid-template-columns:62mm 1fr;align-items:center">
 <div style="display:grid;grid-template-columns:1fr 1fr;gap:3mm"><img class="shot" src="%s"><img class="shot" src="%s"></div>
 <div><h3>Smart profile</h3>%s<p class="mut" style="margin-top:2mm;font-size:8.4pt">Click for the next one, Cmd-click for the previous one, or hit a dot to jump straight there. The line above the starfield says what the profile does.</p></div>
</div><div class="sp"></div>
<img class="shot" src="%s" style="width:72mm;margin-bottom:4mm">
%s<div class="sp"></div>
<div class="card"><h3>The dice knows what is on</h3><p style="margin:0">Each face is the number of active sections - Polarity, Micropitch, Mid-Side, Autopan, Phaser, and the LCR Matrix while its engine is armed. Click it and it rolls for a moment before it lands.</p></div>
<div class="sp"></div><div class="tip"><b>NOT A RANDOMISER</b>%s</div>''' % (
    eyebrow('04', 'Header'), img('profile_lead'), img('profile_backings'), prof, img('liverow'), kv(HEADER), e(SMART_NOTE)))

# 6 path ---------------------------------------------------------------------------
ch = []
for i, c in enumerate(CHAIN):
    cls = 'pill b' if c in ('IN', 'OUT') else ('pill x' if 'MIX' in c else 'pill')
    ch.append('<span class="%s">%s</span>' % (cls, e(c)))
    if i < len(CHAIN) - 1: ch.append('<span class="arr">→</span>')
page('''%s<h1>The signal <em>path.</em></h1>
<div class="card"><div class="chain">%s</div></div><div class="sp2"></div>
<p>%s</p><div class="sp"></div>
<div class="grid2"><div><h2>On every section</h2><p class="mut">Six sections, one set of rules.</p>%s</div>
<div><h2>Mid-side vs. L / C / R</h2><p>%s</p></div></div>
<div class="sp2"></div><img class="shot" src="%s" style="width:148mm;margin:0 auto">
<p class="mut" style="margin-top:3mm;font-size:8.4pt;text-align:center">Backings profile: sections the dice left out go dark - you always see what the roll actually did.</p>''' % (
    eyebrow('05', 'Architecture'), ''.join(ch), e(CHAIN_NOTE), kv(COMMON, True), e(MS_VS_LCR), img('full_backings')))

# 7 LCR + Polarity ------------------------------------------------------------------
page('''%s<h1>LCR Matrix</h1>
<div class="grid2" style="grid-template-columns:96mm 1fr;align-items:start"><img class="shot" src="%s">
<div><p>Pulls the centre out of the stereo image and treats left, centre and right as three parts. It only runs while the engine is armed - the LCR button in the header.</p></div></div>
<div class="sp2"></div>%s<div class="sp2"></div><div class="tip"><b>TIP</b>%s</div>
<div class="sp"></div><div class="sp"></div>
%s<h1>Polarity</h1>
<div class="grid2" style="grid-template-columns:62mm 1fr;align-items:start"><img class="shot" src="%s">
<div><p>Flips the phase of one channel. The most drastic thing in the plugin - and the one that makes the biggest difference.</p>%s</div></div>
<div class="sp"></div><div class="warn"><b>MONO</b>%s</div>''' % (
    eyebrow('06', 'Section 1'), img('lcr'), kv(LCR), e(LCR_TIP), eyebrow('07', 'Section 2'), img('polarity'), kv(POL), e(POL_WARN)))

# 8 Micropitch + Mid-Side ------------------------------------------------------------
page('''%s<h1>Micropitch</h1><p>%s</p>
<div class="grid2"><img class="shot" src="%s"><img class="shot" src="%s"></div><div class="sp2"></div>%s
<div class="sp"></div>
%s<h1>Mid-Side</h1><p>%s</p>
<div class="grid2"><img class="shot" src="%s"><img class="shot" src="%s"></div><div class="sp2"></div>%s
<div class="sp2"></div><div class="tip"><b>TIP</b>%s</div>''' % (
    eyebrow('08', 'Section 3'), e(MP_TEXT), img('micropitch'), img('micropitch_double'), kv(MP),
    eyebrow('09', 'Section 4'), e(MS_TEXT), img('midside'), img('midside_focus'), kv(MS), e(MS_TIP)))

# 9 Autopan + Phaser ---------------------------------------------------------------
page('''%s<h1>Autopan</h1>
<div class="grid2" style="grid-template-columns:92mm 1fr;align-items:start"><img class="shot" src="%s"><p>%s</p></div>
<div class="sp2"></div>%s<div class="sp"></div><div class="sp"></div>
%s<h1>Phaser</h1>
<div class="grid2" style="grid-template-columns:62mm 1fr;align-items:start"><img class="shot" src="%s"><p>%s</p></div>
<div class="sp2"></div>%s''' % (
    eyebrow('10', 'Section 5'), img('autopan'), e(AP_TEXT), kv(AP), eyebrow('11', 'Section 6'), img('phaser'), e(PH_TEXT), kv(PH)))

# 10 Output + starfield --------------------------------------------------------------
page('''%s<h1>Output</h1><img class="shot" src="%s"><div class="sp2"></div>%s
<div class="sp"></div>
%s<div class="grid2" style="grid-template-columns:70mm 1fr;align-items:start"><img class="shot" src="%s">
<div><h1 style="font-size:19pt">The starfield</h1><p>%s</p>%s</div></div>
<div class="sp"></div><img class="shot" src="%s" style="width:100mm;margin-bottom:3mm"><p class="mut">%s</p>''' % (
    eyebrow('12', 'Bottom left'), img('footer'), kv(OUT), eyebrow('13', 'Goniometer'), img('starfield'), e(STAR_NOTE),
    kv(STAR), img('infoline'), e(HELP)))

# 11 presets -----------------------------------------------------------------------------
fo = ''
for i, (name, items) in enumerate(PRESET_FOLDERS):
    fo += '<div class="card%s"><h3>%s<span>%d</span></h3><ul>%s</ul></div>' % (
        ' cat' if i < 4 else '', e(name), len(items), ''.join('<li>%s</li>' % e(x) for x in items))
page('''%s<h1>Presets.<br><em>35 starting points.</em></h1>
<p>Smart categories first, then instrument folders, then your own. Every preset was set by ear on real sessions - start there, then make it yours.</p>
<div class="folders">%s</div><div class="sp"></div>
<img class="shot" src="%s" style="margin-bottom:4mm">%s<div class="sp2"></div><p class="mut">%s</p>''' % (
    eyebrow('14', 'Library'), fo, img('header'), kv(PRESETS, True), e(PRESET_NOTE)))

# 12 settings + licence ---------------------------------------------------------------
lab = ''.join('<div>%s<b>%s</b></div>' % (e(a), e(b)) for a, b in LABELS)
page('''%s<h1>Settings.</h1><p>The icon with the three sliders, top right. Changes apply live; Save keeps them, Cancel takes everything back.</p>
<div class="sp2"></div>%s<div class="sp"></div>
<div class="card"><h3>SpaceX Labels</h3><p>Prefer the story over the engineering? Switch SpaceX Labels on and the sections carry their original names:</p><div class="lab">%s</div></div>
<div class="sp"></div><div class="sp"></div>%s<h1 style="font-size:19pt">Demo and activation</h1>%s''' % (
    eyebrow('15', 'Preferences'), kv(SETTINGS, True), lab, eyebrow('16', 'Licence'), ''.join('<p>%s</p>' % e(x) for x in DEMO)))

# 13 recipes ---------------------------------------------------------------------------------
rc = ''.join('<div class="card recipe" style="padding:6mm 6mm"><h3 style="font-size:8.4pt">%s</h3><div class="from">Start from · %s</div><p style="margin:0;font-size:10pt;line-height:1.55">%s</p></div>' % (e(a), e(b), e(c)) for a, b, c in RECIPES)
page('''%s<h1>Six settings<br><em>that work.</em></h1><p class="lead">Load the preset, or build it by hand - both roads end in the same place.</p>
<div class="grid2" style="gap:6mm">%s</div><div class="sp"></div><div class="sp"></div>
<p class="big-quote">The more room already sits in front of SpaceX, <em>the further you can push it.</em></p>''' % (eyebrow('17', 'Recipes'), rc))

# 14 shortcuts -------------------------------------------------------------------------------
sh = ''.join('<div><span>%s</span><span>%s</span></div>' % (e(a), e(b)) for a, b in SHORT)
page('''%s<h1>Clicks worth <em>knowing.</em></h1><p class="lead">The basics you will guess. These you will not.</p>
<div class="short" style="font-size:10pt">%s</div><div class="sp"></div><div class="sp"></div>
<div class="grid3"><div class="card pillar"><div class="ic"></div><h3>Take the Tour</h3><p>A short guided walk through every section. In the settings - and on first launch.</p></div>
<div class="card pillar"><div class="ic"></div><h3>Info line</h3><p>The ? at the bottom left. Hover anything and it says what it is.</p></div>
<div class="card pillar"><div class="ic"></div><h3>Back panel</h3><p>Click the logo. Who built this, how to reach him, which copy this is.</p></div></div>''' % (
    eyebrow('18', 'Shortcuts'), sh))

# 15 credits ---------------------------------------------------------------------------------
page('''<div class="bg" style="background-image:url(%s);opacity:.35;background-position:center 30%%"></div><div class="veil"></div>
<div style="position:absolute;left:18mm;right:18mm;top:34mm">%s<h1 style="font-size:30pt">Made by <em>listening.</em></h1>%s</div>
<div style="position:absolute;left:18mm;right:18mm;bottom:28mm;text-align:center"><img src="%s" style="width:92mm;display:block;margin:0 auto 6mm">
<p style="font-family:Sora;font-size:7.6pt;letter-spacing:.16em;color:var(--gold2);margin:0">%s</p></div>''' % (
    img('starfield'), eyebrow('19', 'Credits'), ''.join('<p class="lead" style="font-size:11pt">%s</p>' % e(x) for x in CREDITS), img('logo_a'), e(CONTACT)), cls='cover')

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
