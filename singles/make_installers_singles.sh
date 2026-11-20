#!/bin/bash
# Je ein macOS-Installer (.pkg) pro Einzel-Plugin (VST3 + AU).
#   ./singles/make_installers_singles.sh                 -> alle sechs
#   ./singles/make_installers_singles.sh Micropitch LCR  -> nur diese
# Ergebnis: ~/PROJEKTE/CLAUDE/_Installer/<Name> <Version>.pkg
# Unsigniert, solange INSTALLER_IDENTITY leer ist (siehe make_installer_mac.sh).
set -e
cd "$(dirname "$0")"

ALL="Micropitch LCR Polarity MidSide Autopan Phaser"
TARGETS="${*:-$ALL}"
INSTALLER_DIR="$HOME/PROJEKTE/CLAUDE/_Installer"
INSTALLER_IDENTITY=""          # z. B. "Developer ID Installer: Paul Misty (TEAMID)"
KEYCHAIN_PROFILE="notary-profile"
VERSION="$(sed -n 's/^project(MistycatSingles VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)"
[ -n "$VERSION" ] || { echo "Fehler: Version nicht gefunden."; exit 1; }

[ -f build/CMakeCache.txt ] || cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
ST=""
for t in $TARGETS; do ST="$ST ${t}_VST3 ${t}_AU"; done
cmake --build build --config Release --parallel --target $ST

mkdir -p "$INSTALLER_DIR"
for t in $TARGETS; do
    ART="build/${t}_artefacts"
    [ -d "$ART/Release" ] && ART="$ART/Release"
    V=$(ls -d "$ART"/VST3/*.vst3 | head -1)
    NAME=$(basename "$V" .vst3)                 # Name im Host, z. B. Mid-Side
    LOWER=$(echo "$t" | tr '[:upper:]' '[:lower:]')
    ROOT="build/pkg-$t/root"; TMP="build/pkg-$t"
    rm -rf "$TMP"
    mkdir -p "$ROOT/Library/Audio/Plug-Ins/VST3" "$ROOT/Library/Audio/Plug-Ins/Components"
    cp -R "$V" "$ROOT/Library/Audio/Plug-Ins/VST3/"
    A=$(ls -d "$ART"/AU/*.component 2>/dev/null | head -1 || true)
    if [ -n "$A" ]; then cp -R "$A" "$ROOT/Library/Audio/Plug-Ins/Components/"; fi
    pkgbuild --root "$ROOT" --identifier "com.mistycat.$LOWER.pkg" --version "$VERSION" \
             --install-location / "$TMP/component.pkg"
    OUT="$INSTALLER_DIR/$NAME $VERSION.pkg"
    if [ -n "$INSTALLER_IDENTITY" ]; then
        productbuild --package "$TMP/component.pkg" --sign "$INSTALLER_IDENTITY" --timestamp "$OUT"
        xcrun notarytool submit "$OUT" --keychain-profile "$KEYCHAIN_PROFILE" --wait
        xcrun stapler staple "$OUT"
    else
        productbuild --package "$TMP/component.pkg" "$OUT"
    fi
    rm -rf "$TMP"
    echo "OK -> $OUT"
done
