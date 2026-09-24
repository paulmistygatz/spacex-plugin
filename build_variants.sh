#!/bin/bash
# Baut die Vergleichs-Plugins als EIGENE Plugins, damit man sie im Host
# neben dem normalen SpaceX oeffnen kann:
#   VERALTET (Runde 54): es gibt keine Vergleichsbuilds mehr. Der normale
#   Build (./install.sh) IST die fertige Fassung und heisst SpaceX.
#   Dieses Skript bleibt nur, falls doch noch mal zwei Fassungen
#   nebeneinander gebraucht werden.
#   (paraCPU baut weiter Parallax mit den drei Reglern - nur auf Zuruf)
# Das normale SpaceX baut weiter install.sh.
#
#   ./build_variants.sh          -> beide
#   ./build_variants.sh paraCPU  -> die Fassung mit den drei Parallax-Reglern
set -e
cd "$(dirname "$0")"

DST="/Library/Audio/Plug-Ins/VST3"
JUCE_SRC="$PWD/build/_deps/juce-src"
if [ ! -d "$JUCE_SRC" ]; then
    echo "Erst einmal ./install.sh laufen lassen (dabei wird JUCE geholt)."
    exit 1
fi
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
VARIANTS="${*:-box}"   # Runde 98 (User): Vergleichsbuild SpaceXbox (alte Pillen)

# Alte Varianten (A/B/C, noV, SpaceFX, para, presets, raye) aus dem Plugin-Ordner raeumen.
sudo rm -rf "$DST/SpaceX-A.vst3" "$DST/SpaceX-B.vst3" "$DST/SpaceX-C.vst3" "$DST/SpaceXnoV.vst3" "$DST/SpaceFX.vst3" "$DST/SpaceXpara.vst3" "$DST/SpaceXpresets.vst3" "$DST/SpaceXclick.vst3" "$DST/SpaceXraye.vst3" "$DST/SpaceXraye1.vst3" "$DST/SpaceXraye2.vst3" "$DST/SpaceXin.vst3"

for V in $VARIANTS; do
    case "$V" in
        box)     PROD="SpaceXbox" ;;
        paraCPU) PROD="SpaceXparaCPU" ;;
        presets) PROD="SpaceXpresets" ;;
        dotsin)  PROD="SpaceXin" ;;
        dotsout) PROD="SpaceXout" ;;
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
echo "Fertig. Im Host neu scannen."
