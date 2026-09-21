#!/bin/bash
# Baut die Layout-Varianten als EIGENE Plugins, damit man sie im Host
# nebeneinander oeffnen und vergleichen kann:
#   SpaceX-A  Tilt/Depth klein unter den Hauptreglern (Dreieck)
#   SpaceX-B  Tilt/Depth klein, gleicher Platz
#   SpaceX-C  alle sechs Regler gleich gross, kleiner
# Das normale SpaceX bleibt davon unberuehrt (das baut weiter install.sh).
#
#   ./build_variants.sh          -> alle drei
#   ./build_variants.sh B        -> nur B
set -e
cd "$(dirname "$0")"

DST="/Library/Audio/Plug-Ins/VST3"
# JUCE nicht dreimal neu herunterladen - die Kopie aus build/ wiederverwenden.
JUCE_SRC="$PWD/build/_deps/juce-src"
if [ ! -d "$JUCE_SRC" ]; then
    echo "Erst einmal ./install.sh laufen lassen (dabei wird JUCE geholt)."
    exit 1
fi
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
VARIANTS="${*:-A B C}"

for V in $VARIANTS; do
    echo ""
    echo "=== SpaceX-$V ==="
    cmake -S . -B "build_$V" -DSPACEX_VARIANT="$V" \
          -DFETCHCONTENT_SOURCE_DIR_JUCE="$JUCE_SRC" > /dev/null
    cmake --build "build_$V" --config Release -j "$JOBS"

    NAME="SpaceX-$V.vst3"
    SRC="$(find "build_$V" -type d -name "$NAME" -path '*VST3*' | head -1)"
    if [ -z "$SRC" ]; then
        echo "Nicht gefunden: $NAME"; exit 1
    fi
    sudo rm -rf "$DST/$NAME"
    sudo mkdir -p "$DST"
    sudo cp -R "$SRC" "$DST/"
    echo "OK -> $DST/$NAME"
done

echo ""
echo "Fertig. Im Host neu scannen - die Varianten heissen SpaceX-A, SpaceX-B, SpaceX-C."
