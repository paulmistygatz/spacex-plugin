#!/bin/bash
# make_installer_mac.sh
#
# Baut aus dem fertigen SpaceX.vst3 eine macOS-Installationsdatei (.pkg).
# Installiert wird:
#   /Library/Audio/Plug-Ins/VST3/SpaceX.vst3
#   /Library/Audio/Plug-Ins/Components/SpaceX.component   (Audio Unit, ab 1.0.1)
#   /Library/Audio/Plug-Ins/Documentation/SpaceX/SpaceX Manual (EN).pdf
#
# Der Dokumentationspfad ist kein Zufall: openManual() im Plugin sucht die
# Anleitung genau dort zuerst. Nur wenn sie fehlt, wird die eingebettete
# Fassung nach "Application Support" entpackt.
#
# Dazu (20.11., Paul): die sechs Einzel-Plugins aus singles/ (VST3 + AU)
# - wer SpaceX kauft, bekommt die Module mit.
#
# Aufruf:     ./make_installer_mac.sh
# Ergebnis:   ~/PROJEKTE/CLAUDE/_Installer/SpaceX <Version>.pkg
#             (Sammelordner fuer alle Installer, Version aus CMakeLists.txt)
#
# Ohne Signatur laeuft das .pkg lokal problemlos; beim Empfaenger meldet
# sich Gatekeeper (Rechtsklick -> Oeffnen, oder Systemeinstellungen ->
# Datenschutz & Sicherheit -> "Trotzdem oeffnen"). Fuer den Verkauf:
# ./sign_and_notarize.sh einrichten und unten INSTALLER_IDENTITY setzen.

set -e
cd "$(dirname "$0")"

PRODUCT="Orbit"   # Runde 190: frueher SpaceX
# 1.0.1: Version direkt aus CMakeLists.txt - nie wieder zwei Stellen pflegen.
VERSION="$(sed -n 's/^project(LCRMSPlugin VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)"
[ -n "$VERSION" ] || { echo "Fehler: Version in CMakeLists.txt nicht gefunden."; exit 1; }
IDENT_AU="com.paulmisty.orbit.au"
IDENT_VST3="com.paulmisty.orbit.vst3"
IDENT_DOCS="com.paulmisty.orbit.docs"
IDENT_SINGLES="com.paulmisty.orbit.singles"
INSTALLER_IDENTITY=""          # z. B. "Developer ID Installer: Paul Misty (TEAMID)"
KEYCHAIN_PROFILE="notary-profile"

# Sammelordner fuer alle fertigen Installer (Paul, 20.11.)
INSTALLER_DIR="$HOME/PROJEKTE/CLAUDE/_Installer"
SINGLES="Micropitch LCR Polarity MidSide Autopan Phaser"

MANUAL_SRC="docs/SpaceXManual_EN.pdf"
MANUAL_DST="Orbit Manual (EN).pdf"

# ---- Plugin finden -----------------------------------------------------
# Review 1.0.1: zuerst den Release-Build nehmen. .../VST3 ohne Release ist ein
# alter Build ohne Optimierung (vor der Umstellung auf Release).
VST3="build/LCRMSPlugin_artefacts/Release/VST3/$PRODUCT.vst3"
[ -d "$VST3" ] || VST3="build/LCRMSPlugin_artefacts/VST3/$PRODUCT.vst3"
if [ ! -d "$VST3" ]; then
    echo "==> Plugin noch nicht gebaut - baue Release ..."
    cmake --build build --config Release
    [ -d "build/LCRMSPlugin_artefacts/VST3/$PRODUCT.vst3" ] && VST3="build/LCRMSPlugin_artefacts/VST3/$PRODUCT.vst3"
    [ -d "build/LCRMSPlugin_artefacts/Release/VST3/$PRODUCT.vst3" ] && VST3="build/LCRMSPlugin_artefacts/Release/VST3/$PRODUCT.vst3"
fi
[ -d "$VST3" ] || { echo "Fehler: $PRODUCT.vst3 nicht gefunden."; exit 1; }
echo "==> Plugin: $VST3"
AU="build/LCRMSPlugin_artefacts/Release/AU/$PRODUCT.component"
[ -d "$AU" ] || AU="build/LCRMSPlugin_artefacts/AU/$PRODUCT.component"
HAVE_AU=0
if [ -d "$AU" ]; then HAVE_AU=1; echo "==> AU:     $AU"; else echo "Hinweis: kein AU gefunden - Installer nur mit VST3."; fi

# ---- Selbsttest VOR dem Verpacken -------------------------------------
# Ein Installer mit falscher Mindest-macOS-Version faellt sonst erst beim
# Empfaenger auf - und dort ohne jede Fehlermeldung (siehe tools/check_binary.sh).
echo "==> Selbsttest:"
./tools/check_binary.sh "$VST3/Contents/MacOS/$PRODUCT" | sed 's/^/    /'
echo ""

# ---- Paketwurzeln bauen ------------------------------------------------
rm -rf dist/pkgroot-vst3 dist/pkgroot-au dist/pkgroot-docs dist/tmp
mkdir -p "dist/pkgroot-vst3/Library/Audio/Plug-Ins/VST3" dist/tmp
cp -R "$VST3" "dist/pkgroot-vst3/Library/Audio/Plug-Ins/VST3/"
if [ "$HAVE_AU" = "1" ]; then
    mkdir -p "dist/pkgroot-au/Library/Audio/Plug-Ins/Components"
    cp -R "$AU" "dist/pkgroot-au/Library/Audio/Plug-Ins/Components/"
fi

# ---- Einzel-Plugins (singles/) ----------------------------------------
echo "==> Baue die Einzel-Plugins ..."
[ -f singles/build/CMakeCache.txt ] || cmake -S singles -B singles/build -DCMAKE_BUILD_TYPE=Release
ST=""
for t in $SINGLES; do ST="$ST ${t}_VST3 ${t}_AU"; done
cmake --build singles/build --config Release --parallel --target $ST
rm -rf dist/pkgroot-singles
mkdir -p "dist/pkgroot-singles/Library/Audio/Plug-Ins/VST3" "dist/pkgroot-singles/Library/Audio/Plug-Ins/Components"
for t in $SINGLES; do
    ART="singles/build/${t}_artefacts"
    [ -d "$ART/Release" ] && ART="$ART/Release"
    V=$(ls -d "$ART"/VST3/*.vst3 | head -1)
    [ -n "$V" ] || { echo "Fehler: $t.vst3 fehlt."; exit 1; }
    cp -R "$V" "dist/pkgroot-singles/Library/Audio/Plug-Ins/VST3/"
    A=$(ls -d "$ART"/AU/*.component 2>/dev/null | head -1 || true)
    [ -n "$A" ] && cp -R "$A" "dist/pkgroot-singles/Library/Audio/Plug-Ins/Components/"
    echo "    + $(basename "$V")"
done

HAVE_DOCS=0
if [ -f "$MANUAL_SRC" ]; then
    mkdir -p "dist/pkgroot-docs/Library/Audio/Plug-Ins/Documentation/$PRODUCT"
    cp "$MANUAL_SRC" "dist/pkgroot-docs/Library/Audio/Plug-Ins/Documentation/$PRODUCT/$MANUAL_DST"
    HAVE_DOCS=1
else
    echo "Hinweis: $MANUAL_SRC fehlt - Installer ohne Anleitung."
fi

echo "==> Baue Komponenten-Pakete ..."
# Runde 190: SpaceX heisst jetzt Orbit (gleiche Plugin-ID) - das alte Bundle
# wird vor der Installation entfernt, sonst sieht der Host das Plugin doppelt.
mkdir -p dist/tmp/scripts
cat > dist/tmp/scripts/preinstall <<'PRE'
#!/bin/sh
rm -rf "/Library/Audio/Plug-Ins/VST3/SpaceX.vst3" "/Library/Audio/Plug-Ins/Components/SpaceX.component"
exit 0
PRE
chmod +x dist/tmp/scripts/preinstall
pkgbuild --root dist/pkgroot-vst3 --identifier "$IDENT_VST3" --version "$VERSION" \
         --scripts dist/tmp/scripts --install-location / "dist/tmp/$PRODUCT-vst3.pkg"
if [ "$HAVE_AU" = "1" ]; then
    pkgbuild --root dist/pkgroot-au --identifier "$IDENT_AU" --version "$VERSION" \
             --install-location / "dist/tmp/$PRODUCT-au.pkg"
fi
pkgbuild --root dist/pkgroot-singles --identifier "$IDENT_SINGLES" --version "$VERSION" \
         --install-location / "dist/tmp/$PRODUCT-singles.pkg"
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
  [ "$HAVE_AU" = "1" ] && echo '            <line choice="au"/>'
  echo '            <line choice="singles"/>'
  [ "$HAVE_DOCS" = "1" ] && echo '            <line choice="docs"/>'
  echo '        </line>'
  echo '    </choices-outline>'
  echo '    <choice id="default"/>'
  echo "    <choice id=\"vst3\" visible=\"false\"><pkg-ref id=\"$IDENT_VST3\"/></choice>"
  echo "    <pkg-ref id=\"$IDENT_VST3\" version=\"$VERSION\" onConclusion=\"none\">$PRODUCT-vst3.pkg</pkg-ref>"
  if [ "$HAVE_AU" = "1" ]; then
    echo "    <choice id=\"au\" visible=\"false\"><pkg-ref id=\"$IDENT_AU\"/></choice>"
    echo "    <pkg-ref id=\"$IDENT_AU\" version=\"$VERSION\" onConclusion=\"none\">$PRODUCT-au.pkg</pkg-ref>"
  fi
  echo "    <choice id=\"singles\" visible=\"false\"><pkg-ref id=\"$IDENT_SINGLES\"/></choice>"
  echo "    <pkg-ref id=\"$IDENT_SINGLES\" version=\"$VERSION\" onConclusion=\"none\">$PRODUCT-singles.pkg</pkg-ref>"
  if [ "$HAVE_DOCS" = "1" ]; then
    echo "    <choice id=\"docs\" visible=\"false\"><pkg-ref id=\"$IDENT_DOCS\"/></choice>"
    echo "    <pkg-ref id=\"$IDENT_DOCS\" version=\"$VERSION\" onConclusion=\"none\">$PRODUCT-docs.pkg</pkg-ref>"
  fi
  echo '</installer-gui-script>'
} > dist/tmp/distribution.xml

cat > dist/tmp/welcome.txt <<WELCOME
$PRODUCT $VERSION

This installs:

  /Library/Audio/Plug-Ins/VST3/$PRODUCT.vst3
  /Library/Audio/Plug-Ins/Components/$PRODUCT.component  (Audio Unit)
  /Library/Audio/Plug-Ins/Documentation/$PRODUCT/$MANUAL_DST

plus the single modules as their own plug-ins (VST3 + Audio Unit):
  Micropitch, LCR, Polarity, Mid-Side, Autopan, Phaser

Afterwards, restart your DAW or let it rescan its plug-ins.

Without a serial, Orbit runs as a demo: everything works, but about every
50 seconds the sound goes quiet for a moment. To activate, click the logo
in the plugin and enter your name and serial.
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

rm -rf dist/pkgroot-vst3 dist/pkgroot-au dist/pkgroot-docs dist/pkgroot-singles dist/tmp
mkdir -p "$INSTALLER_DIR"
cp -f "$OUT" "$INSTALLER_DIR/"
echo ""
echo "Fertig: $INSTALLER_DIR/$(basename "$OUT")"
ls -lh "$INSTALLER_DIR/$(basename "$OUT")"
