# Morgen — Stand vom 20.09., nachts

Vier Änderungen, **vier getrennte Commits**. Du kannst jeden einzeln bauen und
hören. Gebaut habe ich nichts — auf der Linux-VM, auf die ich zugreife, gibt es
weder cmake noch Xcode.

## Der schnellste Weg

```
cd "/Users/paule/PROJEKTE/CLAUDE/Plugin Imaging/LCRMSPlugin"
./install.sh
```

Das ist alles zusammen. Wenn etwas komisch klingt, kannst du damit schrittweise
zurück (siehe unten).

---

## Die vier Stände

| # | Commit | Was drin ist |
|---|---|---|
| 1 | `d8453f2` | Timewarp-Balance greift kräftiger |
| 2 | `99fd08f` | **Auto Gain** |
| 3 | `22f81e1` | Bass-Guard 120 Hz, drei Focus-Knöpfe raus |
| 4 | `cfb93d3` | Solo raus, neuer Sektionskopf |

**Einen Stand einzeln bauen:**
```
git checkout <commit>      # z. B. git checkout 99fd08f
./install.sh
```
**Wieder auf alles:**
```
git checkout main
./install.sh
```

---

## 1 · Timewarp-Balance (`d8453f2`)

Du hattest „+10 %" geschätzt — das wären 0,45 dB gewesen, also nichts. Der
Ausgleich lag bei 1 ms Verzögerung bei **1,8 dB**; nötig sind eher 6–10 dB.
Außerdem hing er am Prozentwert des Reglers statt an der Verzögerungszeit,
und die Skala ist krumm (0–80 % = 0–2 ms, die letzten 20 % springen auf 20 ms).

Jetzt: **3,75 dB pro Millisekunde**, ab 2 ms eingefroren. Bei 1 ms also 3,75 statt
1,8 dB, bei 2 ms 7,5 statt 3,6.

**Hören:** Drift auf ca. 50 %, Balance an/aus. Wenn es jetzt zu weit auf die
andere Seite zieht, sag eine Zahl — der Faktor steht in einer Zeile.

## 2 · Auto Gain (`99fd08f`) — das Wichtigste

Standardmäßig **an**. Im Footer über MIX/VOL steht der angewandte Wert.
Abschalten: Settings → Behaviour → Auto Gain.

- K-gewichtet gemessen (Hochpass 60 Hz + Hochschelf +4 dB @ 1,5 kHz), weil
  Seitenenergie den nackten RMS stärker hebt als die empfundene Lautheit
- **Null Latenz** — rein rückwärtsgewandt, kein Lookahead
- Träge (~0,3 s Messung + ~0,35 s Regelung), damit daraus kein Kompressor wird
- Die ersten 0,5 s nach dem Start laufen schnell → ein Offline-Bounce schwingt
  nicht hörbar ein
- Sitzt **vor** dem VOL-Trim, damit VOL ein echter Trim bleibt
- ±12 dB begrenzt, bei Stille eingefroren
- Aus dem Smart-Pool ausgenommen

**Hören:** Ein Preset laden, Bypass hin und her. Der Unterschied ist jetzt der
Unterschied — nicht der Pegel. Dann einmal Side Boost und einmal Width
aufdrehen und auf die dB-Zahl schauen: die sagt dir, welche Sektion Pegel macht
und welche wirklich am Bild arbeitet.

## 3 · Bass-Guard + Focus-Knöpfe raus (`22f81e1`)

Unter **120 Hz** bleibt das Stereobild unangetastet, fest verdrahtet, ohne
Bedienelement. In Galaxy und Dimension jeweils als „Original + Hochpass(Änderung)",
also ohne Kammfilter; bei neutralen Einstellungen bitgenau wie vorher.

Die drei Focus-Bypass-Knöpfe sind unsichtbar und wirkungslos. Der Focus-Bereich
selbst ist geblieben — den beurteilst du jetzt mit Auto Gain neu.

**Hören:** Etwas mit Bass, Width weit auf. Der Bass sollte stehen bleiben statt
zu zerfasern.

## 4 · Solo raus, neuer Sektionskopf (`cfb93d3`)

Die Sektionsköpfe tragen jetzt **Name links, Lock rechts** — das Mod-Paar liegt
dazwischen, links vom Lock. Der Name bewegt sich nie, egal was rechts steht.

Solo ist unsichtbar und fest auf „aus" gezogen, auch für alte Presets und
Offline-Rendering. Knopf und Parameter bleiben im Code.

**Schauen:** Steht der Name überall auf einer Linie? Liegen die Lock-Icons
untereinander?

---

## Was ich NICHT gemacht habe und warum

- **Life-Regler statt der 14 Mod-Bedienelemente** — der größte Eingriff von
  allen, berührt sieben Sektionen, Parameter und Presets. Den wollte ich nicht
  über Nacht blind machen, ohne dass du den neuen Sektionskopf gesehen hast.
- **Distance + Elevate zusammenlegen, Vision-Width raus, Polarity 4 → 2** —
  alle drei entfernen Parameter und brechen damit deine bestehenden Presets.
  Das ist ein bewusster Schnitt, den du wach treffen solltest.
- **„Keep Solo When Off"** steht noch im Menü, ist aber jetzt wirkungslos.
  Fliegt raus, sobald du bestätigst, dass Solo weg bleibt.

## Wenn etwas nicht baut

```
rm -rf build && ./install.sh
```
Und schick mir die Fehlerausgabe. Jeder Commit ist einzeln syntaxgeprüft
(`g++ -fsyntax-only` über Editor, Prozessor und Goniometer), aber das fängt
keine Link- oder Logikfehler.
