#!/bin/bash
# Prueft ein gebautes SpaceX.vst3 auf die beiden Dinge, an denen es auf
# fremden Rechnern scheitert: fehlende Architektur und zu hohe
# Mindest-macOS-Version. Letzteres sieht man dem Plugin sonst nirgends an -
# es taucht auf dem anderen Mac einfach nicht in der Plugin-Liste auf.
BIN="${1:-/Library/Audio/Plug-Ins/VST3/SpaceX.vst3/Contents/MacOS/SpaceX}"
[ -f "$BIN" ] || { echo "Nicht gefunden: $BIN"; exit 1; }
echo "Datei:          $BIN"
echo -n "Architekturen:  "; lipo -archs "$BIN"
echo "Mindest-macOS:"
otool -l "$BIN" | grep -A4 LC_BUILD_VERSION | grep -E "minos|sdk" | sort -u | sed 's/^/  /'
