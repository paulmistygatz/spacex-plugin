#!/bin/bash
# Baut die Vergleichs-Plugins als EIGENE Plugins, damit man sie im Host
# neben dem normalen SpaceX oeffnen kann:
#   SpaceXparaCPU  Parallax mit drei Reglern (Drift/Shift/Tilt)
#   SpaceXclick    Parallax: Amount + Klick-Knopf
#   SpaceXraye     wie click, dazu RAYE mit Amount + Charakter
# Das normale SpaceX baut weiter install.sh.
#
#   ./build_variants.sh          -> alle drei
#   ./build_variants.sh paraCPU  -> nur SpaceXparaCPU
set -e
cd "$(dirname "$0")"

DST="/Library/Audio/Plug-Ins/VST3"
JUCE_SRC="$PWD/build/_deps/juce-src"
if [ ! -d "$JUCE_SRC" ]; then
    echo "Erst einmal ./install.sh laufen lassen (dabei wird JUCE geholt)."
    exit 1
fi
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
VARIANTS="${*:-paraCPU click raye}"

# Alte Varianten (A/B/C, noV, SpaceFX, para, presets, raye) aus dem Plugin-Ordner raeumen.
sudo rm -rf "$DST/SpaceX-A.vst3" "$DST/SpaceX-B.vst3" "$DST/SpaceX-C.vst3" "$DST/SpaceXnoV.vst3" "$DST/SpaceFX.vst3" "$DST/SpaceXpara.vst3" "$DST/SpaceXpresets.vst3"

for V in $VARIANTS; do
    case "$V" in
        paraCPU) PROD="SpaceXparaCPU" ;;
        presets) PROD="SpaceXpresets" ;;
        click)   PROD="SpaceXclick" ;;
        raye)    PROD="SpaceXraye" ;;
        *)   PROD="SpaceX-$V" ;;
    esac
    echo ""
    echo "=== $PROD ==="
    cmake -S . -B "build_$V" -DSPACEX_VARIANT="$V" \
          -DFETCHCONTENT_SOURCE_DIR_JUCE="$JUCE_SRC" > /dev/null
    cmake --build "build_$V" --config Release -j "$JOBS"

    NAME="$PROD.vst3"
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
echo "Fertig. Im Host neu scannen: SpaceXparaCPU, SpaceXclick, SpaceXraye."
