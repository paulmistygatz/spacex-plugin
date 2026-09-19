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

cmake --build build --config Release

# Alte Fassung erst weg, sonst bleiben geloeschte Dateien im Bundle liegen.
sudo rm -rf "$DST/$NAME"
sudo mkdir -p "$DST"
sudo cp -R "$SRC" "$DST/"

# Doppelte Eintraege vermeiden: liegt SpaceX auch noch in der User-Library,
# laedt der Host im Zweifel die alte Fassung. Auch der fruehere Name mit
# Leerzeichen wird hier entfernt.
rm -rf "$HOME/Library/Audio/Plug-Ins/VST3/$NAME" \
       "$HOME/Library/Audio/Plug-Ins/VST3/Space X.vst3" 2>/dev/null || true

echo "OK -> $DST/$NAME"
