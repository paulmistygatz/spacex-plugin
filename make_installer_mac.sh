#!/bin/bash
# make_installer_mac.sh
#
# Baut aus dem fertigen SpaceX.vst3 eine macOS-Installationsdatei (.pkg).
# Installiert wird:
#   /Library/Audio/Plug-Ins/VST3/SpaceX.vst3
#   /Library/Audio/Plug-Ins/Documentation/SpaceX/SpaceX Manual (EN).pdf
#
# Der Dokumentationspfad ist kein Zufall: openManual() im Plugin sucht die
# Anleitung genau dort zuerst. Nur wenn sie fehlt, wird die eingebettete
# Fassung nach "Application Support" entpackt.
#
# Aufruf:     ./make_installer_mac.sh
# Ergebnis:   dist/SpaceX 1.0.0.pkg
#
# Ohne Signatur laeuft das .pkg lokal problemlos; beim Empfaenger meldet
# sich Gatekeeper (Rechtsklick -> Oeffnen, oder Systemeinstellungen ->
# Datenschutz & Sicherheit -> "Trotzdem oeffnen"). Fuer den Verkauf:
# ./sign_and_notarize.sh einrichten und unten INSTALLER_IDENTITY setzen.

set -e
cd "$(dirname "$0")"

PRODUCT="SpaceX"
VERSION="1.0.0"
IDENT_VST3="com.paulmisty.spacex.vst3"
IDENT_DOCS="com.paulmisty.spacex.docs"
INSTALLER_IDENTITY=""          # z. B. "Developer ID Installer: Paul Misty (TEAMID)"
KEYCHAIN_PROFILE="notary-profile"

MANUAL_SRC="docs/SpaceXManual_EN.pdf"
MANUAL_DST="SpaceX Manual (EN).pdf"

# ---- Plugin finden -----------------------------------------------------
VST3="build/LCRMSPlugin_artefacts/VST3/$PRODUCT.vst3"
[ -d "$VST3" ] || VST3="build/LCRMSPlugin_artefacts/Release/VST3/$PRODUCT.vst3"
if [ ! -d "$VST3" ]; then
    echo "==> Plugin noch nicht gebaut - baue Release ..."
    cmake --build build --config Release
    [ -d "build/LCRMSPlugin_artefacts/VST3/$PRODUCT.vst3" ] && VST3="build/LCRMSPlugin_artefacts/VST3/$PRODUCT.vst3"
    [ -d "build/LCRMSPlugin_artefacts/Release/VST3/$PRODUCT.vst3" ] && VST3="build/LCRMSPlugin_artefacts/Release/VST3/$PRODUCT.vst3"
fi
[ -d "$VST3" ] || { echo "Fehler: $PRODUCT.vst3 nicht gefunden."; exit 1; }
echo "==> Plugin: $VST3"

# ---- Selbsttest VOR dem Verpacken -------------------------------------
# Ein Installer mit falscher Mindest-macOS-Version faellt sonst erst beim
# Empfaenger auf - und dort ohne jede Fehlermeldung (siehe tools/check_binary.sh).
echo "==> Selbsttest:"
./tools/check_binary.sh "$VST3/Contents/MacOS/$PRODUCT" | sed 's/^/    /'
echo ""

# ---- Paketwurzeln bauen ------------------------------------------------
rm -rf dist/pkgroot-vst3 dist/pkgroot-docs dist/tmp
mkdir -p "dist/pkgroot-vst3/Library/Audio/Plug-Ins/VST3" dist/tmp
cp -R "$VST3" "dist/pkgroot-vst3/Library/Audio/Plug-Ins/VST3/"

HAVE_DOCS=0
if [ -f "$MANUAL_SRC" ]; then
    mkdir -p "dist/pkgroot-docs/Library/Audio/Plug-Ins/Documentation/$PRODUCT"
    cp "$MANUAL_SRC" "dist/pkgroot-docs/Library/Audio/Plug-Ins/Documentation/$PRODUCT/$MANUAL_DST"
    HAVE_DOCS=1
else
    echo "Hinweis: $MANUAL_SRC fehlt - Installer ohne Anleitung."
fi

echo "==> Baue Komponenten-Pakete ..."
pkgbuild --root dist/pkgroot-vst3 --identifier "$IDENT_VST3" --version "$VERSION" \
         --install-location / "dist/tmp/$PRODUCT-vst3.pkg"
if [ "$HAVE_DOCS" = "1" ]; then
    pkgbuild --root dist/pkgroot-docs --identifier "$IDENT_DOCS" --version "$VERSION" \
             --install-location / "dist/tmp/$PRODUCT-docs.pkg"
fi

# ---- Verteilungs-Beschreibung -----------------------------------------
{
  echo '<?xml version="1.0" encoding="utf-8"?>'
  echo '<installer-gui-script minSpecVersion="1">'
  echo "    <title>$PRODUCT $VERSION</title>"
  echo '    <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>'
  echo '    <welcome file="welcome.txt"/>'
  echo '    <choices-outline>'
  echo '        <line choice="default">'
  echo '            <line choice="vst3"/>'
  [ "$HAVE_DOCS" = "1" ] && echo '            <line choice="docs"/>'
  echo '        </line>'
  echo '    </choices-outline>'
  echo '    <choice id="default"/>'
  echo "    <choice id=\"vst3\" visible=\"false\"><pkg-ref id=\"$IDENT_VST3\"/></choice>"
  echo "    <pkg-ref id=\"$IDENT_VST3\" version=\"$VERSION\" onConclusion=\"none\">$PRODUCT-vst3.pkg</pkg-ref>"
  if [ "$HAVE_DOCS" = "1" ]; then
    echo "    <choice id=\"docs\" visible=\"false\"><pkg-ref id=\"$IDENT_DOCS\"/></choice>"
    echo "    <pkg-ref id=\"$IDENT_DOCS\" version=\"$VERSION\" onConclusion=\"none\">$PRODUCT-docs.pkg</pkg-ref>"
  fi
  echo '</installer-gui-script>'
} > dist/tmp/distribution.xml

cat > dist/tmp/welcome.txt <<WELCOME
$PRODUCT $VERSION

Installiert wird:

  /Library/Audio/Plug-Ins/VST3/$PRODUCT.vst3
  /Library/Audio/Plug-Ins/Documentation/$PRODUCT/$MANUAL_DST

Danach die DAW neu starten oder die Plugin-Liste neu scannen lassen.

Ohne Seriennummer laeuft SpaceX im Demo-Modus: vollstaendig, aber alle
50 Sekunden fuer gut drei Sekunden leise. Die Seriennummer traegst du im
Plugin unter Settings -> Activate ein.
WELCOME

OUT="dist/$PRODUCT $VERSION.pkg"
echo "==> Baue Installer ..."
if [ -n "$INSTALLER_IDENTITY" ]; then
    productbuild --distribution dist/tmp/distribution.xml --resources dist/tmp \
                 --package-path dist/tmp --sign "$INSTALLER_IDENTITY" --timestamp "$OUT"
    echo "==> Notarisiere ..."
    xcrun notarytool submit "$OUT" --keychain-profile "$KEYCHAIN_PROFILE" --wait
    xcrun stapler staple "$OUT"
else
    productbuild --distribution dist/tmp/distribution.xml --resources dist/tmp \
                 --package-path dist/tmp "$OUT"
fi

rm -rf dist/pkgroot-vst3 dist/pkgroot-docs dist/tmp
echo ""
echo "Fertig: $OUT"
ls -lh "$OUT"
