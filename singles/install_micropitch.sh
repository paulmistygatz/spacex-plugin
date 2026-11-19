#!/bin/bash
# Micropitch bauen und systemweit installieren (VST3 + AU).
# Aufruf aus dem Terminal:  ./singles/install_micropitch.sh
# Fragt einmal nach dem Passwort (Kopieren nach /Library braucht sudo).
set -e
cd "$(dirname "$0")"

if [ ! -f build/CMakeCache.txt ]; then
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
fi
cmake --build build --config Release --target Micropitch_VST3 Micropitch_AU --parallel

ART="build/Micropitch_artefacts"
pick () { [ -d "$ART/Release/$1" ] && echo "$ART/Release/$1" || echo "$ART/$1"; }

VST3_SRC="$(pick VST3/Micropitch.vst3)"
VST3_DST="/Library/Audio/Plug-Ins/VST3"
sudo rm -rf "$VST3_DST/Micropitch.vst3"
sudo mkdir -p "$VST3_DST"
sudo cp -R "$VST3_SRC" "$VST3_DST/"
echo "OK -> $VST3_DST/Micropitch.vst3"

AU_SRC="$(pick AU/Micropitch.component)"
AU_DST="/Library/Audio/Plug-Ins/Components"
if [ -d "$AU_SRC" ]; then
    sudo rm -rf "$AU_DST/Micropitch.component"
    sudo mkdir -p "$AU_DST"
    sudo cp -R "$AU_SRC" "$AU_DST/"
    killall -9 AudioComponentRegistrar 2>/dev/null || true
    echo "OK -> $AU_DST/Micropitch.component"
fi

../tools/check_binary.sh "$VST3_DST/Micropitch.vst3/Contents/MacOS/Micropitch" 2>/dev/null || true
