#!/bin/bash
# SpaceX bauen und systemweit installieren.
#
# Ziel ist bewusst /Library/Audio/Plug-Ins/VST3 (fuer alle Benutzer) statt
# ~/Library/... - dieser Ordner gehoert root, deshalb braucht der Kopierschritt
# sudo und fragt einmal nach dem Passwort. Gebaut wird OHNE sudo, damit im
# build-Ordner keine root-Dateien landen.
set -e
cd "$(dirname "$0")"

NAME="SpaceX.vst3"
SRC="build/LCRMSPlugin_artefacts/VST3/$NAME"
DST="/Library/Audio/Plug-Ins/VST3"

# Review 1.0.1: build/ bei Bedarf selbst anlegen, als Release. So klappt auch
# "rm -rf build && ./install.sh" (BEFEHLE.md). --parallel nutzt alle Kerne.
if [ ! -f build/CMakeCache.txt ]; then
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
fi
cmake --build build --config Release --parallel

# Review 1.0.1: Als Release legt JUCE das Plugin unter .../Release/VST3 ab. Der
# alte Ordner .../VST3 (ohne Release) stammt noch vom Build ohne Optimierung und
# wird nicht mehr aktualisiert - also zuerst den Release-Pfad nehmen.
if [ -d "build/LCRMSPlugin_artefacts/Release/VST3/$NAME" ]; then
    SRC="build/LCRMSPlugin_artefacts/Release/VST3/$NAME"
fi
echo "==> Quelle: $SRC"

# Alte Fassung erst weg, sonst bleiben geloeschte Dateien im Bundle liegen.
sudo rm -rf "$DST/$NAME"
sudo mkdir -p "$DST"
sudo cp -R "$SRC" "$DST/"

# Doppelte Eintraege vermeiden: liegt SpaceX auch noch in der User-Library,
# laedt der Host im Zweifel die alte Fassung. Auch der fruehere Name mit
# Leerzeichen wird hier entfernt.
rm -rf "$HOME/Library/Audio/Plug-Ins/VST3/$NAME" \
       "$HOME/Library/Audio/Plug-Ins/VST3/Space X.vst3" 2>/dev/null || true

# Die Vergleichsbuilds sind Geschichte (Runde 54): was sie konnten, kann jetzt
# SpaceX selbst. Sie werden hier entfernt, damit im Host nur noch EIN SpaceX
# auftaucht und man nie wieder das falsche oeffnet.
sudo rm -rf "$DST/SpaceXout.vst3" "$DST/SpaceXin.vst3" \
            "$DST/SpaceXraye1.vst3" "$DST/SpaceXraye2.vst3" \
            "$DST/SpaceXparaCPU.vst3" "$DST/SpaceXpresets.vst3" \
            "$DST/SpaceXclick.vst3" "$DST/SpaceXpara.vst3" \
            "$DST/SpaceXdiag.vst3" 2>/dev/null || true

echo "OK -> $DST/$NAME"

# 1.0.1: Audio Unit (nur Mac) nach /Library/Audio/Plug-Ins/Components.
AU_NAME="SpaceX.component"
AU_SRC="build/LCRMSPlugin_artefacts/Release/AU/$AU_NAME"
[ -d "$AU_SRC" ] || AU_SRC="build/LCRMSPlugin_artefacts/AU/$AU_NAME"
AU_DST="/Library/Audio/Plug-Ins/Components"
if [ -d "$AU_SRC" ]; then
    sudo rm -rf "$AU_DST/$AU_NAME"
    sudo mkdir -p "$AU_DST"
    sudo cp -R "$AU_SRC" "$AU_DST/"
    rm -rf "$HOME/Library/Audio/Plug-Ins/Components/$AU_NAME" 2>/dev/null || true
    # macOS merkt sich Audio Units - ohne das sieht Logic die neue Fassung oft erst nach einem Neustart.
    killall -9 AudioComponentRegistrar 2>/dev/null || true
    echo "OK -> $AU_DST/$AU_NAME"
else
    echo "Hinweis: kein AU gebaut ($AU_SRC fehlt)."
fi

# Kurzer Selbsttest: beide Architekturen drin, und wie alt darf das Ziel-macOS
# sein? Ohne das faellt "laeuft nur auf meinem Rechner" erst beim Kollegen auf.
"$(dirname "$0")/tools/check_binary.sh" "$DST/$NAME/Contents/MacOS/SpaceX" 2>/dev/null || true
