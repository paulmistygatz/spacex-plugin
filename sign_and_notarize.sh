#!/bin/bash
# sign_and_notarize.sh
#
# Signiert, notarisiert und staplet "Space X.vst3" nach dem Build, damit das
# Plugin bei JEDEM Empfaenger ohne Gatekeeper-Warnung ("kann nicht auf
# Schadsoftware geprueft werden") laeuft - auch offline.
#
# EINMALIGE VORAUSSETZUNGEN (bevor du dieses Script zum ersten Mal nutzt):
#   1. Apple Developer Program Mitgliedschaft (developer.apple.com, 99$/Jahr)
#   2. In Xcode: Settings -> Accounts -> Account hinzufuegen ->
#      "Manage Certificates" -> "+" -> "Developer ID Application"
#      (legt das Signing-Zertifikat automatisch im Schluesselbund ab)
#   3. App-spezifisches Passwort erzeugen: appleid.apple.com ->
#      Sign-In and Security -> App-Specific Passwords
#   4. Einmalig Notarisierungs-Zugangsdaten lokal speichern:
#      xcrun notarytool store-credentials "notary-profile" \
#        --apple-id "deine@appleid.de" \
#        --team-id "DEINETEAMID" \
#        --password "das-app-spezifische-passwort"
#
# Danach unten SIGN_IDENTITY (Team-ID) einmal eintragen - dann reicht
# kuenftig nur noch: ./sign_and_notarize.sh
#
# -----------------------------------------------------------------------

set -e

# ---- Anpassen ----------------------------------------------------------
SIGN_IDENTITY="Developer ID Application: DEIN NAME (DEINETEAMID)"
KEYCHAIN_PROFILE="notary-profile"
VST3_PATH="build/LCRMSPlugin_artefacts/Release/VST3/Space X.vst3"
# -------------------------------------------------------------------------

if [ ! -d "$VST3_PATH" ]; then
    echo "Fehler: '$VST3_PATH' nicht gefunden."
    echo "Pfad oben in VST3_PATH anpassen (haengt vom CMake-Projektnamen ab)."
    exit 1
fi

echo "==> Signiere $VST3_PATH ..."
codesign --deep --force --options runtime --timestamp \
    --sign "$SIGN_IDENTITY" \
    "$VST3_PATH"

echo "==> Pruefe Signatur ..."
codesign --verify --deep --strict --verbose=2 "$VST3_PATH"

ZIP_PATH="${VST3_PATH%.vst3}.zip"
echo "==> Packe fuer Notarisierung ..."
ditto -c -k --keepParent "$VST3_PATH" "$ZIP_PATH"

echo "==> Reiche zur Notarisierung ein (kann 1-15 Minuten dauern) ..."
xcrun notarytool submit "$ZIP_PATH" --keychain-profile "$KEYCHAIN_PROFILE" --wait

echo "==> Staple Notarisierungs-Ticket ins Bundle ..."
xcrun stapler staple "$VST3_PATH"

rm -f "$ZIP_PATH"

echo ""
echo "Fertig! '$VST3_PATH' ist signiert, notarisiert und gestapelt."
echo "Kann jetzt an jeden weitergegeben werden - keine Gatekeeper-Warnung mehr."
