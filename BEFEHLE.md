# SpaceX – Terminal-Spickzettel

## 0. Immer zuerst (neues Terminal-Fenster)

```
cd "/Users/paule/PROJEKTE/CLAUDE/Plugin Imaging/LCRMSPlugin"
```

Alles Weitere setzt voraus, dass du in diesem Ordner stehst.
Tipp: Pfeiltaste ↑ holt frühere Befehle zurück.

---

## 1. Mac bauen und installieren

```
./install.sh
```

Baut Release, kopiert nach `/Library/Audio/Plug-Ins/VST3/SpaceX.vst3`
(fragt einmal nach deinem Mac-Passwort) und prüft am Ende selbst:

```
Architekturen:  x86_64 arm64
Mindest-macOS:
  minos 10.15
```

Stimmt das nicht, läuft es nicht auf fremden Rechnern.

---

## 2. Windows-Version anstoßen

```
git add -A
git commit -m "kurze Beschreibung was sich geändert hat"
git push
```

Der Push startet den Build auf GitHub. Nach ~10 Minuten:

https://github.com/paulmistygatz/spacex-plugin/actions
→ obersten Lauf anklicken → unten **Artifacts** → `SpaceX-VST3-Windows.zip`

---

## 3. Installer (.pkg) bauen

```
./make_installer_mac.sh
```

Ergebnis: `dist/SpaceX 1.0.0.pkg`
Enthält Plugin **und** Anleitung. Vor jeder Weitergabe die Version in
`make_installer_mac.sh` und `CMakeLists.txt` hochzählen.

---

## 4. Seriennummern erzeugen

```
python3 tools/make_serials.py 20
python3 tools/make_serials.py 1 --name "Jeff Ellis"
```

Die zweite Form ist aus dem Namen abgeleitet: derselbe Name ergibt immer
dieselbe Nummer. Praktisch, wenn jemand seine Nummer verloren hat.

---

## 5. Prüfen / testen

```
./tools/check_binary.sh                 # Architekturen + Mindest-macOS
./tools/latency-test/run.sh             # Galaxy-Latenz gemeldet vs. echt
git status                              # was habe ich geändert?
git log --oneline | head -10            # letzte Änderungen
```

**Demo-Modus nochmal testen** (löscht die eingetragene Seriennummer,
sonst nichts – alle anderen Einstellungen bleiben):

```
/usr/libexec/PlistBuddy -c "Delete :licence" \
  ~/Library/Application\ Support/SpaceX/SpaceX.settings 2>/dev/null; echo ok
```

Danach DAW neu starten. Die DEMO-Plakette steht wieder neben dem Slogan.

---

## Wenn etwas klemmt

| Problem | Befehl |
|---|---|
| Build hakt, Fehler ergeben keinen Sinn | `rm -rf build && ./install.sh` (dauert ~10 min) |
| Push abgelehnt | Token-Rechte prüfen: braucht **Contents** und **Workflows** je „Read and write" |
| Plugin taucht in der DAW nicht auf | `./tools/check_binary.sh` – meist zu hohe Mindest-macOS-Version |
