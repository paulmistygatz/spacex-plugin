#!/bin/bash
# make_installer_mac.sh
#
# Baut aus dem fertigen "Space X.vst3" eine macOS-Installationsdatei
# (.pkg), die das Plugin nach /Library/Audio/Plug-Ins/VST3 installiert.
#
# Aufruf (im Projektordner):   ./make_installer_mac.sh
# Ergebnis:                    dist/Space X Installer.pkg
#
# Optional (fuer Weitergabe ohne Gatekeeper-Warnung): vorher
# ./sign_and_notarize.sh laufen lassen und unten INSTALLER_IDENTITY
# eintragen ("Developer ID Installer: ..."), dann wird auch das .pkg
# signiert und notarisiert. Ohne Signatur laeuft das .pkg lokal
# problemlos (Rechtsklick -> Oeffnen).

set -e
cd "$(dirname "$0")"

PRODUCT="Space X"
VERSION="1.0.0"
IDENT="com.yourcompany.spacex.vst3"
INSTALLER_IDENTITY=""          # z. B. "Developer ID Installer: Dein Name (TEAMID)"
KEYCHAIN_PROFILE="notary-profile"

VST3="build/LCRMSPlugin_artefacts/Release/VST3/$PRODUCT.vst3"
[ -d "$VST3" ] || VST3="build/LCRMSPlugin_artefacts/VST3/$PRODUCT.vst3"
if [ ! -d "$VST3" ]; then
    echo "==> Plugin noch nicht gebaut - baue Release ..."
    cmake --build build --config Release
    [ -d "build/LCRMSPlugin_artefacts/Release/VST3/$PRODUCT.vst3" ] && VST3="build/LCRMSPlugin_artefacts/Release/VST3/$PRODUCT.vst3"
fi
[ -d "$VST3" ] || { echo "Fehler: $VST3 nicht gefunden."; exit 1; }

rm -rf dist/pkgroot dist/tmp
mkdir -p "dist/pkgroot/Library/Audio/Plug-Ins/VST3" dist/tmp
cp -R "$VST3" "dist/pkgroot/Library/Audio/Plug-Ins/VST3/"

echo "==> Baue Komponenten-Paket ..."
pkgbuild --root dist/pkgroot \
         --identifier "$IDENT" \
         --version "$VERSION" \
         --install-location / \
         "dist/tmp/$PRODUCT-vst3.pkg"

cat > dist/tmp/distribution.xml <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>$PRODUCT $VERSION</title>
    <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <welcome file="welcome.txt"/>
    <choices-outline>
        <line choice="default"><line choice="vst3"/></line>
    </choices-outline>
    <choice id="default"/>
    <choice id="vst3" visible="false">
        <pkg-ref id="$IDENT"/>
    </choice>
    <pkg-ref id="$IDENT" version="$VERSION" onConclusion="none">$PRODUCT-vst3.pkg</pkg-ref>
</installer-gui-script>
EOF
cat > dist/tmp/welcome.txt <<EOF
$PRODUCT $VERSION

Installiert das VST3-Plugin nach:
/Library/Audio/Plug-Ins/VST3/$PRODUCT.vst3

Danach die DAW neu starten bzw. die Plugin-Liste neu scannen.
EOF

OUT="dist/$PRODUCT Installer.pkg"
echo "==> Baue Installer ..."
if [ -n "$INSTALLER_IDENTITY" ]; then
    productbuild --distribution dist/tmp/distribution.xml --resources dist/tmp --package-path dist/tmp \
                 --sign "$INSTALLER_IDENTITY" --timestamp "$OUT"
    echo "==> Notarisiere Installer ..."
    xcrun notarytool submit "$OUT" --keychain-profile "$KEYCHAIN_PROFILE" --wait
    xcrun stapler staple "$OUT"
else
    productbuild --distribution dist/tmp/distribution.xml --resources dist/tmp --package-path dist/tmp "$OUT"
fi

rm -rf dist/pkgroot dist/tmp
echo ""
echo "Fertig: $OUT"
