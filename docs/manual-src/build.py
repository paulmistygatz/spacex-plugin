# -*- coding: utf-8 -*-
# Space X - Handbuch-Generator.
# Schreibt manual_en.html und manual_de.html; die PDFs entstehen daraus per
# Druck aus dem Browser (Seitengroesse A4, Hintergrundgrafiken an).
#
# Die Texte liegen bewusst in zwei eigenen Dateien (text_en.py / text_de.py),
# damit man am Wortlaut arbeiten kann, ohne durch Layoutcode zu scrollen.

import base64, os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from text_en import EN
from text_de import DE

def img(name):
    with open(os.path.join(HERE, 'img', name + '.png'), 'rb') as f:
        return 'data:image/png;base64,' + base64.b64encode(f.read()).decode()

CSS = """
@page { size: A4; margin: 17mm 15mm 16mm 15mm; }
* { box-sizing: border-box; }
body { font-family: -apple-system, "Helvetica Neue", Helvetica, Arial, sans-serif;
       color: #1b1d23; font-size: 10.4pt; line-height: 1.52; margin: 0; }
h1 { font-size: 40pt; letter-spacing: 4px; margin: 0; font-weight: 700; }
h1 span { color: #6f5bd6; }
h2 { font-size: 15pt; margin: 22px 0 7px; letter-spacing: 1.1px; font-weight: 700;
     border-bottom: 1.5px solid #e2e4ea; padding-bottom: 5px; }
h3 { font-size: 11pt; margin: 14px 0 3px; font-weight: 700; }
p  { margin: 0 0 9px; }
.lead { font-size: 11.6pt; color: #2b2e37; }
.cover { text-align: center; padding-top: 44mm; }
.cover .sub { letter-spacing: 5px; font-size: 10pt; color: #6f5bd6; margin-top: 8px; }
.cover .sub2 { color: #8a8f9a; margin-top: 3px; }
.contact { margin-top: 14px; font-size: 9.4pt; color: #6f5bd6; font-weight: 600; }
.cover img { width: 100%; margin-top: 20mm; border-radius: 5px; }
.cover .meta { margin-top: 12mm; font-size: 9pt; color: #8a8f9a; }
.pb { page-break-before: always; }
.sec { page-break-inside: avoid; }
table { width: 100%; border-collapse: collapse; margin: 7px 0 10px; }
td { padding: 4px 8px; border-bottom: 1px solid #ecedf1; vertical-align: top; }
td.k { width: 30%; font-weight: 700; white-space: nowrap; }
.tip, .warn { padding: 8px 11px; border-radius: 4px; margin: 9px 0; font-size: 9.8pt; }
.tip  { background: #f1f0fb; border-left: 3px solid #6f5bd6; }
.warn { background: #fdf2f2; border-left: 3px solid #cf5a5a; }
.chain { display: flex; flex-wrap: wrap; gap: 5px; margin: 9px 0 12px; }
.chain span { background: #f2f3f6; border: 1px solid #e0e2e8; border-radius: 3px;
              padding: 3px 8px; font-size: 8.8pt; letter-spacing: 0.6px; }
.chain span.f { background: #f1f0fb; border-color: #d6d1f2; }
.row { display: flex; gap: 14px; align-items: flex-start; }
img.shot { width: 100%; border-radius: 4px; border: 1px solid #e2e4ea; }
img.w100 { width: 100%; } img.w60 { width: 60%; }
ol, ul { margin: 0 0 9px 18px; padding: 0; } li { margin-bottom: 3px; }
.footer-note { margin-top: 26px; font-size: 8.4pt; color: #9aa0ab; text-align: center; }
"""

def table(rows):
    out = ['<table>']
    for k, v in rows:
        out.append(f'<tr><td class="k">{k}</td><td>{v}</td></tr>')
    out.append('</table>')
    return ''.join(out)

def build(T, lang):
    L = lambda k: T[k]
    H = ['<html><head><meta charset="utf-8"><style>' + CSS + '</style></head><body>']

    # --- Titelseite ---
    H.append('<div class="cover">')
    H.append('<h1>SPACE <span>X</span></h1>')
    H.append(f'<div class="sub">{L("title_sub")}</div>')
    H.append(f'<div class="sub sub2">{L("title_sub2")}</div>')
    H.append(f'<p class="lead" style="margin-top:16px">{L("manual")}</p>')
    H.append(f'<img src="{img("overview")}">')
    H.append(f'<div class="meta">{L("cover_meta")}</div>')
    H.append('</div>')

    # --- Positionierung ---
    H.append(f'<h2 class="pb">{L("h_what")}</h2>')
    H.append(f'<p class="lead">{L("p_what1")}</p>')
    for k in ('p_what2', 'p_what2b', 'p_what3', 'p_what4'):
        H.append(f'<p>{L(k)}</p>')

    H.append(f'<h2>{L("h_instead")}</h2><p>{L("p_instead")}</p>' + table(L('instead')))
    H.append(f'<p>{L("p_instead2")}</p>')

    H.append(f'<h2>{L("h_where")}</h2>')
    for k in ('p_where1', 'p_where2'):
        H.append(f'<p>{L(k)}</p>')
    H.append(f'<div class="tip">{L("p_where3")}</div>')
    H.append(f'<div class="warn">{L("p_where4")}</div>')

    H.append(f'<h2>{L("h_quick")}</h2><ol>'
             + ''.join(f'<li>{x}</li>' for x in L('quick')) + '</ol>')
    H.append(f'<p>{L("p_quick")}</p>')

    H.append(f'<h2>{L("h_smart")}</h2>')
    for k in ('p_smart1', 'p_smart2', 'p_smart3'):
        H.append(f'<p>{L(k)}</p>')
    H.append(f'<div class="tip">{L("p_smart4")}</div>')
    H.append(table(L('smart')))

    # --- Signalweg ---
    H.append(f'<h2 class="pb">{L("h_layout")}</h2><p>{L("p_layout")}</p>')
    H.append('<div class="chain">'
             + ''.join(f'<span class="{"f" if i >= 7 else ""}">{c}</span>' for i, c in enumerate(L('chain')))
             + '</div>')
    H.append(f'<p>{L("p_chain_pol")}</p>')
    H.append(f'<p>{L("p_chain_filter")}</p>')
    H.append(f'<h3>{L("h_common")}</h3>' + table(L('common')))

    def section(hk, pk, rk, image, tip=None, warn=None, wide=False):
        H.append('<div class="sec">')
        H.append(f'<h2>{L(hk)}</h2>')
        if wide:
            H.append(f'<img class="shot w100" src="{img(image)}"><p>{L(pk)}</p>' + table(L(rk)))
        else:
            H.append('<div class="row"><div style="flex:0 0 40%">'
                     f'<img class="shot" src="{img(image)}">'
                     f'</div><div><p>{L(pk)}</p></div></div>')
            H.append(table(L(rk)))
        if tip:  H.append(f'<div class="tip">{L(tip)}</div>')
        if warn: H.append(f'<div class="warn">{L(warn)}</div>')
        H.append('</div>')

    section('h_galaxy', 'p_galaxy', 'galaxy', 'galaxy', tip='tip_galaxy')
    section('h_pol', 'p_pol', 'pol', 'polarity', warn='warn_pol')
    H.append(f'<p>{L("p_pol2")}</p>')
    section('h_tw', 'p_tw', 'tw', 'timewarp')
    section('h_dim', 'p_dim', 'dim', 'dimension', tip='tip_dim')
    section('h_hyp', 'p_hyp', 'hyp', 'hyperdrive', wide=True)
    section('h_vis', 'p_vis', 'vis', 'vision')
    section('h_raye', 'p_raye', 'raye', 'raye')

    H.append('<div class="sec">')
    H.append(f'<h2>{L("h_footer")}</h2><img class="shot w60" src="{img("footer")}">' + table(L('footer')))
    H.append('</div>')
    section('h_prism', 'p_prism', 'prism', 'prism', tip='tip_prism', wide=True)

    H.append('<div class="sec">')
    H.append(f'<h2>{L("h_star")}</h2>')
    H.append('<div class="row"><div style="flex:0 0 46%">'
             f'<img class="shot" src="{img("starfield")}">'
             f'</div><div><p>{L("p_star1")}</p><p>{L("p_star2")}</p></div></div>')
    H.append(table(L('star')))
    H.append('</div>')

    H.append(f'<h2>{L("h_look")}</h2><p>{L("p_look")}</p>')

    H.append(f'<h2 class="pb">{L("h_recipes")}</h2>')
    for title, body in L('recipes'):
        H.append(f'<h3>{title}</h3><p>{body}</p>')

    H.append(f'<h2>{L("h_short")}</h2>' + table(L('short')))

    H.append(f'<h2>{L("h_thanks")}</h2><p>{L("p_thanks1")}</p><p>{L("p_thanks2")}</p>')
    H.append(f'<p class="contact">{L("p_thanks3")}</p>')
    H.append('<p class="footer-note">Space X - ' + L('manual') + ' 1.0</p>')
    H.append('</body></html>')
    return '\n'.join(H)

for T, lang in ((EN, 'en'), (DE, 'de')):
    with open(os.path.join(HERE, f'manual_{lang}.html'), 'w') as f:
        f.write(build(T, lang))
print('html ok')
