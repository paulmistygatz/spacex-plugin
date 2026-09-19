# Erzeugt Source/GUI/SpaceAssets.{h,cpp} aus den Bildern in diesem Ordner.
# Aufruf: python3 gen_space_assets.py   (aus docs/assets-src heraus)
import os
names=['bg_milkyway','bg_nebula','bg_purplesky']
files=[(n,n+'.jpg') for n in names]
for g in ['moon','earth','earthnight','redhalf']:
    files += [(f'grav_{g}_rgb',f'grav_{g}_rgb.jpg'),(f'grav_{g}_a',f'grav_{g}_a.png')]
for sp in ['mars','earth','yellowmoon','redmoon','neptune','sun','hotjup','darkblue','eclipse','eclipseblue','venus','mercury','saturn','redgiant','iss']:
    files += [(f'spr_{sp}_rgb',f'spr_{sp}_rgb.jpg'),(f'spr_{sp}_a',f'spr_{sp}_a.png')]
h=['#pragma once','','// Eingebettete Fotos fuer das Sternenfeld (Hintergruende + Gravity-Planeten).','// Generiert aus docs/assets-src (Python), nicht von Hand bearbeiten.','// Gravity-Assets: RGB als JPEG + Alpha als 8-Bit-PNG, werden beim Laden','// zusammengesetzt (kleiner als ein RGBA-PNG).','','namespace SpaceAssets','{']
cpp=['#include "SpaceAssets.h"','','namespace SpaceAssets','{']
for name,fn in files:
    data=open(fn,'rb').read()
    h.append(f'    extern const unsigned char {name}[];'); h.append(f'    extern const int {name}Size;')
    cpp.append(f'const int {name}Size = {len(data)};'); cpp.append(f'const unsigned char {name}[] = {{')
    for i in range(0,len(data),40): cpp.append('    '+','.join(str(b) for b in data[i:i+40])+',')
    cpp.append('};')
h.append('}'); cpp.append('}')
out='../../Source/GUI/'
open(out+'SpaceAssets.h','w').write('\n'.join(h)+'\n'); open(out+'SpaceAssets.cpp','w').write('\n'.join(cpp)+'\n')
