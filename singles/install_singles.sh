#!/bin/bash
# Einzel-Plugins bauen und systemweit installieren (VST3 + AU).
#   ./singles/install_singles.sh                 -> alle sechs
#   ./singles/install_singles.sh Micropitch LCR  -> nur diese
# Namen: Micropitch LCR Polarity MidSide Autopan Phaser
# Fragt einmal nach dem Passwort (Kopieren nach /Library braucht sudo).
set -e
cd "$(dirname "$0")"

ALL="Micropitch LCR Polarity MidSide Autopan Phaser"
TARGETS="${*:-$ALL}"

if [ ! -f build/CMakeCache.txt ]; then
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
fi
BUILD=""
for t in $TARGETS; do BUILD="$BUILD ${t}_VST3 ${t}_AU"; done
cmake --build build --config Release --parallel --target $BUILD

VST3_DST="/Library/Audio/Plug-Ins/VST3"
AU_DST="/Library/Audio/Plug-Ins/Components"
sudo mkdir -p "$VST3_DST" "$AU_DST"

for t in $TARGETS; do
    ART="build/${t}_artefacts"
    [ -d "$ART/Release" ] && ART="$ART/Release"
    # Name im Host (Bundle-Name) kann vom Target abweichen: MidSide -> Mid-Side
    VST3_SRC=$(ls -d "$ART"/VST3/*.vst3 | head -1)
    AU_SRC=$(ls -d "$ART"/AU/*.component 2>/dev/null | head -1 || true)
    NAME=$(basename "$VST3_SRC")
    sudo rm -rf "$VST3_DST/$NAME"
    sudo cp -R "$VST3_SRC" "$VST3_DST/"
    echo "OK -> $VST3_DST/$NAME"
    if [ -n "$AU_SRC" ]; then
        AUNAME=$(basename "$AU_SRC")
        sudo rm -rf "$AU_DST/$AUNAME"
        sudo cp -R "$AU_SRC" "$AU_DST/"
        echo "OK -> $AU_DST/$AUNAME"
    fi
done

killall -9 AudioComponentRegistrar 2>/dev/null || true
